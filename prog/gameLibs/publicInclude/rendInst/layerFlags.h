//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once


namespace rendinst
{

enum LayerFlags : uint32_t
{
  LAYER_OPAQUE = 0x1,
  LAYER_TRANSPARENT = 0x2,
  LAYER_DECALS = 0x4,
  LAYER_DISTORTION = 0x8,
  LAYER_FOR_GRASS = 0x10,
  LAYER_RENDINST_CLIPMAP_BLEND = 0x20,
  LAYER_RENDINST_HEIGHTMAP_PATCH = 0x80,
  LAYER_NOT_EXTRA = 0x100,
  LAYER_NO_SEPARATE_ALPHA = 0x200,
  LAYER_FORCE_LOD_MASK = 0xF000,
  LAYER_FORCE_LOD_SHIFT = 12,
  LAYER_STAGES = LAYER_DECALS | LAYER_TRANSPARENT | LAYER_OPAQUE | LAYER_NOT_EXTRA,
};

}
