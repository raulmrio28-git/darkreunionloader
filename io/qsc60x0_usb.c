#include "dcc/plat.h"
#include "io/io_type.h"

/*
The following is code for USB I/O, reverse engineered from a
QSC60x0-based device's ARMPRG.

For understanding the registers, some help from a Qualcomm
Software interface document was needed.
*/

#pragma pack(1)
typedef struct
{
  uint32_t USB_SETUP_ADDR;
  uint32_t USB_SETUP_WORD;
  uint32_t USB_CORE_STATUS;
  uint32_t USBRAM_BUFFER_REG;
  uint32_t USB_INT_CLEAR;
  uint32_t USB_INT_STATUS;
  uint32_t USB_INT_MASK_WR;
  uint32_t USB_INT_SOF_MASK;
  uint32_t USB_INT_SOF_COUNT;
  uint32_t USB_HDR_INT_CLR;
  uint32_t USB_CONTROL_CMD;
  uint32_t USB_IN_FIFO_CMD;
  uint32_t USB_OUT_FIFO_CMD;
  uint32_t USB_HDR_FL_CMD;
  uint32_t USB_HDR_RL_CMD;
  uint32_t USB_HDR_FL_XFER;
  uint32_t USB_HDR_FL_CONFIG;
  uint32_t USB_HDR_RLF_FILL_STATUS;
  uint32_t USB_HDR_FLF_FILL_STATUS;
  uint32_t USB_IN_FIFO_STATUS;
  uint32_t USB_OUT_FIFO_5_STATUS;
  uint32_t USB_OUT_FIFO_6_STATUS;
  uint32_t USB_OUT_FIFO_8_STATUS;
} qsc60x0_usb_reg_t;

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
} qsc60x0_usb_serial_t;

typedef struct {
  uint8_t fifo_has_data;
  uint8_t fifo_is_empty;
  uint32_t wr_len;
} qsc60x0_usb_state_t;


static qsc60x0_usb_reg_t USB_Registers;
static qsc60x0_usb_serial_t USB_Serial;
static qsc60x0_usb_state_t USB_State;

static uint32_t USB_fifo3_banks[] = {0x62000240,0x62000280};
static uint32_t USB_fifo6_banks[] = {0x620002C0,0x62000300};

static uint32_t USB_fifo2_banks[] = {0x62000040,0x62000080,0x620000C0,0x62000100};
static uint32_t USB_fifo5_banks[] = {0x62000140,0x62000180,0x620001C0,0x62000200};

bool USB_packet_process = false;
bool USB_packet_started = false;

uint8_t USB_rx_buffer[0x100];
uint8_t USB_tx_buffer[0x100];

static void qsc60x0_init_usb_reg_table()
{
  USB_Registers.USB_SETUP_ADDR = 0x6200041C;
  USB_Registers.USB_SETUP_WORD = 0x62000418;
  USB_Registers.USB_CORE_STATUS = 0x62000410;
  USB_Registers.USBRAM_BUFFER_REG = 0x62000500;
  USB_Registers.USB_INT_CLEAR = 0x62000404;
  USB_Registers.USB_INT_STATUS = 0x62000404;
  USB_Registers.USB_INT_MASK_WR = 0x62000400;
  USB_Registers.USB_INT_SOF_MASK = 0x62000408;
  USB_Registers.USB_INT_SOF_COUNT = 0x6200040C;
  USB_Registers.USB_HDR_INT_CLR = 0x62000444;
  USB_Registers.USB_CONTROL_CMD = 0x62000420;
  USB_Registers.USB_IN_FIFO_CMD = 0x62000424;
  USB_Registers.USB_OUT_FIFO_CMD = 0x62000428;
  USB_Registers.USB_HDR_FL_CMD = 0x62000450;
  USB_Registers.USB_HDR_RL_CMD = 0x6200045C;
  USB_Registers.USB_HDR_FL_XFER = 0x62000448;
  USB_Registers.USB_HDR_FL_CONFIG = 0x6200044C;
  USB_Registers.USB_HDR_RLF_FILL_STATUS = 0x62000448;
  USB_Registers.USB_HDR_FLF_FILL_STATUS = 0x62000444;
  USB_Registers.USB_IN_FIFO_STATUS = 0x6200041C;
  USB_Registers.USB_OUT_FIFO_5_STATUS = 0x62000420;
  USB_Registers.USB_OUT_FIFO_6_STATUS = 0x62000424;
  USB_Registers.USB_OUT_FIFO_8_STATUS = 0x62000428;
}

