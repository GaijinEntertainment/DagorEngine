//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;


// read AVIF headers and dimensions
bool read_avif32_dimensions(const char *fn, int &out_w, int &out_h, bool &out_may_have_alpha);

// load AVIF file (RGBX/RGBA)
TexImage32 *load_avif32(const char *fn, IMemAlloc *mem, bool *out_used_alpha = nullptr);
TexImage32 *load_avif32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha = nullptr);
