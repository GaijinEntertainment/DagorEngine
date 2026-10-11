// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>

enum SsaoCreationFlags
{
  SSAO_NONE = 0,
  SSAO_USE_WSAO = 1 << 0,
  SSAO_USE_CONTACT_SHADOWS = 1 << 1,
  SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED = 1 << 2,
  SSAO_IMMEDIATE = 1 << 3,
  SSAO_SKIP_POISSON_POINTS_GENERATION = 1 << 4,

  SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES = 1 << 31,
};

enum SsaoShaderMode
{
  SSAO_MODE_IMMEDIATE = 0,
  SSAO_MODE_TEMPORAL = 1,
};

namespace ao_common
{
uint32_t consider_shader_assumes(uint32_t flags);
uint32_t creation_flags_to_format(uint32_t flags);

// The blur/temporal-sample frame counter.
struct FrameState
{
  uint32_t frameNo = 0;
};

struct ReprojectionState
{
  TMatrix4 prevGlobTm = TMatrix4(0), prevProjTm = TMatrix4(0);
  Point4 prevViewVecLT = Point4(0, 0, 0, 0), prevViewVecRT = Point4(0, 0, 0, 0), prevViewVecLB = Point4(0, 0, 0, 0),
         prevViewVecRB = Point4(0, 0, 0, 0);
  DPoint3 prevWorldPos = DPoint3(0, 0, 0);
};
} // namespace ao_common
