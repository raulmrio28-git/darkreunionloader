/*
AMD/Spansion/Cypress-related probe code
*/

#include "nor.h"
#include "dcc/dn_dcc_proto.h"
#include "dcc/plat.h"

//for debugs!
extern uint16_t nor_read_id_codes[];

/* AMD parts go here */

drl_nor_info_t AM41LV3204MT10I = 
{
  "AMD Am41LV3204MT10I",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x221A, 0x2201 }                   /* Manufacture codes. */
};

drl_nor_info_t AM41LV3204MB10I = 
{
  "AMD Am41LV3204MB10I",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x221A, 0x2200 }                   /* Manufacture codes. */
};

drl_nor_info_t AM29BDS643G = 
{
  "AMD Am29BDS643G",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x2202, 0x2200 }                   /* Manufacture codes. */
};

/* Spansion parts go here */

drl_nor_info_t S29WS256N0SB = 
{
  "SPANSION S29WS256N0SB",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x2230, 0x2200 }                   /* Manufacture codes. */
};

drl_nor_info_t S29WS512P =
{
  "SPANSION S29WS512P",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x223D, 0x2200 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS64R_TOP =
{
  "SPANSION S71VS64R_TOP",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0061, 0x0001 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS64R_BOT =
{
  "SPANSION S71VS64R_BOT",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0061, 0x0002 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS128R_TOP =
{
  "SPANSION S71VS128R_TOP",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0063, 0x0001 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS128R_BOT =
{
  "SPANSION S71VS128R_BOT",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0065, 0x0001 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS256R_TOP =
{
  "SPANSION S71VS256R_TOP",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0064, 0x0001 },                  /* Manufacture codes. */
};

drl_nor_info_t S71VS256R_BOT =
{
  "SPANSION S71VS256R_BOT",
  4,                                              /* # of codes to match */
  {1,  0x007e, 0x0066, 0x0001 },                  /* Manufacture codes. */
};

drl_nor_info_t S29PL256N =
{
  "SPANSION S29PL256N",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x223C, 0x2200 },                  /* Manufacture codes. */
};

drl_nor_info_t S29PL127NJ =
{
  "SPANSION S29PL127N/J",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x2220, 0x2200 },                  /* Manufacture codes. */
};

drl_nor_info_t S29PL129N =
{
  "SPANSION S29PL129N",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x2221, 0x2200 },                  /* Manufacture codes. */
};

drl_nor_info_t S29PL064J =
{
  "SPANSION S29PL064J",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x2202, 0x2201 },                  /* Manufacture codes. */
};

drl_nor_info_t S29PL032J =
{
  "SPANSION S29PL032J",
  4,                                              /* # of codes to match */
  {1,  0x227e, 0x220A, 0x2201 },                  /* Manufacture codes. */
};

drl_nor_info_t S29NS512P =
{
  "SPANSION S29NS512P",
  4,                                              /* # of codes to match */
  {1,  0x307e, 0x303F, 0x3000 },                  /* Manufacture codes. */
};

drl_nor_info_t S29NS256P =
{
  "SPANSION S29NS256P",
  4,                                              /* # of codes to match */
  {1,  0x317e, 0x3141, 0x3100 },                  /* Manufacture codes. */
};

drl_nor_info_t S29NS128P =
{
  "SPANSION S29NS256P",
  4,                                              /* # of codes to match */
  {1,  0x327e, 0x3243, 0x3200 },                  /* Manufacture codes. */
};

//As used in Jivi JV-C200

drl_nor_info_t S71NS256PC0 =
{
  "SPANSION S71NS256PC0",
  4,
  {0x0001, 0x317E, 0x3141, 0x3100}
};

drl_nor_info_t S71NS128PC0 =
{
  "SPANSION S71NS128PC0",
  4,
  {0x0001, 0x327E, 0x3243, 0x3200}
};

//As used in Haier M371