void qsc60x0_read_usb_fifo(uint32_t r, uint8_t *d, uint32_t len, uint16_t fifo_state)
{
    uint32_t len_d = (len + 3) >> 2;
    wdog_reset();
    switch ((uintptr_t)d & 3)
    {
      case 0:
          for (uint32_t i = 0; i < len_d; i++)
              ((uint32_t *)d)[i] = READ_U32(r+(i<<2));
          break;
      case 1: 
      case 3:
          for (uint32_t i = 0; i < len_d; i++)
          {
              uint32_t v = READ_U32(r+(i<<2));
              *d++ = (uint8_t)v;  v >>= 8;
              *d++ = (uint8_t)v;  v >>= 8;
              *d++ = (uint8_t)v;  v >>= 8;
              *d++ = (uint8_t)v;
          }
          break;

      case 2:
          for (uint32_t i = 0; i < len_d; i++)
          {
              uint32_t v = READ_U32(r+(i<<2));
              *(uint16_t *)d = (uint16_t)v;
              *(uint16_t *)d = (uint16_t)(v >> 16);
          }
          break;
    }

    WRITE_U16(USB_Registers.USB_OUT_FIFO_CMD, fifo_state);
}

void qsc60x0_write_usb_fifo(uint32_t r, uint8_t *s, uint32_t len, uint16_t fifo_state)
{
    uint32_t len_d = (len + 3) >> 2;
    wdog_reset();
    switch ((uintptr_t)s & 3)
    {
      case 0:
          for (uint32_t i = 0; i < len_d; i++)
              WRITE_U32(r+(i<<2), ((uint32_t *)s)[i]);
          break;
      case 1: 
      case 3:
          for (uint32_t i = 0; i < len_d; i++)
          {
            uint32_t v = *s++;
            v += (uint32_t)*s++<<8;
            v += (uint32_t)*s++<<16;
            v += (uint32_t)*s++<<24;
            WRITE_U32(r+(i<<2), v);
          }
          break;

      case 2:
          for (uint32_t i = 0; i < len_d; i++)
          {
            uint32_t v = *(uint16_t *)s; s += 2;
            v += (uint32_t)(*(uint16_t *)s) << 16; s += 2;
			      WRITE_U32(r+(i<<2), v);
          }
          break;
    }

    WRITE_U16(USB_Registers.USB_IN_FIFO_CMD, (len<<8)|fifo_state);
}

void qsc60x0_usb_handle_pkts(uint32_t a, uint32_t len, uint16_t fifo_state)
{
  uint8_t r;
  int i;

  wdog_reset();
  for (i=0;i<len;i++)
  {
    r = READ_U8(a+i);
    DRL_Packet_RX(r);
  }
  WRITE_U16(USB_Registers.USB_OUT_FIFO_CMD, fifo_state);
}

int qsc60x0_usb_get_bank(int fifo)
{
  int setup_addr_register;
  setup_addr_register = READ_U8(USB_Registers.USB_IN_FIFO_STATUS);
  if ( fifo == 2 )
    return (setup_addr_register & 0xC) >> 2; //CURRENT_BANK_2
  if ( fifo == 3 )
    return (setup_addr_register & 0x20) >> 5; //CURRENT_BANK_3
  return -1;
}

uint16_t qsc60x0_usb_get_fill_status()
{
  uint16_t rlf_status;

  rlf_status = READ_U16(USB_Registers.USB_HDR_RLF_FILL_STATUS);

  if (((rlf_status&0x3ff)==0) 
    && (rlf_status&0x400))
    return 0x400; //FIFO full!
  else
    return rlf_status&0x3ff; //Nope
}

void qsc60x0_usb_setup_update()
{
  if (READ_U16(USB_Registers.USB_INT_STATUS)&0x100) //setup packet r/w
  {
    uint16_t usb_setup_word;
    uint16_t usb_setup_len;
    int i;
    //read every setup byte 
    WRITE_U16(USB_Registers.USB_SETUP_ADDR, 0);
    usb_setup_word = READ_U16(USB_Registers.USB_SETUP_WORD);
    WRITE_U16(USB_Registers.USB_SETUP_ADDR, 2);
    WRITE_U16(USB_Registers.USB_SETUP_ADDR, 4);
    WRITE_U16(USB_Registers.USB_SETUP_ADDR, 6);
    usb_setup_len = READ_U16(USB_Registers.USB_SETUP_WORD);
    wdog_reset();
    if (usb_setup_word == 0x2021) //USB serial info read
    {
      qsc60x0_read_usb_fifo(0x62000350, (uint8_t*)&USB_Serial, READ_U16(USB_Registers.USB_OUT_FIFO_8_STATUS)&0x1f, 5);
    }
    else if (usb_setup_word == 0x21a1) //USB serial info write
    {
      qsc60x0_write_usb_fifo(0x62000340, (uint8_t*)&USB_Serial, usb_setup_len, 2);
      while(!(READ_U16(USB_Registers.USB_INT_STATUS)&1)) //we need to wait till fifo0 has data
      {
        for(i=0;i<1000;i++)
          ;
      }
    }
    else if (usb_setup_word != 0x2221 && (READ_U16(USB_Registers.USB_CORE_STATUS)&0x40)) //fifo8 has data
    {
      WRITE_U16(USB_Registers.USB_OUT_FIFO_CMD, 4); //flush fifo8
    }
    WRITE_U16(USB_Registers.USB_CONTROL_CMD, 0); //ctrl transfer succeed!
  }
}

