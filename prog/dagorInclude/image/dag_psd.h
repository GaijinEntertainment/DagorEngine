//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
struct TexImage32;
class IGenLoad;

// read PSD headers and dimensions
bool read_psd32_dimensions(const char *fn, int &out_w, int &out_h, bool &out_may_have_alpha);

// load PSD file (RGBX/RGBA)
TexImage32 *load_psd32(const char *fn, IMemAlloc *mem, bool *out_used_alpha);
TexImage32 *load_psd32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha);
