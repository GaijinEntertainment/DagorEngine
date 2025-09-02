// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/stateRequest.h>
#include <frontend/internalRegistry.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_info.h>


namespace dafg
{

StateRequest::StateRequest(InternalRegistry *reg, NodeNameId nodeId) : id{nodeId}, registry{reg}
{
  auto &reqs = registry->nodes[id].stateRequirements;
  if (DAGOR_UNLIKELY(reqs.has_value()))
  {
    logerr("daFG: Global state requested twice on '%s' frame graph node!"
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
    logerr("daFG: Block requested to be set to layer 'FRAME' twice within"
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
    logerr("daFG: Block requested to be set to layer 'SCENE' twice within"
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
    logerr("daFG: Block requested to be set to layer 'OBJECT' twice within"
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

StateRequest StateRequest::maxVrs() &&
{
  auto &RenderReq = registry->nodes[id].renderingRequirements;
  if (DAGOR_UNLIKELY(RenderReq.has_value() && RenderReq->vrsRateAttachment.nameId != ResNameId::Invalid))
  {
    logerr("daFG: Node %s requested maxVrs after requesting a VRS rate texture to be used!"
           " Using max VRS and using a rate texture are incompatible, ignoring maxVrs.",
      registry->knownNames.getName(id));

    return *this;
  }

  VrsStateRequirements vrsSet;

  if (d3d::get_driver_desc().caps.hasVariableRateShadingBy4)
  {
    vrsSet.rateX = 4;
    vrsSet.rateY = 4;
  }
  else if (d3d::get_driver_desc().caps.hasVariableRateShading)
  {
    vrsSet.rateX = 2;
    vrsSet.rateY = 2;
  }

  registry->nodes[id].stateRequirements->vrsState = vrsSet;

  return *this;
}

StateRequest StateRequest::enableOverride(shaders::OverrideState override) &&
{
  if (DAGOR_LIKELY(override.bits))
    registry->nodes[id].stateRequirements->pipelineStateOverride = override;
  return *this;
}

} // namespace dafg
