#include <render/daBfg/stateRequest.h>
#include <api/internalRegistry.h>
#include <shaders/dag_shaderBlock.h>


namespace dabfg
{

StateRequest::StateRequest(InternalRegistry *reg, NodeNameId nodeId) : id{nodeId}, registry{reg}
{
  auto &reqs = registry->nodes[id].stateRequirements;
  if (EASTL_UNLIKELY(reqs.has_value()))
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

  if (EASTL_UNLIKELY(previousValue != -1))
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

  if (EASTL_UNLIKELY(previousValue != -1))
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

  if (EASTL_UNLIKELY(previousValue != -1))
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

StateRequest StateRequest::allowVrs(VrsRequirements vrs) &&
{
  if (!vrs.rateTexture.has_value())
  {
    registry->nodes[id].stateRequirements->vrsState.reset();
    return *this;
  }

  const auto rateTexId = vrs.rateTexture->resUid.resId;
  eastl::move(*vrs.rateTexture).texture().atStage(dabfg::Stage::ALL_GRAPHICS).useAs(dabfg::Usage::VRS_RATE_TEXTURE);
  registry->nodes[id].stateRequirements->vrsState =
    VrsStateRequirements{vrs.rateX, vrs.rateY, rateTexId, vrs.vertexCombiner, vrs.pixelCombiner};
  return *this;
}

StateRequest StateRequest::enableOverride(shaders::OverrideState override) &&
{
  registry->nodes[id].stateRequirements->pipelineStateOverride = override;
  return *this;
}

} // namespace dabfg
