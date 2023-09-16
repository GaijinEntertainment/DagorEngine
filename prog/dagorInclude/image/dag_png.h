//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;


// load PNG file (RGBX/RGBA)
TexImage32 *load_png32(const char *fn, IMemAlloc *mem, bool *out_used_alpha);
TexImage32 *load_png32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha);

bool save_png32(const char *fn, TexPixel32 *ptr, int wd, int ht, int stride,
  unsigned char *text_data, // should be nullptr or valid C-string
  unsigned int text_data_len, bool save_alpha = true);
