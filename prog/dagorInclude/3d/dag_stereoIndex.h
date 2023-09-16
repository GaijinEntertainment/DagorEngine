//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum class StereoIndex
{
  Mono,
  Left,
  Right,
  Bounding = Mono,
};

constexpr uint32_t operator*(StereoIndex index) noexcept { return static_cast<uint32_t>(index); }
