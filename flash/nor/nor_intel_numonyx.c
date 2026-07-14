/*
Intel/Numonyx-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

/* Intel parts go here */

//As used in Jivi JV-C200

drl_nor_info_t PF38F3040M0Y3DF =
{
  "INTEL PF38F3040M0Y3DF",
  2,
  {0x0089, 0x8903}
};

//As used in Haier M371

drl_nor_info_t PH28F640W18B85 = 
{
  "INTEL PH28F640W18B85",
  2,
  {0x0089, 0x8865}
};
drl_nor_info_t PF38F4050M0Y3DE =
{
  "INTEL PF38F4050M0Y3DE",
  2,
  {0x0089, 0x8904}
};
drl_nor_info_t PF38F5060M0Y3DFA =
{
  "INTEL PF38F5060M0Y3DFA",
  2,
  {0x0089, 0x8881}
};
//As used in LG RD3630

drl_nor_info_t PF38F1030W0ZBQ0 =
{
  "INTEL PF38F1030W0ZBQ0",
  2,
  {0x0089, 0x8890}
};

/* Numonyx parts go here */

drl_nor_info_t M36W0R5040U6ZS = 
{
  "NUMONYX M36W0R5040U6ZS",
  2,                             /* # of codes to match */    
  { 0x20, 0x8828 },              /* Manufacture codes. */
};

drl_nor_info_t M58LR128KC = 
{
  "NUMONYX M58LR128KC",
  2,                             /* # of codes to match */    
  { 0x20, 0x882E },
};

//As used in Alcatel OT-219C
//NOTE: MISLABELED AS AN INTEL FLASH

drl_nor_info_t M36W0R6050 =
{
  "NUMONYX M36W0R6050",
  2,
  {0x0020, 0x88C0}
};

//As used in Huawei C2807
//NOTE: MISLABELED AS M28W640HCT
//Also added its bottomboot variant

drl_nor_info_t M28W320CT =
{
  "NUMONYX M28W320CT",
  2,
  {0x0020, 0x88BA}
};

drl_nor_info_t M28W320CB =
{
  "NUMONYX M28W320CT",
  2,
  {0x0020, 0x88BB}
};

drl_nor_info_t M28W640ECT =
{
  "NUMONYX M28W640(E/H)CT",
  2,
  {0x0020, 0x8848}
};

drl_nor_info_t M28W640ECB =
{
  "NUMONYX M28W640(E/H)CB",
  2,
  {0x0020, 0x8849}
};

//As used in Micromax C111 (QSC6010 version)

drl_nor_info_t M36C0W6050T0 =
{
  "ST M36C0W6050T0",
  2,
  {0x0020, 0x8848}
};

const drl_nor_info_t *(intel_parts[]) =
{
  &PF38F3040M0Y3DF,
  &PH28F640W18B85,
  &PF38F4050M0Y3DE,
  &PF38F5060M0Y3DFA,
  &PF38F1030W0ZBQ0,
  &M36W0R6050,
  &M28W320CT,
  &M28W320CB,
  &M28W640ECT,
  &M28W640ECB,
  &M36W0R5040U6ZS,
  &M36C0W6050T0,
  &M58LR128KC,
  NULL   
};

void DRL_NOR_Probe_Intel_Numonyx_Codes(uint32_t base_address, uint16_t* codes)
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

drl_nor_info_t* DRL_NOR_Probe_Intel_Numonyx(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_Intel_Numonyx_Codes(base_address, nor_read_id_codes);
  for (dev = intel_parts; *dev != NULL; dev++)
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
