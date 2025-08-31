//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;

// read TIFF headers and dimensions
bool read_tiff32_dimensions(const char *fn, int &out_w, int &out_h, bool &out_may_have_alpha);

// load TIFF file as 8bit 4channel image (RGBX/RGBA)
TexImage32 *load_tiff32(const char *fn, IMemAlloc *mem, bool *out_used_alpha = NULL);
TexImage32 *load_tiff32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha = NULL);

// save TIFF file as 8bit 4(3)channel image (RGBA/RGB)
bool save_tiff32(const char *fn, const TexPixel32 *, int w, int h, int stride, unsigned char *app_data = 0,
  unsigned int app_data_len = 0);
bool save_tiff24(const char *fn, const TexPixel32 *, int w, int h, int stride, unsigned char *app_data = 0,
  unsigned int app_data_len = 0);
bool save_tiff32(const char *fn, TexImage32 *, unsigned char *app_data = 0, unsigned int app_data_len = 0);
bool save_tiff24(const char *fn, TexImage32 *, unsigned char *app_data = 0, unsigned int app_data_len = 0);
