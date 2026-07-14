/*
Sharp-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

drl_nor_info_t LRS1805 =
{
  "SHARP LRS1805/LRS1826A",
  2,
  {0x00b0, 0x00b0}
};

drl_nor_info_t LRS1806 =
{
  "SHARP LRS1806",
  2,
  {0x00b0, 0x00b1}
};

const drl_nor_info_t *(sharp_parts[]) =
{
  &LRS1805,
  &LRS1806,
  NULL   
};

//I suspect this code read stuff is same as Intel's...

void DRL_NOR_Probe_Sharp_Codes(uint32_t base_address, uint16_t* codes)
{
    //clear status
    NOR_WRITE(base_address, 0, 0x50);
    //reset
    NOR_WRITE(base_address, 0, 0xff);
    //id
    NOR_WRITE(base_address, 0, 0x90);
    codes[0] = NOR_READ(base_address, 0);
    codes[1] = NOR_READ(base_address, 1); 
    //reset
    NOR_WRITE(base_address, 0, 0xff);
}

drl_nor_info_t* DRL_NOR_Probe_Sharp(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_Sharp_Codes(base_address, nor_read_id_codes);
  for (dev = sharp_parts; *dev != NULL; dev++)
  {
    ids_found = 0;
    for (ids = 0; ids < (*dev)->num_ids; ids++)
    {
      if (nor_read_id_codes[ids] == (*dev)->codes[ids])
      {
        //found!
        ids_found++;
      }
    }
    if (ids_found == (*dev)->num_ids)
    {
      break; //found exact flashmem
    }
  }

  return (drl_nor_info_t *) *dev;
}