/* Stars of the show */

bool qsc60x0_usb_init()
{
  qsc60x0_init_usb_reg_table();

  if (READ_U16(0x62000408)&0x1) //USB_EP_STATUS has 1 endpoint present
		return false;
	
	WRITE_U16(USB_Registers.USB_INT_MASK_WR, 0); //set every uint32_terrupt to 0
	WRITE_U16(USB_Registers.USB_INT_SOF_MASK, 0); //set every FIFO mask to 0
	WRITE_U16(USB_Registers.USB_INT_SOF_COUNT, 0); //set every FIFO throttle to 0
	
	memset(&USB_State,0,sizeof(qsc60x0_usb_state_t));
	//everything is false or 0
	memset(&USB_Serial, 0, sizeof(qsc60x0_usb_serial_t));
	
	//what's needed is baud and databits
	USB_Serial.baud = 115200;
	USB_Serial.databits = 8;
	
	//Reset FIFO 2 and 3
	WRITE_U16(USB_Registers.USB_IN_FIFO_CMD, 6);
	WRITE_U16(USB_Registers.USB_IN_FIFO_CMD, 9);
	
	//Flush FIFO 5 and 6
	WRITE_U16(USB_Registers.USB_OUT_FIFO_CMD, 0);
	WRITE_U16(USB_Registers.USB_OUT_FIFO_CMD, 2);
	
	return true;
}

bool qsc60x0_usb_active()
{
  bool ret = false;
  //FIFO5 has data state
  if (READ_U8(USB_Registers.USB_INT_STATUS) & (1<<4))
  {
    USB_State.fifo_has_data = 5;
    USB_State.fifo_is_empty = 2;
    ret = true;
    goto usb_active_finish;
  }
  //FIFO6 has data state
  if (READ_U8(USB_Registers.USB_INT_STATUS) & (1<<5))
  {
    USB_State.fifo_has_data = 6;
    USB_State.fifo_is_empty = 3;
    ret = true;
    goto usb_active_finish;
  }
  if (qsc60x0_usb_get_fill_status())
  {
    USB_State.fifo_has_data = 0xb; //invalid FIFO, used as a flag
    USB_State.fifo_is_empty = 0xa; //ditto    
    ret = true;
    goto usb_active_finish;
  }
usb_active_finish:
  qsc60x0_usb_setup_update();
  return ret;
}

void qsc60x0_usb_drain()
{
  bool fifo_invalid = false;
  uint8_t bank_mask = 0;

  switch (USB_State.fifo_is_empty)
  {
    case 2:
      bank_mask = 0x10;
      break;
    case 3:
      bank_mask = 0x40;
      break;
    case 0xa:
      fifo_invalid = true;
      break;
    default:
      break;
  }
  if (fifo_invalid == true)
  {
    while (READ_U16(USB_Registers.USB_HDR_FLF_FILL_STATUS)&0x3ff)
      wdog_reset();
  }
  else
  {
    while ((READ_U8(USB_Registers.USB_IN_FIFO_STATUS)&bank_mask) == 0)
      wdog_reset();
  }
}

int qsc60x0_usb_read_remaining()
{
  bool pr = false;
  uint16_t rl = 0;
  int i;
  while(1)
  {
    uint16_t fill = qsc60x0_usb_get_fill_status();
    if (fill == 0)
      return 0; //no fills
    if (fill>=0x40) //over limit
    {
      rl = 0x40;
      goto do_rx_crunching;
    }
    if ((READ_U16(USB_Registers.USB_HDR_FL_CONFIG)&0x800) == 0) //bit 11 of watermark not set
    {
      WRITE_U16(USB_Registers.USB_HDR_RL_CMD,4); //drain request
      while ( (READ_U16(USB_Registers.USB_HDR_FL_CONFIG)&0x400) == 0 ) //finish flushing
        ;
      rl = READ_U16(USB_Registers.USB_HDR_FL_CONFIG)&0x3ff;
      pr = true;
do_rx_crunching:
      {
        uint32_t* t = (uint32_t*)&USB_rx_buffer;
        uint32_t rl_d = (rl+3)>>2;
        for(i=0;i<rl_d;i++)
        {
          *t = READ_U32(USB_Registers.USBRAM_BUFFER_REG+(i<<2));
          t++;
        }
        wdog_reset();
      }
      if (pr == true)
      {
        if ( (READ_U16(USB_Registers.USB_HDR_FL_CONFIG) & 0x800) //bit 11 wm set but fill more
           && qsc60x0_usb_get_fill_status() == 0x400)
          WRITE_U16(USB_Registers.USB_HDR_RL_CMD, 1); //reset RL
        WRITE_U16(USB_Registers.USB_HDR_RL_CMD, 8); //drain clear
      }
      return rl;
    }
  }
}

