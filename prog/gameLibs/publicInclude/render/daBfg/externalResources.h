//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/variant.h>

#include <generic/dag_fixedMoveOnlyFunction.h>
#include <render/daBfg/multiplexing.h>
#include <3d/dag_resPtr.h>


namespace dabfg
{

/**
 * \brief A concrete physical resource provided to daBfg for external
 * virtual resource on a particular multiplexing iteration.
 */
using ExternalResource = eastl::variant<ManagedTexView, ManagedBufView>;

/**
 * \brief A callback that provides a physical resource to daBfg for a
 * particular external virtual resource on a particular multiplexing
 * iteration. Note that if a node that registers this virtual resource
 * is multiplexed along any dimension, this callback will be called
 * multiple times and must return different values each time.
 */
using ExternalResourceProvider = dag::FixedMoveOnlyFunction<32, ExternalResource(const multiplexing::Index &) const>;

} // namespace dabfg
