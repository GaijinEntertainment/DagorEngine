//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// forward declarations for external classes
struct TexImage32;
class IGenLoad;


// load PSD file (RGBX/RGBA)
TexImage32 *load_psd32(const char *fn, IMemAlloc *mem, bool *out_used_alpha);
TexImage32 *load_psd32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha);
