//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_KRNLIMP.h>

//! initializes symbolic debug info module
KRNLIMP bool symhlp_init(bool in = false);
//! default initialization for executable and some known dlls
KRNLIMP void symhlp_init_default();

//! finds base address and module size for specified PE binary (module must be already loaded!)
//! pe_img_name must be full path name to module (it is compared as path string)
KRNLIMP bool symhlp_find_module_addr(const char *pe_img_name, uintptr_t &out_base_addr, uintptr_t &out_module_size);

//! loads symbols for PE binary (.exe, .dll, etc.)
//! when base_addr==0, symhlp_load() uses symhlp_find_module_addr() to determine addr/size
KRNLIMP bool symhlp_load(const char *pe_img_name, uintptr_t base_addr = 0, uintptr_t module_size = 0);

//! unloads symbols for PE binary (.exe, .dll, etc.)
//! when base_addr==0, symhlp_unload() uses symhlp_find_module_addr() to determine base address
KRNLIMP bool symhlp_unload(const char *pe_img_name, uintptr_t base_addr = 0);

//! closes symbolic debug info
KRNLIMP void symhlp_close();

#include <supp/dag_undef_KRNLIMP.h>