void qsc60x0_usb_read()
{
  wdog_reset();
  if (USB_packet_process == false)
  {
    uint16_t usb_core_state;
    uint16_t fifo_status;
    int remaining;
    qsc60x0_usb_setup_update();
    usb_core_state = READ_U16(USB_Registers.USB_CORE_STATUS);
    switch(USB_State.fifo_has_data)
    {
      case 5:
        if((usb_core_state&0x10)==0) //FIFO5 already RXed
          return;
        fifo_status = READ_U16(USB_Registers.USB_OUT_FIFO_5_STATUS);
        qsc60x0_usb_handle_pkts(USB_fifo5_banks[(fifo_status>>8)&0x3], fifo_status&0x7f, 1);
        return;
      case 6:
        if((usb_core_state&0x20)==0) //FIFO6 already RXed
          return;
        fifo_status = READ_U16(USB_Registers.USB_OUT_FIFO_6_STATUS);
        qsc60x0_usb_handle_pkts(USB_fifo6_banks[(fifo_status>>8)&0x1], fifo_status&0x7f, 3);
        return;
      case 0xb: //invalid, used as a flag
        remaining = qsc60x0_usb_read_remaining();
        if (remaining)
        {
          int i;
          for(i=0;i<remaining;i++)
          {
            DRL_Packet_RX(USB_rx_buffer[i]);
          }
        }
        return;
      default:
        return;
    }
  }
}

void qsc60x0_usb_write_remaining(int len)
{
  int i;
  int len_d = (len+3)>>2;
  uint32_t* b = (uint32_t*)&USB_tx_buffer;

  WRITE_U16(USB_Registers.USB_HDR_FL_XFER, len&0xff);
  for(i=0;i<len_d;i++)
    WRITE_U32(USB_Registers.USBRAM_BUFFER_REG, *b++);
  WRITE_U16(USB_Registers.USB_HDR_FL_CMD,0);
  WRITE_U16(USB_Registers.USB_HDR_INT_CLR, 0x80); //empty FIFO
}

void qsc60x0_usb_write(uint32_t b)
{
  bool is_beginning = false;
  bool write_to_fifo = false;
  USB_tx_buffer[USB_State.wr_len++] = b;
  wdog_reset();
  if (b==0x7e) //escape
  {
    if (USB_packet_started == true)
      USB_packet_started = false;
    else
    {
      write_to_fifo = true;
      is_beginning = true;
      USB_packet_started = true;
    }
  } 
  else if (USB_State.wr_len == 0x40)
  {
    write_to_fifo = true;
  }
  if (write_to_fifo == true)
  {
    USB_State.wr_len = 0;
    switch (USB_State.fifo_is_empty)
    {
      case 2:
        while((READ_U16(USB_Registers.USB_CORE_STATUS)&4)==0) //fifo2 not ready to rx
          wdog_reset();
        qsc60x0_write_usb_fifo(USB_fifo2_banks[qsc60x0_usb_get_bank(2)],
                               USB_tx_buffer,USB_State.wr_len,(is_beginning==true)?8:7);
        return;
      case 3:
        while((READ_U16(USB_Registers.USB_CORE_STATUS)&8)==0) //fifo3 not ready to rx
          wdog_reset();
        qsc60x0_write_usb_fifo(USB_fifo3_banks[qsc60x0_usb_get_bank(3)],
                               USB_tx_buffer,USB_State.wr_len,(is_beginning==true)?0xb:0xa);
        return;
      case 0xa: //invalid, used as a flag
        qsc60x0_usb_write_remaining(USB_State.wr_len);
        return;
      default:
        break;
    }
  }
}

drl_io_funcs_t io_usb = {
    .init = qsc60x0_usb_init,
    .active = qsc60x0_usb_active,
    .drain = qsc60x0_usb_drain,
    .read = qsc60x0_usb_read,
    .write = qsc60x0_usb_write,
    .bitsize = 8
};