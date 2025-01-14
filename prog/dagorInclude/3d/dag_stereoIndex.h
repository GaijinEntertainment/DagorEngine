//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum class StereoIndex
{
  Mono,
  Left,
  Right,
  Bounding = Mono,
  Current = -1,
};

constexpr uint32_t operator*(StereoIndex index) noexcept { return static_cast<uint32_t>(index); }
