// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <drv/3d/dag_tex3d.h>


static constexpr const char *THIN_GBUFFER_RESOLVE_SHADER = "thin_g_resolve";
static constexpr int THIN_GBUFFER_RT_COUNT = 2;
static constexpr eastl::array<unsigned, THIN_GBUFFER_RT_COUNT> THIN_GBUFFER_RT_FORMATS = {
  TEXFMT_R11G11B10F,
  TEXFMT_R11G11B10F,
};

// NOTE: no motion is the same as full, but without the last layer
static constexpr const char *NO_MOTION_GBUFFER_RESOLVE_SHADER = "deferred_simple";
static constexpr const int NO_MOTION_GBUFFER_RT_COUNT = 3;

static constexpr const int FULL_GBUFFER_RT_COUNT = NO_MOTION_GBUFFER_RT_COUNT + 1;
static constexpr eastl::array<unsigned, FULL_GBUFFER_RT_COUNT> FULL_GBUFFER_FORMATS = {
  TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE,
  TEXFMT_A2B10G10R10,
  TEXFMT_A8R8G8B8,
  TEXFMT_A16B16G16R16F,
};

static constexpr const int MOBILE_GBUFFER_RT_COUNT = 3;
static constexpr eastl::array<unsigned, MOBILE_GBUFFER_RT_COUNT> MOBILE_GBUFFER_FORMATS = {
  TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_TRANSIENT,
  TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_TRANSIENT,
  TEXFMT_R8G8 | TEXCF_RTARGET | TEXCF_TRANSIENT,
};

static constexpr const int MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT = 2;
static constexpr eastl::array<unsigned, MOBILE_GBUFFER_RT_COUNT> MOBILE_SIMPLIFIED_GBUFFER_FORMATS = {
  TEXFMT_R8G8B8A8 | TEXCF_RTARGET | TEXCF_TRANSIENT,
  TEXFMT_R8G8 | TEXCF_RTARGET | TEXCF_TRANSIENT,
};

static constexpr const eastl::array<const char *, MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT> MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES = {
  "mobile_deferred_mrt0",
  "mobile_deferred_mrt1",
};

static constexpr const eastl::array<const char *, MOBILE_GBUFFER_RT_COUNT> MOBILE_GBUFFER_RT_NAMES = {
  "mobile_deferred_mrt0", "mobile_deferred_mrt1", "mobile_deferred_mrt2"};

enum
{
  GBUF_ALBEDO_AO = 0,
  GBUF_NORMAL_ROUGH_MET = 1,
  GBUF_MATERIAL = 2,
  GBUF_NUM = 4
};
