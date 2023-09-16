//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes
struct TexImage32;
class IGenLoad;


// load TIFF file as 8bit 4channel image (RGBX/RGBA)
TexImage32 *load_tiff32(const char *fn, IMemAlloc *mem, bool *out_used_alpha = NULL);
TexImage32 *load_tiff32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha = NULL);

// save TIFF file as 8bit 4(3)channel image (RGBA/RGB)
bool save_tiff32(const char *fn, TexImage32 *, unsigned char *app_data = 0, unsigned int app_data_len = 0);
bool save_tiff24(const char *fn, TexImage32 *, unsigned char *app_data = 0, unsigned int app_data_len = 0);
