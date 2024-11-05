// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum FxRenderTag : uint8_t
{
  ERT_TAG_NORMAL = 0,
  ERT_TAG_FOM = 1,
  ERT_TAG_VOLMEDIA = 2,

  ERT_TAG_LOWRES = 3,
  ERT_TAG_HIGHRES = 4,
  ERT_TAG_GBUFFER = 5,
  ERT_TAG_HAZE = 6,
  ERT_TAG_ATEST = 7,
  // ERT_TAG_WATER      = 8,
  ERT_TAG_WATER_PROJ = 9,
  ERT_TAG_THERMAL = 10,
  ERT_TAG_UNDERWATER = 11,
  ERT_TAG_VOLFOG_INJECTION = 12,
  ERT_TAG_BVH = 13,
  ERT_TAG_COUNT,
};

static const char *render_tags[] = {
  "normal",   // aliased to dynamic_scene_trans
  "fom",      // dynamic_scene_trans_fom
  "volmedia", // dynamic_scene_trans_volmedia

  "lowres",
  "highres",
  "gbuffer",
  "haze",
  "atest",
  "water",
  "water_proj",
  "thermal",
  "underwater",
  "volfog_injection",
  "bvh",
};

enum
{
  FX_RENDER_MODE_NORMAL = 0,
  FX_RENDER_MODE_HAZE = 5,
  FX_RENDER_MODE_FOM = 7,
  FX_RENDER_MODE_VOLMEDIA = 8,
};