/*
Commands for DRL (Dark Reunion Loader),
*/
#include "dcc/dn_dcc_proto.h"
#include "io/io_type.h"
#include "devices.h"
#include <stdint.h>

typedef DCC_RETURN DCC_INIT_PTR(DCCMemory *mem, uint32_t offset);
typedef DCC_RETURN DCC_READ_PTR(DCCMemory *mem, uint32_t offset, uint32_t size, uint8_t *dest, uint32_t *dest_size);

#ifdef DCC_TESTING
extern uint32_t DCC_COMPRESS_MEMCPY(uint32_t algo, uint32_t src_offset, uint32_t size, uint8_t *dest);
void *absolute_to_relative(void* ptr) { return ptr; };
#else
extern void *absolute_to_relative(void *ptr);
#endif

extern uint32_t BUF_INIT[];
extern uint32_t dcc_init_offset;
extern uint8_t need_hello;
extern DCCMemory mem[];
extern uint8_t mem_has_spare[];

static uint8_t compBuf[DCC_BUFFER_SIZE + 0x2000];
static uint8_t rawBuf[DCC_BUFFER_SIZE + 0x2000];

size_t strlen(const char *str);

int DRL_Cmd_Handshake(incoming_t i)
{
    /* The reason we put two headers (both QCOM and DRL) is for the tool
       to spit out what it should on which is which.
       If Qualcomm FDP header found -> TX that it isn't
       If DRL header found -> TX confirmation and open the gates
    */
    //RX
    static const uint8_t qcom_rx_hdr[] = "QCOM fast download protocol host";
    static const uint8_t drl_rx_hdr[]  = "KAIDOU_SHUN!";
    //TX
    static const uint8_t qcom_tx_hdr[] = "Sorry, I'm not a QCOM FDP loader";
    static const uint8_t drl_tx_hdr[]  = "CHUUNIBYOU!!";

    if (memcmp (i->buffer + 1, qcom_rx_hdr, 32) != 0)
    {
        reply_buffer[0] = 0x2;
        memcpy(reply_buffer+1,qcom_tx_hdr,32);
        reply_length = 33;
        DRL_Packet_CalcCRC16();
        DRL_Packet_TX();
        return 0;
    }

    if (memcmp (i->buffer + 1, drl_rx_hdr, 12))
    {
        reply_buffer[0] = DRL_CMD_HANDSHAKE_W;
        memcpy(reply_buffer+1,drl_tx_hdr,12);
        need_hello = 0;
        reply_length = 13;
        memcpy(reply_buffer+13,BUF_INIT,dcc_init_offset<<2);
        reply_length += dcc_init_offset<<2;
        DRL_Packet_CalcCRC16();
        DRL_Packet_TX();       
        return 0;
    }   

    DRL_Packet_SendErrorCodeText(DRL_ERR_INVALID, DRL_MSG_INV);
    return 0;
}

int DRL_Cmd_Info(incoming_t t)
{
    reply_buffer[0] = DRL_CMD_INFO_W;
    memcpy(reply_buffer+1,BUF_INIT,dcc_init_offset<<2);
    reply_length = 1+(dcc_init_offset<<2);
    DRL_Packet_CalcCRC16();
    DRL_Packet_TX();    
    return 0;    
}

int DRL_Cmd_FlashSize(incoming_t i)
{
    unsigned char flash_id;
    reply_buffer[0] = DRL_CMD_FLASH_SIZE_W;

    flash_id = i->buffer[1];
    if (flash_id == 0) {
        //no sz
        reply_buffer[1] = 0x0;
        reply_buffer[2] = 0x0;
        reply_buffer[3] = 0x0;
        reply_buffer[4] = 0x0;
    } else if (flash_id < 0x11 && mem[flash_id - 1].type != MEMTYPE_NONE) {
        uint32_t fs = mem[flash_id-1].size;
        reply_buffer[1] = (fs)&0xff;
        reply_buffer[2] = (fs>>8)&0xff;
        reply_buffer[3] = (fs>>16)&0xff;
        reply_buffer[4] = (fs>>24)&0xff;
    } else {
        //deadbeef
        reply_buffer[1] = 0xef;
        reply_buffer[2] = 0xbe;
        reply_buffer[3] = 0xad;
        reply_buffer[4] = 0xde;
    }
    reply_length = 5;
    DRL_Packet_CalcCRC16();
    DRL_Packet_TX();    
    return 0;     
}

