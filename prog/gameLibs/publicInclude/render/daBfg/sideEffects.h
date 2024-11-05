//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace dabfg
{

/// \brief Specified the side effects of executing a node.
enum class SideEffects : uint8_t
{
  /// Node has an empty execution callback that can safely be skipped
  None,
  /// Default: node only accesses daBfg state and may be culled away.
  Internal,
  /// Node has side effects outside daBfg and will never be culled away.
  External
};

} // namespace dabfg
