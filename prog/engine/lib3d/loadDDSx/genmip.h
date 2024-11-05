// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <3d/ddsxTex.h>

void d3d_genmip_reserve(int sz);
void d3d_genmip_store_sysmem_copy(TEXTUREID tid, const ddsx::Header &hdr, const void *data);
