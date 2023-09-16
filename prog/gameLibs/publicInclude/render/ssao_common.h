//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
};

namespace ssao_detail
{
uint32_t creation_flags_to_format(uint32_t flags);
} // namespace ssao_detail
