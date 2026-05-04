// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "basetex.h"
#include "driver_defs.h"

#include <util/dag_globDef.h>
#include <generic/dag_smallTab.h>
#include <3d/ddsxTex.h>
#include <DXGIFormat.h>
#include <EASTL/variant.h>

#define MAX_STAT_NAME 64
#define MAX_MIPLEVELS 16

struct ResourceDumpTexture;
struct ResourceDumpBuffer;
struct ResourceDumpRayTrace;
typedef eastl::variant<ResourceDumpTexture, ResourceDumpBuffer, ResourceDumpRayTrace> ResourceDumpInfo;

namespace ddsx
{
struct Header;
}

namespace drv3d_dx11
{

Texture *create_d3d_tex(ID3D11Texture2D *tex_res, const char *name, int flg, int backbufferCount = 1);
Texture *create_backbuffer_tex(int id, IDXGI_SWAP_CHAIN *swap_chain);

void dump_resources(Tab<ResourceDumpInfo> &dump_info);

void clear_textures_pool_garbage();
} // namespace drv3d_dx11
