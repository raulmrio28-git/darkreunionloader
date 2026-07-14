#include "dcc/plat.h"
#include "io/io_type.h"

/*
The following is code for USB I/O, adapted from the unused
ap_usb.c source file in QSC11x0 ARMPRG sources for use with
QSC60x5 devices. Credits to QCOM for this.

The register locations are the same as MSM6280, albeit
different base address.
*/

#pragma pack(1)
//Equivalates to serial_iface_info_type
typedef struct {
  uint32_t baud;
  uint8_t stopbit;
  uint8_t parity;
  uint8_t databits;
  uint8_t cd;
  uint8_t ri;
  uint8_t dsr;
  uint8_t sbd;
  uint8_t rts;
} qsc60x5_usb_serial_t;

typedef enum {
    USBDC_EP_DIR_OUT = 0,
    USBDC_EP_DIR_IN = 1
} qsc60x5_ep_dir_e;

typedef enum {
    USBDC_CONTROL = 0,
    USBDC_DATA = 2,
    USBDC_DIAG = 4
} qsc60x5_ep_type_e;

#define USB_RX_BUFFER_SIZE 1152
#define USBDC_ENDPOINT_TTLBTECNT_M 0x1FFFFF

typedef struct
{
  uint8_t line_coding[8]; //line coding
  uint8_t tx_buffer[1040]; //TX buffer
  //We need two RX buffers
  uint8_t rx_buffer1[USB_RX_BUFFER_SIZE];
  uint8_t rx_buffer2[USB_RX_BUFFER_SIZE];
  //Buffer idx'es
  uint16_t tx_buf_index;
  uint16_t rx_index;
  //How much did we read?
  uint16_t rx_num_bytes_read;
  //EP attr's
  uint8_t usb_in_endpoint;
  uint8_t usb_out_endpoint;
  //Address
  uint16_t address;
  //Msg flag
  bool setup_data_stage_flag;
} qsc60x5_usb_state_t;

qsc60x5_usb_serial_t USB_Serial;
qsc60x5_usb_state_t USB_State;

static bool wait_for_tx_done = false;
static uint8_t *process_rx_buffer = 0;

uint8_t* USB_rx_buffer;

static void qsc60x5_usb_wait_for_dma_done(int ep_num, qsc60x5_ep_dir_e ep_dir);

static void qsc60x5_usb_ep_clear_xbuf_int_status(int ep_num, qsc60x5_ep_dir_e ep_dir);

static void qsc60x5_usb_ep_clear_ybuf_int_status(int ep_num, qsc60x5_ep_dir_e ep_dir);

static void qsc60x5_usb_ep_disable_done_int(int ep_num, qsc60x5_ep_dir_e ep_dir, bool flush_flag);

static void qsc60x5_usb_ep_disable_ready(int ep_num, qsc60x5_ep_dir_e ep_dir);

static uint32_t * qsc60x5_usb_ep_get_descriptor_addr(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t *ep_mem_ptr;

  ep_mem_ptr = (uint32_t *)0x80016400;
  ep_mem_ptr += (4*2*ep_num );

  if ( ep_dir == USBDC_EP_DIR_IN)
    ep_mem_ptr += 4;

  return ep_mem_ptr;
}

static void qsc60x5_usb_ep_clear_xfilled(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_xfilled_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_xfilled_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_xfilled_mask = 2 << ep_num*2;
    break;
  }

  if (READ_U32(0x8001605C) & ep_xfilled_mask)
      WRITE_U32(0x8001605C, ep_xfilled_mask);

}

static void qsc60x5_usb_ep_clear_yfilled(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_yfilled_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_yfilled_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_yfilled_mask = 2 << ep_num*2;
    break;
  }

  if (READ_U32(0x80016060) & ep_yfilled_mask)
    WRITE_U32(0x80016060, ep_yfilled_mask);
}

static void qsc60x5_usb_ep_clear_done_status(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_clear_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_clear_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_clear_mask = 2 << ep_num*2;
    break;
  }

  if ( READ_U32(0x80016070) & ep_clear_mask )
    WRITE_U32(0x80016070, ep_clear_mask);
}

static void qsc60x5_usb_ep_enable_done_int(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_done_enable_val;
  uint32_t ep_done_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_done_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_done_mask = 2 << ep_num*2;
    break;
  }

  ep_done_enable_val = READ_U32(0x80016074) | ep_done_mask;
  WRITE_U32(0x80016074, ep_done_enable_val);

}

