//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace dabfg
{

/**
 * \ingroup DabfgCore
 * \brief Describes various global state that can influence the
 * execution of the frame graph but is not managed by daBfg.
 */
struct ExternalState
{
  /// Enables wireframe debug mode for nodes that allow it.
  bool wireframeModeEnabled = false;
  /**
   * Enables variable rate shading for all nodes that allow it using
   * the per-node settings specified inside VrsRequirements.
   */
  bool vrsEnabled = false;
};

} // namespace dabfg
