// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_bitFlagsMask.h>


namespace rendinst
{

enum class DrawShadedCollisionsFlag : uint32_t
{
  Alone = 1 << 0,
  WithVis = 1 << 1,
  Wireframe = 1 << 2,
  FaceOrientation = 1 << 3,

  ALL_FLAGS = Alone | WithVis | Wireframe | FaceOrientation
};
using DrawShadedCollisionsFlags = BitFlagsMask<DrawShadedCollisionsFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(DrawShadedCollisionsFlag);

} // namespace rendinst

template <>
struct BitFlagsTraits<rendinst::DrawShadedCollisionsFlag>
{
  static constexpr auto allFlags = rendinst::DrawShadedCollisionsFlag::ALL_FLAGS;
};

constexpr BitFlagsMask<rendinst::DrawShadedCollisionsFlag> operator~(rendinst::DrawShadedCollisionsFlag bit)
{
  return ~make_bitmask(bit);
}