static void qsc60x5_usb_ep_enable_endpoint(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_enable_mask = 0; 
 
  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_enable_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_enable_mask = 2 << ep_num*2;
    break;
  }
  
  if ( !(READ_U32(0x80016064) & ep_enable_mask) ) 
    WRITE_U32(0x80016064, ep_enable_mask);
}

static void qsc60x5_usb_ep_disable_endpoint(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_disable_mask = 0; 

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_disable_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_disable_mask = 2 << ep_num*2;
    break;
  }
  
  if ( READ_U32(0x80016064) & ep_disable_mask )
    WRITE_U32(0x80016064, ep_disable_mask);
}

static void qsc60x5_usb_ep_enable_ready(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_ready_mask =0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_ready_mask = 1<<ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_ready_mask = 2 << ep_num*2;
    break;
  }

  if ( !(READ_U32(0x80016068) & ep_ready_mask))
    WRITE_U32(0x80016068, ep_ready_mask);
}

static void qsc60x5_usb_ep_disable_ready(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_ready_mask = 0; 

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_ready_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_ready_mask = 2 << ep_num*2;
    break;
  }

  WRITE_U32(0x8001607C, ep_ready_mask);  
}

static void qsc60x5_usb_setup_rx_DMA(int ep_num, uint8_t *rx_buffer, int total_byte_count)
{
  uint32_t *ep_mem_ptr;
  uint32_t temp_dword;

  /* set DWORD3 of EP for ttlbtecnt */
  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num, USBDC_EP_DIR_OUT);
  temp_dword = *(ep_mem_ptr +3 );
  temp_dword &= ~USBDC_ENDPOINT_TTLBTECNT_M;
  temp_dword += total_byte_count;
  *(ep_mem_ptr +3 ) = temp_dword;

  /* set system mem start addr */
  *(uint32_t*)(0x80016980+(2*ep_num)) = rx_buffer;

  /* Tell USB core that RX buffer is drained & ready for rxing more data */
  qsc60x5_usb_ep_clear_xfilled(ep_num, USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_yfilled(ep_num, USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_xbuf_int_status(ep_num,  USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_ybuf_int_status(ep_num,  USBDC_EP_DIR_OUT);

  /* Enable DMA */
  WRITE_U32 (0x80016824,  ( 1 << 2*ep_num ) );

  /* Clear the done status from any previous transfer */
  qsc60x5_usb_ep_clear_done_status(ep_num, USBDC_EP_DIR_OUT);
  
  /* Enable the done interrupt status */
  qsc60x5_usb_ep_enable_done_int(ep_num, USBDC_EP_DIR_OUT);

  /* Enable EP */
  qsc60x5_usb_ep_enable_ready(ep_num, USBDC_EP_DIR_OUT);
}

static void qsc60x5_usb_setup_tx_DMA(int ep_num, uint8_t *tx_buffer, int total_byte_count)
{
  uint32_t *ep_mem_ptr;
  uint32_t temp_dword;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num, USBDC_EP_DIR_IN );

  /* There should not an active transfer */
  if ( ((*(ep_mem_ptr +3)) & USBDC_ENDPOINT_TTLBTECNT_M) != 0)
  {
    /* Flush to get back to a known state */
    qsc60x5_usb_ep_disable_done_int(ep_num, USBDC_EP_DIR_IN, true);
  }

  /* set DWORD3 of EP for ttlbtecnt */
  temp_dword = *(ep_mem_ptr +3 );
  temp_dword &= ~USBDC_ENDPOINT_TTLBTECNT_M;
  temp_dword += total_byte_count;
  *(ep_mem_ptr +3 ) = temp_dword;

  /* point to the data */
  *(uint32_t*)(0x80016984+(2*ep_num)) = tx_buffer;

  /* enable this DMA transfer */
  WRITE_U32(0x80016824,  ( 2 << 2*ep_num ) );

  /* Clear X and Y filled status bits */
  qsc60x5_usb_ep_clear_xfilled(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_yfilled(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_xbuf_int_status(ep_num,  USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_ybuf_int_status(ep_num,  USBDC_EP_DIR_IN);
  
  /* Enable the done interrupt status */
  qsc60x5_usb_ep_enable_done_int(ep_num, USBDC_EP_DIR_IN);

  /* ready EP */
  qsc60x5_usb_ep_enable_ready(ep_num, USBDC_EP_DIR_IN);

}

static void qsc60x5_usb_ep_set_toggle_bit(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_toggle_val;
  uint32_t ep_toggle_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_toggle_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_toggle_mask = 2 << ep_num*2;
    break;
  }

  ep_toggle_val = READ_U32(0x80016078);
  if ((ep_toggle_val & ep_toggle_mask) == 0)
  {
    /* Disable endpoint */
    qsc60x5_usb_ep_disable_endpoint(ep_num, ep_dir);

    ep_toggle_val |= ep_toggle_mask;
    WRITE_U32(0x80016078, ep_toggle_val);
    
    /* Enable endpoint */
    qsc60x5_usb_ep_enable_endpoint(ep_num, ep_dir);
  }

}

static void qsc60x5_usb_ep_clear_toggle_bit(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_toggle_val;
  uint32_t ep_toggle_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_toggle_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_toggle_mask = 2 << ep_num*2;
    break;
  }

  ep_toggle_val = READ_U32(0x80016078);
  if (ep_toggle_val & ep_toggle_mask)
    WRITE_U32(0x80016078, ep_toggle_mask);
}

static void qsc60x5_usb_ep_clear_stall(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t *ep_mem_ptr;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num,ep_dir );

  (*ep_mem_ptr) &= ~(0x80000000); 
}

static void qsc60x5_usb_ep_set_xfilled(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_xfilled_mask = 0;

  ep_xfilled_mask = 2 << ep_num*2;

  if ( !(READ_U32(0x80016060) & ep_xfilled_mask) )
    WRITE_U32(0x8001605C, ep_xfilled_mask);
}

static void qsc60x5_usb_clear_DMA_channel(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_mask = 0;

  switch (ep_dir)
  {
  case  USBDC_EP_DIR_OUT:
    ep_mask =  1 << (2*ep_num);
    break;

  case USBDC_EP_DIR_IN:
    ep_mask =  2 << (2*ep_num);
    break;
  }
  if ( READ_U32(0x80016824) & ep_mask)
    WRITE_U32(0x8001684C, ep_mask );
}

static void qsc60x5_usb_ep_disable_done_int(int ep_num, qsc60x5_ep_dir_e ep_dir, bool flush_flag)
{
  uint32_t ep_done_enable_val;
  uint32_t ep_done_mask = 0;
  uint32_t *ep_mem_ptr;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_done_mask = 1 << ep_num*2;
    break;
  case USBDC_EP_DIR_IN:
    ep_done_mask = 2 << ep_num*2;
    break;
  }

  ep_done_enable_val = READ_U32(0x80016074) & ~( ep_done_mask );
  WRITE_U32(0x80016074, ep_done_enable_val);
  
  if (flush_flag )
  {
    qsc60x5_usb_ep_disable_ready(ep_num,ep_dir);
    qsc60x5_usb_ep_clear_done_status(ep_num,ep_dir);

    ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num,ep_dir);
    *(ep_mem_ptr+3) &=  ~USBDC_ENDPOINT_TTLBTECNT_M;  

    if ( (*(ep_mem_ptr+3) &  ~USBDC_ENDPOINT_TTLBTECNT_M) != 0 ) 
      qsc60x5_usb_clear_DMA_channel(ep_num,ep_dir);

    qsc60x5_usb_ep_clear_xfilled(ep_num,ep_dir);
    qsc60x5_usb_ep_clear_yfilled(ep_num,ep_dir);
  }

  qsc60x5_usb_ep_clear_done_status(ep_num,ep_dir);

}

