//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum HdrMode
{
  HDR_MODE_NONE = 0,
  HDR_MODE_FAKE = 1,
  HDR_MODE_FLOAT = 2,
  HDR_MODE_10BIT = 3,
  HDR_MODE_16BIT = 4,
  HDR_MODE_PS3 = 5,     // opaque is RGBE, final is 16 bit float
  HDR_MODE_XBLADES = 6, // old postfx used in X-Blades
};

#include <supp/dag_define_COREIMP.h>

extern KRNLIMP int hdr_render_mode;
extern KRNLIMP unsigned int hdr_render_format;
extern KRNLIMP unsigned int hdr_opaque_render_format;
extern KRNLIMP float hdr_max_overbright; // default maximum overbright. 1 for float/fake, 4 for 10bit, 32 for 16bit
extern KRNLIMP float hdr_max_value;      // maximum value in render target
extern KRNLIMP float hdr_max_bright_val; // histogram width

inline HdrMode get_hdr_mode() { return (HdrMode)::hdr_render_mode; }

#include <supp/dag_undef_COREIMP.h>
