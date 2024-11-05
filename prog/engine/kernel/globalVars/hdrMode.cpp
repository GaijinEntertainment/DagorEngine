// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texFlags.h>
#include "vars.h"
#include <fx/dag_hdrRender.h>

int hdr_render_mode = HDR_MODE_NONE;
unsigned hdr_render_format = TEXCF_BEST | TEXCF_ABEST;
unsigned hdr_opaque_render_format = TEXCF_BEST | TEXCF_ABEST;
float hdr_max_overbright = 1.0f;
float hdr_max_value = 1.0f;
float hdr_max_bright_val = 1.0f;
