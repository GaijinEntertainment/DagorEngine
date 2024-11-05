//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum RenderType
{
  RENDER_PREPASS,
  RENDER_NORMAL,
  RENDER_SUN_SHADOWMAP1,
  RENDER_SUN_SHADOWMAP2,
  RENDER_SUN_SHADOWMAP3,
  RENDER_SUN_SHADOWMAP4,
  RENDER_SHADOWS,
  RENDER_UNKNOWN,
};

extern RenderType current_render_type;
