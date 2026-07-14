/*
CFI-related code for reads
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

void DRL_NOR_CFI_Read(uint32_t base_address, uint16_t* data, uint32_t len, bool intel)
{
    int i;

    //Intel breaks the rule unlike most common vendors here
    //CFI CMD is written at 0 and not 0x555.
    //Reset CMD is 0xff and not 0xf0.
    
    NOR_WRITE(base_address, (intel==true)?0:0x555, 0x98);
    //read
    for(i=0;i<len;i++)
        data[i] = NOR_READ(base_address, 0x10+i);
    //reset
    NOR_WRITE(base_address, 0, (intel==true)?0xff:0xf0);
}