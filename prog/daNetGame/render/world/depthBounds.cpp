// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "depthBounds.h"

#include <util/dag_convar.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include "global_vars.h"


static CONSOLE_BOOL_VAL("render", use_depth_bounds, true);

bool pull_depth_bounds_enabled() { return use_depth_bounds.pullValueChange(); }

void api_set_depth_bounds(float zmin, float zmax)
{
  const float zminToSet = (zmin < zmax) ? max(0.f, zmin) : zmax;
  const float zmaxToSet = (zmin < zmax) ? zmax : max(0.f, zmin);

  d3d::set_depth_bounds(zminToSet, zmaxToSet);
  ShaderGlobal::set_color4(depth_boundsVarId, Color4(zminToSet, zmaxToSet, 0, 0));
}

float far_plane_depth(int texfmt)
{
  // Basically returns minimal positive nonzero depth

  // These formats are SFLOAT
  if (texfmt == TEXFMT_DEPTH32 || texfmt == TEXFMT_DEPTH32_S8)
    return FLT_MIN;

  // These formats are UNORM
  if (texfmt == TEXFMT_DEPTH24)
    return 1.f / (1LL << 24);

  if (texfmt == TEXFMT_DEPTH16)
    return 1.f / (1LL << 16);

  G_ASSERTF_RETURN(false, 0, "Unknown depth format!");
}

bool depth_bounds_enabled() { return d3d::get_driver_desc().caps.hasDepthBoundsTest && use_depth_bounds.get(); }
