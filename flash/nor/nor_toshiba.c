/*
Toshiba-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

drl_nor_info_t TC58FYM7T8C_TOP =
{
  "TOSHIBA TC58FYM7T8C_TOP",
  2,                            /* # of codes to match */
  {0x0098, 0x009E},
};

drl_nor_info_t TC58FYM7T8C_BOT =
{
  "TOSHIBA TC58FYM7T8C_BOT",
  2,                            /* # of codes to match */
  {0x0098, 0x009F},
};

//As used in Jivi JV-C200

drl_nor_info_t TV00560002EDGB =
{
  "TOSHIBA TV00560002EDGB",
  2,
  {0x0098, 0x0096}
};
drl_nor_info_t TV00670002CDGB =
{
  "TOSHIBA TV00670002CDGB",
  2,
  {0x0098, 0x0003}
};
drl_nor_info_t TY5701111183KC04 =
{
  "TOSHIBA TY5701111183KC04",
  2,
  {0x0098, 0x009E}
};
drl_nor_info_t TV00560002DDGB =
{
  "TOSHIBA TV00560002DDGB",
  2,
  {0x0098, 0x0049}
};


const drl_nor_info_t *(toshiba_parts[]) =
{
  &TC58FYM7T8C_TOP,
  &TC58FYM7T8C_BOT,
  &TV00560002EDGB,
  &TV00670002CDGB,
  &TY5701111183KC04,
  &TV00560002DDGB,
  NULL   
};

void DRL_NOR_Probe_Toshiba_Codes(uint32_t base_address, uint16_t* codes)
{
    //reset1
    NOR_WRITE(base_address, 0, 0xf0);
    //autoselect mode
    NOR_WRITE(base_address, 0x555, 0xaa);
    NOR_WRITE(base_address, 0x2aa, 0x55);
    //read ID
    NOR_WRITE(base_address, 0x555, 0x90);
    //get iid
    codes[0] = NOR_READ(base_address, 0x00);
    codes[1] = NOR_READ(base_address, 0x01);
    //autoselect mode
    NOR_WRITE(base_address, 0x555, 0xaa);
    NOR_WRITE(base_address, 0x2aa, 0x55);
    //normal rom mode
    NOR_WRITE(base_address, 0x555, 0xf0);
}

drl_nor_info_t* DRL_NOR_Probe_Toshiba(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_Toshiba_Codes(base_address, nor_read_id_codes);
  for (dev = toshiba_parts; *dev != NULL; dev++)
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