static void qsc60x5_usb_ep_clear_xbuf_int_status(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_mask = 0; 

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_mask = 2 << ep_num*2;
    break;
  }

  if ( READ_U32(0x80016050) & ep_mask )
    WRITE_U32(0x80016050, ep_mask);
}

static void qsc60x5_usb_ep_clear_ybuf_int_status(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_mask = 0; 

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_mask = 2 << ep_num*2;
    break;
  }
  
  if ( READ_U32(0x80016054) & ep_mask )
    WRITE_U32(0x80016054, ep_mask);
}

static void qsc60x5_usb_send_zero_length_pkt(int ep_num)
{
  uint32_t *ep_mem_ptr;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num, USBDC_EP_DIR_IN);

  qsc60x5_usb_ep_clear_xfilled(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_yfilled(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_xbuf_int_status(ep_num,  USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_ybuf_int_status(ep_num,  USBDC_EP_DIR_IN);
  
  qsc60x5_usb_ep_set_xfilled(ep_num, USBDC_EP_DIR_IN);

  qsc60x5_usb_ep_enable_done_int(ep_num, USBDC_EP_DIR_IN);

  qsc60x5_usb_ep_enable_ready(ep_num, USBDC_EP_DIR_IN);
}

static void qsc60x5_usb_wait_for_dma_done(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_mask = 0;

  switch (ep_dir)
  {
  case  USBDC_EP_DIR_OUT:
    ep_mask =  1 << (2*ep_num);
    break;

  case USBDC_EP_DIR_IN:
    ep_mask =  2 << (2*ep_num);
    break;
  }

  while (READ_U32(0x80016824) & ep_mask)
    wdog_reset();

  qsc60x5_usb_ep_disable_done_int(ep_num, ep_dir, false);
}
static void qsc60x5_usb_do_handle_setup_msgs()
{
  uint16_t reply_length;
  uint8_t *reply_data_ptr;
  uint32_t temp_dword;
  uint32_t *buf_dword_ptr = 0x80017000;
  uint32_t *data_mem_ptr, *ep_mem_ptr;
  usbdc_setup_type setup;
  int ep_num;
  qsc60x5_ep_dir_e ep_dir;

  reply_length = 0;
  reply_data_ptr = NULL;

  wdog_reset();

  if (USB_State.setup_data_stage_flag)
  {
    USB_State.setup_data_stage_flag = false;

    *((uint32_t *) &USB_State.line_coding[0]) = *buf_dword_ptr++;

    temp_dword = *buf_dword_ptr++;
    USB_State.line_coding[4] = (uint8_t) (temp_dword & 0xff);

    temp_dword = temp_dword >>8;
    USB_State.line_coding[5] = (uint8_t) (temp_dword & 0xff);

    temp_dword = temp_dword >>8;
    USB_State.line_coding[6] =  (uint8_t) (temp_dword & 0xff);
  }
  else
  {
    temp_dword = *buf_dword_ptr++;
    setup.bmRequest_and_Type = temp_dword;
    setup.wValue = temp_dword >> 16;
    temp_dword = *buf_dword_ptr;
    setup.wIndex = temp_dword;
    setup.wLength = temp_dword >> 16;

    switch (setup.bmRequest_and_Type)
    {
    case USBDC_GET_LINE_CODING:
      {
        reply_data_ptr = USB_State.line_coding;
        reply_length = setup.wLength;
        break;
      }

    case USBDC_SET_CONTROL_LINE_STATE_I:
      {
        break;
      }

    case USBDC_SET_LINE_CODING:
      {
        USB_State.setup_data_stage_flag = true;
        break;
      }

    case USBDC_SETUP_CLEAR_FEATURE_E:
      {
        if ( (setup.wIndex & 0x80) == 0)
        {
          ep_dir = USBDC_EP_DIR_OUT;
        }
        else
        {
          ep_dir = USBDC_EP_DIR_IN;
        }
        ep_num = (int)  (setup.wIndex & 0x0f);


        qsc60x5_usb_ep_clear_toggle_bit(ep_num, ep_dir);   
        
        qsc60x5_usb_ep_clear_stall(ep_num, ep_dir);
        break;
      }

    default:
      break;
    }
  }

  qsc60x5_usb_ep_clear_xfilled(USBDC_CONTROL, USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_yfilled(USBDC_CONTROL, USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_xbuf_int_status(USBDC_CONTROL,  USBDC_EP_DIR_OUT);
  qsc60x5_usb_ep_clear_ybuf_int_status(USBDC_CONTROL,  USBDC_EP_DIR_OUT);

  ep_mem_ptr = (uint32_t *)0x80016400;
  *ep_mem_ptr &= ~(0x40000000);

  ep_mem_ptr += 3;

  (*ep_mem_ptr) &= ~USBDC_ENDPOINT_TTLBTECNT_M;

  if (USB_State.setup_data_stage_flag)
  {
    *(ep_mem_ptr) += setup.wLength;
  }
  else
  {
     *(ep_mem_ptr) += 128;
  }

  qsc60x5_usb_ep_enable_ready(USBDC_CONTROL, USBDC_EP_DIR_OUT);

  if (USB_State.setup_data_stage_flag == false)
  {
    if (reply_length > 0)
    {
      qsc60x5_usb_setup_tx_DMA(USBDC_CONTROL, 
                         reply_data_ptr, 
                         reply_length);
    }
    else
    {
      qsc60x5_usb_send_zero_length_pkt(USBDC_CONTROL);
    }
  }
}

static void qsc60x5_usb_process_ctrl_done()
{
  uint32_t *ep_mem_ptr;
  int num_bytes_leftover;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(USBDC_CONTROL, USBDC_EP_DIR_IN);

  num_bytes_leftover =( *(ep_mem_ptr + 3) ) & USBDC_ENDPOINT_TTLBTECNT_M;

  if (num_bytes_leftover > 0)
    return;

  qsc60x5_usb_ep_clear_xbuf_int_status(USBDC_CONTROL, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_ybuf_int_status(USBDC_CONTROL, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_disable_done_int(USBDC_CONTROL, USBDC_EP_DIR_IN, true);
  qsc60x5_usb_ep_clear_done_status(USBDC_CONTROL, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_set_toggle_bit(USBDC_CONTROL, USBDC_EP_DIR_IN);

  if ( (READ_U32(0x80016044) == 0) && (USB_State.address > 0) )
    WRITE_U32(0x80016044, USB_State.address);
}

static void qsc60x5_usb_process_tx_done(int ep_num)
{
  uint32_t *ep_mem_ptr;
  int num_bytes_leftover;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(ep_num, USBDC_EP_DIR_IN);

  num_bytes_leftover =( *(ep_mem_ptr + 3) ) & USBDC_ENDPOINT_TTLBTECNT_M;

  if (num_bytes_leftover > 0)
    return;

  qsc60x5_usb_ep_clear_xbuf_int_status(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_clear_ybuf_int_status(ep_num, USBDC_EP_DIR_IN);
  qsc60x5_usb_ep_disable_done_int(ep_num, USBDC_EP_DIR_IN, false);
  qsc60x5_usb_ep_clear_done_status(ep_num, USBDC_EP_DIR_IN);
}

static bool qsc60x5_usb_ep_done_status(int ep_num, qsc60x5_ep_dir_e ep_dir)
{
  uint32_t ep_done_mask = 0;

  switch (ep_dir)
  {
  case USBDC_EP_DIR_OUT:
    ep_done_mask = 1 << ep_num*2;
    break;

  case USBDC_EP_DIR_IN:
    ep_done_mask = 2 << ep_num*2;
    break;
  }

  if ( READ_U32(0x80016070) & ep_done_mask )
    return true;

  return false;
}

bool qsc60x5_usb_init(void)
{
  int i = 0;
  uint32_t *ep_mem_ptr;

  if (READ_U32(0x80016044) == 0)
    return false;

  USB_State.tx_buf_index = 0;
  USB_State.rx_index = 0;
  USB_State.rx_num_bytes_read = 0;
  USB_State.setup_data_stage_flag = false;
  USB_State.address = 0;
  for (i = 0; i < USB_RX_BUFFER_SIZE; i++)
  {
    USB_State.rx_buffer1[i] = 0;
    USB_State.rx_buffer2[i] = 0;
  }

  memset(&USB_Serial, 0, sizeof(qsc60x5_usb_serial_t));
	
  //what's needed is baud and databits
  USB_Serial.baud = 115200;
  USB_Serial.databits = 8;

  /* Init line coding data */
  memcpy(USB_State.line_coding,
         (uint8_t*) &(USB_Serial.baud),
         sizeof(uint32_t));
  USB_State.line_coding[4] = 0;
  USB_State.line_coding[5] = 0;
  USB_State.line_coding[6] = USB_Serial.databits;

  ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(USBDC_DIAG, USBDC_EP_DIR_OUT);

  if ( (uint16_t) (*(ep_mem_ptr +3) & USBDC_ENDPOINT_TTLBTECNT_M) )
  {
    USB_State.usb_out_endpoint = USBDC_DIAG;
    USB_State.usb_in_endpoint  = USBDC_DIAG;
  }
  else
  {
    USB_State.usb_out_endpoint = USBDC_DATA;
    USB_State.usb_in_endpoint  = USBDC_DATA;
  }
  
  while (usb_ep_done_status(USB_State.usb_out_endpoint, USBDC_EP_DIR_OUT) == false)
    wdog_reset();

  qsc60x5_usb_ep_enable_done_int(USBDC_CONTROL, USBDC_EP_DIR_OUT);
  USB_rx_buffer = USB_State.rx_buffer1;
  qsc60x5_usb_setup_rx_DMA(USBDC_DIAG, USB_rx_buffer, USB_RX_BUFFER_SIZE);
  return true;
}

bool qsc60x5_usb_active(void)
{
  wdog_reset();
  return true;
}

void qsc60x5_usb_drain()
{
  qsc60x5_usb_wait_for_dma_done(USB_State.usb_in_endpoint, USBDC_EP_DIR_IN);
}

void qsc60x5_usb_read()
{
  uint16_t num_bytes_read = 0;
  uint32_t sys_int_status;
  uint32_t which_ep;
  uint32_t ep_mask = 0;
  uint32_t *ep_mem_ptr;
  int i;

  wdog_reset();

  if (READ_U32(0x80012064)&1 == 0)
    return;
  else
    WRITE_U32(0x80016004, 0x2);

  wdog_reset();

  sys_int_status = READ_U32(0x80016048);

  /* Check for cable pull (suspend) */
  if (sys_int_status & 0x4)
  {
    WRITE_U32(0x80016040,  0x4);
    WRITE_U32(0x80016048,  0x4 | 0x1);
    sys_int_status = READ_U32(0x80016048);
  }

  if (sys_int_status & 0x8)
  {
    which_ep = READ_U32(0x80016070); 
    which_ep &= READ_U32(0x80016074);

    WRITE_U32(0x80016070, which_ep );

    WRITE_U32(0x80016048,  0x8);

    if ( which_ep & 0x2)
      qsc60x5_usb_process_ctrl_done();

    if (which_ep & 0x1)
    { 
      qsc60x5_usb_ep_disable_done_int(USBDC_CONTROL, USBDC_EP_DIR_IN, true);
      qsc60x5_usb_do_handle_setup_msgs();
    }

    ep_mask =  2 << (2*USB_State.usb_in_endpoint);
    if (which_ep & ep_mask)
    { 
      qsc60x5_usb_process_tx_done(USB_State.usb_in_endpoint);
      wait_for_tx_done = false;
    }

    ep_mask =  1 << (2*USB_State.usb_out_endpoint);
    if (which_ep & ep_mask)
      process_rx_buffer = USB_rx_buffer;
      
    if (wait_for_tx_done)
      return;

    if (process_rx_buffer)
    {
      ep_mem_ptr = qsc60x5_usb_ep_get_descriptor_addr(
           USB_State.usb_out_endpoint,
           USBDC_EP_DIR_OUT);

      num_bytes_read = USB_RX_BUFFER_SIZE -
        ((uint16_t) (*(ep_mem_ptr +3) & USBDC_ENDPOINT_TTLBTECNT_M));

      if (USB_rx_buffer == USB_State.rx_buffer1)
        USB_rx_buffer = USB_State.rx_buffer2;
      else
        USB_rx_buffer = USB_State.rx_buffer1;

      qsc60x5_usb_setup_rx_DMA(USBDC_DIAG, USB_rx_buffer, USB_RX_BUFFER_SIZE);
    }
  }
  
  if(num_bytes_read)
  {
     for(i = 0; i < num_bytes_read; i++)
     {
       DRL_Packet_RX(process_rx_buffer[USB_State.rx_index++]);
     }
     USB_State.rx_index = 0;
     process_rx_buffer = 0;
  }

}

void qsc60x5_usb_write(uint32_t b)
{
  USB_State.tx_buffer[USB_State.tx_buf_index++] = b;

  if ((b == 0x7E) && (USB_State.tx_buf_index != 1))
  {
    wdog_reset();
    qsc60x5_usb_setup_tx_DMA(USB_State.usb_in_endpoint, USB_State.tx_buffer, USB_State.tx_buf_index);
    USB_State.tx_buf_index = 0;
    wait_for_tx_done = true;

    while (wait_for_tx_done)
    {
      qsc60x5_usb_read();
      wdog_reset();
    }
  }
}

drl_io_funcs_t io_usb = {
    .init = qsc60x5_usb_init,
    .active = qsc60x5_usb_active,
    .drain = qsc60x5_usb_drain,
    .read = qsc60x5_usb_read,
    .write = qsc60x5_usb_write,
    .bitsize = 8
};