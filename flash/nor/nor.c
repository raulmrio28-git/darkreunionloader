// CFI compliant NOR Memory interface

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

typedef drl_nor_info_t* (*nor_device_probe)(uint32_t);

typedef struct {
    nor_device_probe p;
    bool is_cfi;
    bool cfi_intel_way; //See nor_cfi.c as to why
} drl_nor_devices_t;

extern drl_nor_info_t* DRL_NOR_Probe_Intel_Numonyx(uint32_t base_address);
extern drl_nor_info_t* DRL_NOR_Probe_AMD_Spansion(uint32_t base_address);
extern drl_nor_info_t* DRL_NOR_Probe_Samsung(uint32_t base_address);
extern drl_nor_info_t* DRL_NOR_Probe_Toshiba(uint32_t base_address);
extern drl_nor_info_t* DRL_NOR_Probe_Mitsubishi_Renesas(uint32_t base_address);


size_t strlen(const char *str);

typedef struct {
    uint8_t bit_width;
    uint32_t block_size;
    uint32_t size;
} CFIQuery;

//for debugs!
uint16_t nor_read_id_codes[4];
uint16_t cfi_data[0xf0];

drl_nor_devices_t nor_devices_supported[]= {
    /* Probe table for NOR flash devices usable by flash driver clients 
    * Do Intel probe first as the Samsung/Spansion probe command sequence 
    * would put the Intel part in Read Device Info mode.
    */
    {DRL_NOR_Probe_Intel_Numonyx, true, true},
    {DRL_NOR_Probe_Mitsubishi_Renesas, false, false},
    {DRL_NOR_Probe_Samsung, true, false},
    {DRL_NOR_Probe_AMD_Spansion, true, false},
    {DRL_NOR_Probe_Toshiba, true, false},
    {(drl_nor_info_t * (*)(uint32_t))0xDEADBEEF, false} //stop flag
};

int nor_devices_supported_cnt = (sizeof(nor_devices_supported)/sizeof(drl_nor_devices_t))-1;

DCC_RETURN DRL_NOR_Probe(DCCMemory *mem, uint32_t offset) {
    int i;
    bool support_cfi = false;
    bool cfi_uses_intel_way = false;
    drl_nor_devices_t* curr_d;
    drl_nor_info_t* curr_d_info = NULL;

    curr_d = nor_devices_supported;
    for(i=0;i<nor_devices_supported_cnt&&((uint32_t)(curr_d->p)!=0xdeadbeef);i++)
    {
        curr_d_info = curr_d->p(offset);
        if (curr_d_info)
        {
            support_cfi = curr_d->is_cfi;
            cfi_uses_intel_way = curr_d->cfi_intel_way;
            break;
        }
        else
            curr_d++;
    }

    if (!curr_d_info) //mystery...
    {
invalid_flash:
        //put a placeholder value for a good reason and hope for the best?
        //if flash is unknown then submit a pull request or issue 
        PLAT_SNPRINTF(mem->name, 255, "UNKNOWN!!! (%04x, %04x)", nor_read_id_codes[0], nor_read_id_codes[1]);
        mem->bit_width = 16;
        mem->size = 0x02000000;
        mem->page_size = 0x200;
        mem->block_size = 0x10000;
        mem->type = MEMTYPE_NOR;
        mem->nor_cmd_set = 3;
        mem->base_offset = offset;
        mem->manufacturer = nor_read_id_codes[0];
        mem->device_id = nor_read_id_codes[1];
        return DCC_OK;
    }
    else
    {
        PLAT_MEMCPY(mem->name, curr_d_info->model, strlen(curr_d_info->model)+1);
        if (support_cfi)
        {
            DRL_NOR_CFI_Read(offset, cfi_data, 0xf0, cfi_uses_intel_way);
            if (cfi_data[0] != 0x51 || cfi_data[1] != 0x52 || cfi_data[3] != 0x59)
                goto invalid_flash;
            mem->bit_width = 16;
            mem->size = 1 << cfi_data[0x17];
            mem->block_size = 0;

            /* Compute block size via Erase region */
            uint16_t qry_erase_size = NOR_READ(offset, 0x2c);
            for (uint16_t i = 0; i < qry_erase_size; i++) {
                uint16_t z = cfi_data[0x1f + (4 * i)] | (cfi_data[0x20 + (4 * i)]<<8);
                if (mem->block_size < (z << 8)) {
                    mem->block_size = (z << 8);
                }
            }
            mem->type = MEMTYPE_NOR;
            mem->nor_cmd_set = cfi_data[0x3];
            mem->base_offset = offset;
            mem->manufacturer = nor_read_id_codes[0];
            mem->device_id = nor_read_id_codes[1];
        }
        else
        {
            mem->bit_width = 16;
            mem->size = 0x02000000;
            mem->page_size = 0x200;
            mem->block_size = 0x10000;
            mem->type = MEMTYPE_NOR;
            mem->nor_cmd_set = 3;
            mem->base_offset = offset;
            mem->manufacturer = nor_read_id_codes[0];
            mem->device_id = nor_read_id_codes[1];
            if (mem->manufacturer == 0x1c) { 
                switch (mem->device_id >> 4) {
                    case 0x7: // Found in PNC DM-P100
                    case 0xf: // Found in Sanyo RL-4920
                        mem->size = 0x02000000;
                        break;

                    case 0xc: // Found in Sanyo SCP-3100
                        mem->size = 0x01000000;
                        break;

                    case 0x3: // Found in Sanyo PM-8200 (Overlaps with 4MB M5M29KB331ATP)
                    case 0xb: // Found in Audiovox CDM-8910
                        mem->size = 0x00800000;
                        break;

                    case 0x2: // Found in SCP-5150
                        mem->size = 0x00400000;
                        break;

                    case 0xa: // Found in SCP-5150
                    case 0x6: // M6MF16S2AVP datasheet
                        mem->size = 0x00200000;
                        break;

                    case 0x5: // M5M29FBT800FP datasheet
                        mem->size = 0x00100000;
                        break;
                }
            }
        }
    }
    return DCC_OK;
}

Driver nor_controller = {
    .initialize = DRL_NOR_Probe,
    .read = NULL
};