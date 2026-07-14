/*
I/O Type struct
*/
#include "dcc/dn_dcc_proto.h"

typedef enum {
    USBDC_GET_LINE_CODING = 0x102,
    USBDC_SET_CONTROL_LINE_STATE_I = 0x2221,
    USBDC_SET_LINE_CODING = 0x2021,
    USBDC_SETUP_CLEAR_FEATURE_E = 0x21A1
} usb_cdc_csr_e;

#pragma pack(1)
typedef struct {
    usb_cdc_csr_e bmRequest_and_Type:16;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
}usbdc_setup_type;

typedef struct {
    //Initializer of I/O
    bool (*init) (void);
    //Check if I/O is active
    bool (*active) (void);
    //Drain remaining I/O stuff
    void (*drain) (void);
    //Read to packet I/O area
    void (*read) (void);
    //Write to host
    void (*write) (uint32_t);
    //Bitsize of write/read ops
    int bitsize;
} drl_io_funcs_t;

/* Packet I/O */

#define CHECK_FOR_DATA() DRL_Current_IO->read()
#define TRANSMIT_BYTE(v) DRL_Current_IO->write((v))

/* Maximum data length. */
#define MAX_DATA_LENGTH 1024

/* Number of packets.  NUMBER_OF_PACKETS * MAX_DATA_LENGTH will be our
   maximum window size. */
#define NUMBER_OF_PACKETS 100


/* ----------------------------------------------------------------------
 * Defines used to calculate size of reply buffer to QPST and also
 * number of Flash part sectors that will fit into that reply buffer.
 * Code in packet layer will cause a fatal error if the size of the
 * buffer is exceeded at run time.  Code in NOR flash layer will cause
 * a fatal error at compile time if number of sectors is exceeded.
  ----------------------------------------------------------------------*/

#define HOST_REPLY_BUFFER_SIZE  2048

/* Fixed size elements of Parameter Request Reply packet */
#define PACKET_OVERHEAD_SIZE    7
#define CMD_SIZE                1
#define MAGIC_SIZE              32
#define VERSION_SIZE            1
#define COMPAT_VERSION_SIZE     1
#define BLOCK_SIZE_SIZE         4
#define FLASH_BASE_SIZE         4
#define FLASH_ID_LEN_SIZE       1
#define WINDOW_SIZE_SIZE        2
#define NUM_SECTORS_SIZE        2
#define FEATURE_BITS_SIZE       4

/* Variable size element of Parameter Request Reply packet.  The length 
 * of the Flash ID string is indeterminate, but currently the largest is
 * 16 bytes, so allow double this size for growth. */
#define FLASH_ID_STRING_SIZE    32


/* Add up all the parts except sectors */
#define REPLY_FIXED_SIZE (PACKET_OVERHEAD_SIZE+CMD_SIZE+MAGIC_SIZE+ \
  VERSION_SIZE+COMPAT_VERSION_SIZE+BLOCK_SIZE_SIZE+FLASH_BASE_SIZE+ \
  FLASH_ID_LEN_SIZE+WINDOW_SIZE_SIZE+NUM_SECTORS_SIZE+ \
  FEATURE_BITS_SIZE+FLASH_ID_STRING_SIZE)


#define REPLY_BUFFER_SIZE   HOST_REPLY_BUFFER_SIZE

/* Calculate amount of 4 byte sector sizes which fit in remaining
 * portion of parameter request reply
 */
#define MAX_SECTORS      ((REPLY_BUFFER_SIZE - REPLY_FIXED_SIZE) / 4)

/* Maximum packet size.  1 for packet type.  4 for length.  2 for CRC. */
#define MAX_PACKET_SIZE (MAX_DATA_LENGTH + 1 + 4 + 2)

struct incoming_data
{
  struct incoming_data *next;   /* Chain appropriately. */
  uint16_t length;              /* Number of valid bytes in buffer. */
  uint8_t buffer[MAX_PACKET_SIZE];
};

typedef struct incoming_data *incoming_t;


/* Command bytes below */

typedef enum {
    /* RXed CMDs */
    DRL_CMD_STARTING_CMD = 0x0,
    DRL_CMD_HANDSHAKE = 0x01,
    DRL_CMD_INFO = 0x02,
    DRL_CMD_FLASH_SIZE = 0x03,
    DRL_CMD_FLASH_ERASE = 0x04,
    DRL_CMD_FLASH_READ = 0x05,
    DRL_CMD_FLASH_WRITE = 0x06,
    //this will update with any addition
    DRL_CMD_ENDING_CMD = DRL_CMD_FLASH_WRITE,
    /* TXed CMDs, They're the same as RXed but append bit 7 */
    DRL_CMD_HANDSHAKE_W = 0x81,
    DRL_CMD_INFO_W = 0x82,
    DRL_CMD_FLASH_SIZE_W = 0x83,
    DRL_CMD_FLASH_ERASE_W = 0x84,
    DRL_CMD_FLASH_READ_W = 0x85,
    DRL_CMD_FLASH_WRITE_W = 0x86,
    /* Error commands */
    DRL_CMD_ERROR_STRING_W = 0xF1,
    DRL_CMD_ERROR_CODE_STRING_W = 0xF2  
} drl_cmd_byte_e;

/* Error msgs and codes below */

#define DRL_MSG_NONE "!OK!"
#define DRL_MSG_INV "!INV"
#define DRL_MSG_LEN "!LEN"
#define DRL_MSG_ADDR "!ADR"
#define DRL_MSG_FAILED_ID "!FLA"
#define DRL_MSG_SUCCEED "!SUC"
#define DRL_MSG_FAILED "!ERR"
#define DRL_MSG_WIN_OVERRUN "!WOV"
#define DRL_MSG_CHECKSUM "!CHK"

typedef enum {
    /* Generic errors */
    DRL_ERR_NONE = 0x0,
    DRL_ERR_INVALID = 0x1,
    DRL_ERR_LEN = 0x2,
    DRL_ERR_ADDR = 0x3,

    /* Op errors */
    DRL_ERR_FAILED_ID = 0x10,
    DRL_ERR_SUCCEED = 0x11,
    DRL_ERR_FAILED = 0x12,

} drl_error_word_e;

extern drl_io_funcs_t* DRL_Current_IO;

void DRL_Packet_RX(uint8_t ch);
void DRL_Packet_TX();
int DRL_Packet_SendInvalid(incoming_t i);

void DRL_Packet_SendErrorText(const char* s);
void DRL_Packet_SendErrorCodeText(uint16_t e, const char* s);


void DRL_Packet_CalcCRC16();

void DRL_Packet_Proc();

extern uint8_t reply_buffer[];
extern uint16_t reply_length;