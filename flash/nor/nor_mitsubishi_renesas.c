/*
Mitsubishi/Renesas-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

drl_nor_info_t M5M29KD157AKT =
{
  "RENESAS M5M29KD157AKT",
  2,
  {0x001c, 0x00c8}
};

drl_nor_info_t R11M4400LRABBB =
{
  "RENESAS R11M4400LRABBB",
  2,
  {0x001c, 0x007d}
};

drl_nor_info_t M6MGT166S4BWG =
{
  "RENESAS M6MGT166S4BWG",
  2,
  {0x001c, 0x00a0}
};

drl_nor_info_t M6MGB166S4BWG =
{
  "RENESAS M6MGB166S4BWG",
  2,
  {0x001c, 0x00a1}
};

drl_nor_info_t M6MGD13TW66CWG =
{
  "RENESAS M6MGB166S4BWG",
  2,
  {0x001c, 0x00b8}
};

const drl_nor_info_t *(renesas_parts[]) =
{
  &M5M29KD157AKT,
  &R11M4400LRABBB,
  &M6MGT166S4BWG,
  &M6MGB166S4BWG,
  &M6MGD13TW66CWG,
  NULL   
};

//I suspect this code read stuff is same as Intel's...

void DRL_NOR_Probe_Mitsubishi_Renesas_Codes(uint32_t base_address, uint16_t* codes)
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

drl_nor_info_t* DRL_NOR_Probe_Mitsubishi_Renesas(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_Mitsubishi_Renesas_Codes(base_address, nor_read_id_codes);
  for (dev = renesas_parts; *dev != NULL; dev++)
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