int DRL_Packet_FlashRead(incoming_t i)
{
    uint32_t src_offset, src_size;
    uint8_t flash_id;
    uint8_t algo;
    uint32_t out_sz;
    uint32_t dest_size;
    Driver *dev_base;
    DCC_RETURN res;
    
    flash_id = i->buffer[1];
    algo = i->buffer[2];
    src_offset = i->buffer[3] | (i->buffer[4]<<8) | (i->buffer[5]<<16) | (i->buffer[6]<<24);
    src_size = i->buffer[7] | (i->buffer[8]<<8);

    if (src_size > DCC_BUFFER_SIZE) {
        DRL_Packet_SendErrorCodeText(DRL_ERR_LEN, DRL_MSG_LEN);
        return 0;
    }

    if (flash_id == 0) {
    Jump_Read_NOR:
#ifndef DCC_TESTING
        switch (algo) {
            case CMD_READ_COMP_NONE:
                out_sz = DN_Packet_CompressNone((uint8_t *)src_offset, src_size, compBuf);
                break;

            case CMD_READ_COMP_RLE:
                out_sz = DN_Packet_Compress((uint8_t *)src_offset, src_size, compBuf);
                break;

            #if HAVE_MINILZO
            case CMD_READ_COMP_LZO:
                out_sz = DN_Packet_Compress2((uint8_t *)src_offset, src_size, compBuf);
                break;
            #endif

            #if HAVE_LZ4
            case CMD_READ_COMP_LZ4:
                out_sz = DN_Packet_Compress3((uint8_t *)src_offset, src_size, compBuf);
                break;
            #endif

            default:
                DRL_Packet_SendErrorCodeText(DRL_ERR_INVALID, DRL_MSG_INV);
                return 0;
        }
#else
        out_sz = DCC_COMPRESS_MEMCPY(algo, src_offset, src_size, compBuf);
#endif
        
        reply_buffer[0] = DRL_CMD_FLASH_READ_W;
        reply_buffer[1] = (src_offset)&0xff;
        reply_buffer[2] = (src_offset>>8)&0xff;
        reply_buffer[3] = (src_offset>>16)&0xff;
        reply_buffer[4] = (src_offset>>24)&0xff;
        reply_buffer[5] = (out_sz)&0xff;
        reply_buffer[6] = (out_sz>>8)&0xff;
        memcpy(reply_buffer+7, compBuf, out_sz);
        reply_length = 7+out_sz;
        DRL_Packet_CalcCRC16();
        DRL_Packet_TX();            
        return 0;
    } else if (flash_id < 0x11 && mem[flash_id - 1].type != MEMTYPE_NONE) {
        switch (mem[flash_id - 1].type) {
            case MEMTYPE_NAND:
            case MEMTYPE_ONENAND:
            case MEMTYPE_SUPERAND:
            case MEMTYPE_AND:
            case MEMTYPE_AG_AND:
                dev_base = (Driver *)absolute_to_relative(devices[flash_id - 1].driver);
                res = ((DCC_READ_PTR *)absolute_to_relative(dev_base->read))(&mem[flash_id - 1], src_offset, src_size, rawBuf, &dest_size);
                if (res != DCC_OK) {
                    DRL_Packet_SendErrorCodeText(DRL_ERR_FAILED, DRL_MSG_FAILED);
                    return 0;
                }
                
                switch (algo) {
                    case CMD_READ_COMP_NONE:
                        out_sz = DN_Packet_CompressNone(rawBuf, dest_size, compBuf);
                        break;

                    case CMD_READ_COMP_RLE:
                        out_sz = DN_Packet_Compress(rawBuf, dest_size, compBuf);
                        break;

                    #if HAVE_MINILZO
                    case CMD_READ_COMP_LZO:
                        out_sz = DN_Packet_Compress2(rawBuf, dest_size, compBuf);
                        break;
                    #endif

                    #if HAVE_LZ4
                    case CMD_READ_COMP_LZ4:
                        out_sz = DN_Packet_Compress3(rawBuf, dest_size, compBuf);
                        break;
                    #endif

                    default:
                        DRL_Packet_SendErrorCodeText(DRL_ERR_INVALID, DRL_MSG_INV);
                        return 0;
                }
                reply_buffer[0] = DRL_CMD_FLASH_READ_W;
                reply_buffer[1] = (src_offset)&0xff;
                reply_buffer[2] = (src_offset>>8)&0xff;
                reply_buffer[3] = (src_offset>>16)&0xff;
                reply_buffer[4] = (src_offset>>24)&0xff;
                reply_buffer[5] = (out_sz)&0xff;
                reply_buffer[6] = (out_sz>>8)&0xff;
                memcpy(reply_buffer+7, compBuf, out_sz);
                reply_length = 7+out_sz;
                DRL_Packet_CalcCRC16();
                DRL_Packet_TX(); 
                return 0;
            case MEMTYPE_NOR:
            default:
                src_offset += mem[flash_id - 1].base_offset;
                goto Jump_Read_NOR;
        }
    } else {
        reply_buffer[0] = DRL_CMD_FLASH_READ_W;
        reply_buffer[1] = 0xef;
        reply_buffer[2] = 0xbe;
        reply_buffer[3] = 0xad;
        reply_buffer[4] = 0xde;
        reply_length = 5;
        DRL_Packet_CalcCRC16();
        DRL_Packet_TX(); 
        return 0;
    }

    return 0;
}

int DRL_Packet_FlashWrite(incoming_t i)
{
    uint32_t src_offset, src_size;
    uint8_t flash_id;
    uint8_t algo;
    uint32_t out_sz;
    uint32_t dest_size;
    Driver *dev_base;
    DCC_RETURN res;
    
    flash_id = i->buffer[1];
    algo = i->buffer[2];
    src_offset = i->buffer[3] | (i->buffer[4]<<8) | (i->buffer[5]<<16) | (i->buffer[6]<<24);
    src_size = i->buffer[7] | (i->buffer[8]<<8);
 
}

int (*DRL_Packet_Cmds[]) (incoming_t) =
{
    DRL_Packet_SendInvalid,
    DRL_Cmd_Handshake,
    DRL_Cmd_Info,
    DRL_Cmd_FlashSize,
    DRL_Packet_SendInvalid, //currently NO erase
    DRL_Packet_FlashRead,
    DRL_Packet_SendInvalid //currently NO write
};