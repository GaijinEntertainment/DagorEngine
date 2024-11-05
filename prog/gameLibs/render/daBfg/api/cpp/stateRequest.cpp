// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/stateRequest.h>
#include <frontend/internalRegistry.h>
#include <shaders/dag_shaderBlock.h>


namespace dabfg
{

StateRequest::StateRequest(InternalRegistry *reg, NodeNameId nodeId) : id{nodeId}, registry{reg}
{
  auto &reqs = registry->nodes[id].stateRequirements;
  if (DAGOR_UNLIKELY(reqs.has_value()))
  {
    logerr("Global state requested twice on '%s' frame graph node!"
           " Ignoring one of the requests!",
      registry->knownNames.getName(nodeId));
  }
  reqs.emplace();
}

StateRequest StateRequest::setFrameBlock(const char *block) &&
{
  int previousValue = eastl::exchange(registry->nodes[id].shaderBlockLayers.frameLayer, ShaderGlobal::getBlockId(block));

  if (DAGOR_UNLIKELY(previousValue != -1))
  {
    logerr("Block requested to be set to layer 'FRAME' twice within"
           " '%s' frame graph node! Ignoring one of the requests!",
      registry->knownNames.getName(id));
  }

  return *this;
}

StateRequest StateRequest::setSceneBlock(const char *block) &&
{
  int previousValue = eastl::exchange(registry->nodes[id].shaderBlockLayers.sceneLayer, ShaderGlobal::getBlockId(block));

  if (DAGOR_UNLIKELY(previousValue != -1))
  {
    logerr("Block requested to be set to layer 'SCENE' twice within"
           " '%s' frame graph node! Ignoring one of the requests!",
      registry->knownNames.getName(id));
  }

  return *this;
}

StateRequest StateRequest::setObjectBlock(const char *block) &&
{
  int previousValue = eastl::exchange(registry->nodes[id].shaderBlockLayers.objectLayer, ShaderGlobal::getBlockId(block));

  if (DAGOR_UNLIKELY(previousValue != -1))
  {
    logerr("Block requested to be set to layer 'OBJECT' twice within"
           " '%s' frame graph node! Ignoring one of the requests!",
      registry->knownNames.getName(id));
  }

  return *this;
}

StateRequest StateRequest::allowWireframe() &&
{
  registry->nodes[id].stateRequirements->supportsWireframe = true;
  return *this;
}

StateRequest StateRequest::vrs(VrsSettings vrs) &&
{
  registry->nodes[id].stateRequirements->vrsState = VrsStateRequirements{vrs.rateX, vrs.rateY, vrs.vertexCombiner, vrs.pixelCombiner};
  return *this;
}

StateRequest StateRequest::enableOverride(shaders::OverrideState override) &&
{
  registry->nodes[id].stateRequirements->pipelineStateOverride = override;
  return *this;
}

} // namespace dabfg
