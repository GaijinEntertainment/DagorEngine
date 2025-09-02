//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_overrideStates.h>

#include <render/daFrameGraph/variableRateShading.h>
#include <render/daFrameGraph/detail/nodeNameId.h>


namespace dafg
{

struct InternalRegistry;

/// \brief Represents a request for a certain global state to be set during the node's execution.
class StateRequest
{
  friend class Registry;
  StateRequest(InternalRegistry *reg, NodeNameId nodeId);

public:
  /**
   * \brief Requests for a \p block to be set to FRAME layer
   * Values of frame block shader vars are supposed to change once per frame.
   *
   * \param block The name of the block to set. Must be present in the
   *   shader dump.
   */
  StateRequest setFrameBlock(const char *block) &&;

  /**
   * \brief Requests for a \p block to be set to SCENE layer.
   * Values of scene block shader vars are supposed to change when
   * the rendering mode changes. Examples of rendering modes are;
   * depth pre-pass, shadow pass, color pass, voxelization pass, etc.
   *
   * \param block The name of the block to set. Must be present in the
   *   shader dump.
   */
  StateRequest setSceneBlock(const char *block) &&;

  /**
   * \brief Requests for a \p block to be set to OBJECT layer.
   * Per-object blocks are evil and should be avoided at all costs.
   * They imply a draw-call-per-object model, which has historically
   * proven itself antagonistic to performance.
   *
   * \param block The name of the block to set. Must be present in the
   *   shader dump.
   */
  StateRequest setObjectBlock(const char *block) &&;

  /**
   * \brief Requests for the driver wireframe mode to be enabled for
   * this node when the user turns it on for debug purposes.
   */
  StateRequest allowWireframe() &&;

  /**
   * \brief Enable variable rate shading with as little rendering as possible.
   *
   * \warning Incompatible with using a VRS rate texture.
   */
  StateRequest maxVrs() &&;

  /**
   * \brief Requests a shader pipeline state override to be active
   * while this node is executing.
   *
   * \param override The override to use.
   */
  StateRequest enableOverride(shaders::OverrideState override) &&;

private:
  NodeNameId id;
  InternalRegistry *registry;
};

} // namespace dafg
