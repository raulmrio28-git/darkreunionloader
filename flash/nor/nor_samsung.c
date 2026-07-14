/*
Samsung-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

drl_nor_info_t K5N5629ABM = 
{
  "SAMSUNG K5N5629ABM",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2601}                   /* Manufacture codes. */
};
drl_nor_info_t K5N5629ATC = 
{
  "SAMSUNG K5N5629ATC",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2208}                   /* Manufacture codes. */
};

drl_nor_info_t K5N5629ABC = 
{
  "SAMSUNG K5N5629ABC",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2209}                   /* Manufacture codes. */
};

drl_nor_info_t K5N5629AUC = 
{
  "SAMSUNG K5N5629AUC",
  2,                                 /* # of codes to match */
  {0x00EC, 0x3018}                   /* Manufacture codes. */
};

drl_nor_info_t K5N6433ABM = 
{
  "SAMSUNG K5N6433ABM",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2255}                   /* Manufacture codes. */
};

drl_nor_info_t K5N6433ATM = 
{
  "SAMSUNG K5N6433ATM",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2254}                   /* Manufacture codes. */
};

drl_nor_info_t K5N2833ATB = 
{
  "SAMSUNG K5N2833ATB",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2404}                   /* Manufacture codes. */
};

drl_nor_info_t K5N2833ABB = 
{
  "SAMSUNG K5N2833-66ABB",
  2,                                 /* # of codes to match */
  {0x00EC, 0x2405}                   /* Manufacture codes. */
};

//As used in Jivi JV-C200
//Also used for a different flash chip

drl_nor_info_t K5J6332CT =
{
  "SAMSUNG K5A6317CT/K5J6332CT",
  2,
  {0x00EC, 0x22E0}
};

//As used in ZTE S189 and C306

drl_nor_info_t K5L6331CA =
{
  "SAMSUNG K5L6331CA",
  4,
  {0x00EC, 0x257E}
};


const drl_nor_info_t *(samsung_parts[]) =
{
  &K5N5629ABM,
  &K5N6433ABM,
  &K5N6433ATM,
  &K5N2833ATB,
  &K5N2833ABB,
  &K5N5629ATC,
  &K5N5629ABC,
  &K5N5629AUC,
  &K5J6332CT,
  &K5L6331CA,
  NULL   
};

void DRL_NOR_Probe_Samsung_Codes(uint32_t base_address, uint16_t* codes)
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
    //reset2
    NOR_WRITE(base_address, 0, 0xf0);
    //autoselect mode
    NOR_WRITE(base_address, 0x555, 0xaa);
    NOR_WRITE(base_address, 0x2aa, 0x55);
    //read ID
    NOR_WRITE(base_address, 0x555, 0x90);
    codes[1] = NOR_READ(base_address, 0x01);
    //normal rom mode
    NOR_WRITE(base_address, 0, 0xf0);
}

drl_nor_info_t* DRL_NOR_Probe_Samsung(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_Samsung_Codes(base_address, nor_read_id_codes);
  for (dev = samsung_parts; *dev != NULL; dev++)
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