//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>

enum SsaoCreationFlags
{
  SSAO_NONE = 0,
  SSAO_USE_WSAO = 1 << 0,
  SSAO_USE_CONTACT_SHADOWS = 1 << 1,
  SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED = 1 << 2,
  SSAO_IMMEDIATE = 1 << 3,
  SSAO_SKIP_POISSON_POINTS_GENERATION = 1 << 4,
  SSAO_SKIP_RANDOM_PATTERN_GENERATION = 1 << 5,

  SSAO_ALLOW_IMPLICIT_FLAGS_FROM_SHADER_ASSUMES = 1 << 31,
};

namespace ssao_detail
{
uint32_t consider_shader_assumes(uint32_t flags);
uint32_t creation_flags_to_format(uint32_t flags);
} // namespace ssao_detail
