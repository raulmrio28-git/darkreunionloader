#pragma once

#include "dcc/dn_dcc_proto.h"
//DRL - modify CFI to NOR 20260713
#include "flash/nor/nor.h"
//DRL - modify CFI to NOR 20260713
#include "flash/nand/nand.h"
#include "flash/onenand/onenand.h"
#include "flash/superand/superand.h"

static Device devices[] = {
//DRL - modify CFI to NOR 20260713
#ifdef USE_NOR_FLASH
    {&nor_controller, 0x0},
    {&nor_controller, 0x12000000},
#endif
//DRL - modify CFI to NOR 20260713
//DRL - add NAND support 20260715
#ifdef USE_NAND_FLASH
    {&nand_controller, 0x0},
#endif
//DRL - add NAND support 20260715
    {0x0, 0x0}
};