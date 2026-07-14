#pragma once
#include "dcc/dn_dcc_proto.h"

#define NOR_READ(o, x) READ_U16(o + ((x) * 2))
#define NOR_WRITE(o, x, y) WRITE_U16(o + ((x) * 2), y)

typedef struct {
    const char* model;
    uint16_t    num_ids;
    uint16_t    codes[4];
} drl_nor_info_t;

typedef  drl_nor_info_t* (*drl_nor_cfi_probe)(volatile uint16_t* base_address);

/* nor_cfi.c */

extern void DRL_NOR_CFI_Read(uint32_t base_address, uint16_t* data, uint32_t len, bool intel);

extern Driver nor_controller;