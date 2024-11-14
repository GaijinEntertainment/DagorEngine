// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <EASTL/span.h>

// Argument handler will transform this into RangeType
template <typename T>
struct ExtraDataArray
{
  size_t index = 0;

  ExtraDataArray() = default;
  ExtraDataArray(size_t i) : index{i} {}

  using RangeType = eastl::span<T>;
};
