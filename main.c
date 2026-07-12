#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"
#include "devices.h"
//DRL - Modify for multi I/O 20260712
#include "io/io_type.h"
//DRL - Modify for multi I/O 20260712

typedef DCC_RETURN DCC_INIT_PTR(DCCMemory *mem, uint32_t offset);
typedef DCC_RETURN DCC_READ_PTR(DCCMemory *mem, uint32_t offset, uint32_t size, uint8_t *dest, uint32_t *dest_size);

#ifdef DCC_TESTING
void *absolute_to_relative(void* ptr) { return ptr; };
#else
extern void *absolute_to_relative(void *ptr);
#endif

size_t strlen(const char *str);

//DRL - move for extern, reduce size for limit 20260712
uint32_t BUF_INIT[511];
uint32_t dcc_init_offset = 0;
DCCMemory mem[16] = { 0 };
uint8_t mem_has_spare[16] = { 0 };
//DRL - move for extern, reduce size for limit 20260712

bool DRL_Flash_Detected = false;
uint32_t DN_Log2(uint32_t value)
{
  uint32_t m = 0;
  while (1) {
    if ((1 << m) >= value)
      return m;
    m++;
  }
}

// dcc code
void dcc_main(uint32_t StartAddress, uint32_t PageSize) {
    uint32_t ext_mem;
    Driver *devBase;
    DCC_RETURN res;

    for (int i = 0; i < 16; i++) {
        if (!devices[i].driver) break;
        devBase = (Driver *)absolute_to_relative(devices[i].driver);
        res = ((DCC_INIT_PTR *)absolute_to_relative(devBase->initialize))(&mem[i], devices[i].base_offset);
        if (res != DCC_OK) mem[i].type = MEMTYPE_NONE;

        switch (mem[i].type) {
            case MEMTYPE_NOR:
            case MEMTYPE_SUPERAND:
                ext_mem = DCC_MEM_EXTENDED(1, mem[i].page_size, mem[i].block_size, mem[i].size >> 20);
                mem_has_spare[i] = 0;
            WRITE_EXTMEM:
                if (strlen(mem[i].name)) ext_mem |= 0x80;

                BUF_INIT[dcc_init_offset++] = DCC_MEM_OK | (ext_mem << 16);
                BUF_INIT[dcc_init_offset++] = mem[i].manufacturer | (mem[i].device_id << 16);
                
                if (strlen(mem[i].name)) {
                    int sLen = strlen(mem[i].name);
                    uint8_t *bufCast = (uint8_t *)BUF_INIT;
                    // int sPos = 0;
                    // uint8_t *sData = (uint8_t *)(BUF_INIT + dcc_init_offset);
                    // BUF_INIT[dcc_init_offset++] = sLen;

                    // for (int j = 0; j < sLen; j++) {
                    //     BUF_INIT[dcc_init_offset++] = mem[i].name[j];
                    // }
                    
                    BUF_INIT[dcc_init_offset] = sLen;
                    INT_MEMCPY((bufCast + (dcc_init_offset << 2) + 1), mem[i].name, sLen);

                    dcc_init_offset += ALIGN4(1 + sLen) >> 2;
                }

                BUF_INIT[dcc_init_offset++] = ext_mem;
                DRL_Flash_Detected = true;
                break;

            case MEMTYPE_NAND:
                if (strlen(mem[i].name)) goto NAND_EXTMEM;
                BUF_INIT[dcc_init_offset++] = DCC_MEM_OK | (mem[i].page_size << 16);
                BUF_INIT[dcc_init_offset++] = mem[i].manufacturer | (mem[i].device_id << 16);
                mem_has_spare[i] = 1;
                DRL_Flash_Detected = true;
                break;

            case MEMTYPE_ONENAND:
            case MEMTYPE_AND:
            case MEMTYPE_AG_AND:
            NAND_EXTMEM:
                ext_mem = DCC_MEM_EXTENDED(0, mem[i].page_size, mem[i].block_size, mem[i].size >> 20);
                mem_has_spare[i] = 1;
                goto WRITE_EXTMEM;

            default:
                BUF_INIT[dcc_init_offset++] = DCC_MEM_OK | (DCC_MEM_NONE << 16);
                BUF_INIT[dcc_init_offset++] = 0;
                mem_has_spare[i] = 0;

        }
    }

    BUF_INIT[dcc_init_offset++] = DCC_MEM_OK | (DCC_MEM_BUFFER(0) << 16);
    BUF_INIT[dcc_init_offset++] = DCC_BUFFER_SIZE;

    while (1) {
        CHECK_FOR_DATA();
        wdog_reset();
        DRL_Packet_Proc();
        wdog_reset();
    }
}