drl_nor_info_t S71WS256PD0 =
{
  "SPANSION S71WS256PD0",
  4,
  {0x0001, 0x227E, 0x2242, 0x2200}
};

drl_nor_info_t S71NS512R =
{
  "SPANSION S71NS512R",
  4,
  {0x0001, 0x387E, 0x3816, 0x3803}
};

//As used in Alcatel OT-219C

drl_nor_info_t S29NS064N = 
{
  "SPANSION S29NS064N",
  4,
  {0x0001, 0x2B7E, 0x2B33, 0x2B00}
};

//As used in MTS C131
//This is not the same identifier as in the firmware
//because the flash die can be either top or bottom boot

drl_nor_info_t S71VS064R_TOP = 
{
  "SPANSION S71VS064R_TOP",
  4,
  {0x0001, 0x007E, 0x0061, 0x0001}
};

drl_nor_info_t S71VS064R_BOT = 
{
  "SPANSION S71VS064R_BOT",
  4,
  {0x0001, 0x007E, 0x0061, 0x0002}
};

//As used in Hisense C127
//Same thing to do as MTS C131

drl_nor_info_t S71GL064A_TOP =
{
  "SPANSION S71GL064A_TOP",
  4,
  {0x0001, 0x227E, 0x2210, 0x2200}
};

drl_nor_info_t S71GL064A_BOT =
{
  "SPANSION S71GL064A_BOT",
  4,
  {0x0001, 0x227E, 0x2210, 0x2200}
};

drl_nor_info_t S71GL032A_TOP =
{
  "SPANSION S71GL032A_TOP",
  4,
  {0x0001, 0x227E, 0x221A, 0x2201}
};

drl_nor_info_t S71GL032A_BOT =
{
  "SPANSION S71GL032A_BOT",
  4,
  {0x0001, 0x227E, 0x221A, 0x2200}
};

drl_nor_info_t S71PL032J =
{
  "SPANSION S71PL032J",
  4,
  {0x0001, 0x227E, 0x220A, 0x2201}
};

//As used in ZTE S189

drl_nor_info_t S71PL064J =
{
  "SPANSION S71PL064J",
  4,
  {0x0001, 0x227E, 0x2202, 0x2201}
};

const drl_nor_info_t *(spansion_parts[]) = {
  &AM41LV3204MT10I,
  &AM41LV3204MB10I,
  &AM29BDS643G,
  &S29WS256N0SB,
  &S29WS512P,
  &S71WS256PD0,
  &S71GL064A_TOP,
  &S71GL064A_BOT,
  &S71GL032A_TOP,
  &S71GL032A_BOT,
  &S71PL064J,
  &S71PL032J,
  &S71NS512R,
  &S71NS256PC0, 
  &S71NS128PC0,
  &S71VS064R_TOP,
  &S71VS064R_BOT,
  &S71VS128R_TOP,
  &S71VS128R_BOT,
  &S71VS256R_TOP,
  &S71VS256R_BOT,
  &S29NS064N,
  &S29NS512P,
  &S29NS256P,
  &S29NS128P,
  &S29PL256N,
  &S29PL127NJ,
  &S29PL129N,
  &S29PL064J,
  &S29PL032J,
  NULL
};


void DRL_NOR_Probe_AMD_Spansion_Codes(uint32_t base_address, uint16_t* codes)
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
    codes[2] = NOR_READ(base_address, 0x0E);
    codes[3] = NOR_READ(base_address, 0x0F);
    //normal rom mode
    NOR_WRITE(base_address, 0, 0xf0);
}

drl_nor_info_t* DRL_NOR_Probe_AMD_Spansion(uint32_t base_address)
{
  drl_nor_info_t const **dev;

  int ids, ids_found;

  DRL_NOR_Probe_AMD_Spansion_Codes(base_address, nor_read_id_codes);
  for (dev = spansion_parts; *dev != NULL; dev++)
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