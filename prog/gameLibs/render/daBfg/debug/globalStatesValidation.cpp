// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <frontend/internalRegistry.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_info.h>
#include <math/dag_dxmath.h>
#include <backend/blobBindingHelpers.h>


namespace dabfg
{

constexpr auto shaderVarValidationMessage = "Frame graph node %s expected the shader variable %s to be bound to the %s %s, "
                                            "but it was spuriously changed to %s while executing the node! Please remove the spurious "
                                            "set, or reset it back to the expected value at the end of the function execution!";

constexpr auto shaderBlockValidationMessage =
  "Frame graph node %s expected the %s layer to contain the shader block %s, "
  "but it was spuriously changed to %s while executing the node! Please remove the spurious set, "
  "or reset it back to the expected value at the end of the function execution!";

constexpr auto shaderVarBlobValidationMessage =
  "Frame graph node %s spuriously changed a bound shader variable's value %s while executing the node."
  "Please remove the spurious set, or reset it back to the expected value at the end of the function execution!";

template <typename ProjectedType, auto bindGetter>
static void validateBlob(const InternalRegistry &registry, NodeNameId node_id, int var_id, const BlobView &blob,
  const detail::TypeErasedProjector &projector)
{
  auto expectedValue = *static_cast<const eastl::remove_reference_t<ProjectedType> *>((projector)(blob.data));
  auto actualValue = bindGetter(var_id);
  if (!ShaderVarBindingValidationHelper<typename eastl::remove_cv<decltype(expectedValue)>::type,
        typename eastl::remove_cv<decltype(actualValue)>::type>::validate(expectedValue, actualValue))
  {
    const char *nodeName = registry.knownNames.getName(node_id);
    const char *shaderVarName = VariableMap::getVariableName(var_id);
    logerr(shaderVarBlobValidationMessage, nodeName, shaderVarName);
  }
}

void validate_global_state(const InternalRegistry &registry, NodeNameId nodeId)
{
  TIME_PROFILE(fg_validate_global_state);

  const auto &nodeData = registry.nodes[nodeId];
  for (const auto &[shVar, res] : nodeData.bindings)
  {
    if (res.type != BindingType::ShaderVar)
      continue;

    D3DRESID expectedId;

    // Shaders dump can be reloaded and some shader vars can be absent in new dump
    if (!VariableMap::isGlobVariablePresent(shVar))
      continue;

    {
      const auto &provided = res.history ? registry.resourceProviderReference.providedHistoryResources
                                         : registry.resourceProviderReference.providedResources;
      const auto it = provided.find(res.resource);

      // If no resource was provided, this is an optional bind request
      // and we expect to see BAD_D3DRESID
      if (it != provided.end())
      {
        if (auto bufPtr = eastl::get_if<ManagedBufView>(&it->second))
        {
          expectedId = bufPtr->getBufId();
        }
        else if (auto texPtr = eastl::get_if<ManagedTexView>(&it->second))
        {
          expectedId = texPtr->getTexId();
        }
        else if (auto blobView = eastl::get_if<BlobView>(&it->second))
        {
          switch (ShaderGlobal::get_var_type(shVar))
          {
#define SHV_CASE(shVarType)     case shVarType:
#define SHV_CASE_END(shVarType) break;
#define TAG_CASE(projType, shVarSetter, shVarGetter)                                        \
  if (res.projectedTag == tag_for<projType>())                                              \
  {                                                                                         \
    validateBlob<projType, shVarGetter>(registry, nodeId, shVar, *blobView, res.projector); \
    break;                                                                                  \
  }
            SHV_BIND_BLOB_LIST
#undef SHV_BIND_BLOB_LIST
#undef SHV_CASE
#undef SHV_CASE_END
#undef TAG_CASE
          }
          continue;
        }
        if (eastl::holds_alternative<MissingOptionalResource>(it->second) &&
            nodeData.resourceRequests.find(res.resource)->second.optional)
          continue;
      }
    }

    D3DRESID observedId;
    // SV type and resource type mismatch is impossible at this point
    eastl::string_view resTypeStr;
    switch (ShaderGlobal::get_var_type(shVar))
    {
      case SHVT_TEXTURE:
        resTypeStr = "texture";
        observedId = ShaderGlobal::get_tex(shVar);
        break;

      case SHVT_BUFFER:
        resTypeStr = "buffer";
        observedId = ShaderGlobal::get_buf(shVar);
        break;

      default:
        G_ASSERT(false); // impossible due to invariants
        continue;
    }

    if (expectedId != observedId)
      logerr(shaderVarValidationMessage, registry.knownNames.getName(nodeId), VariableMap::getVariableName(shVar), resTypeStr,
        get_managed_res_name(expectedId), get_managed_res_name(observedId));
  }

  auto validateBlock = [&registry, nodeId](int node_block, int global_block, const char *layer) {
    if (node_block != global_block)
    {
      const char *nodeBlockName = ShaderGlobal::getBlockName(node_block);
      const char *globalBlockName = ShaderGlobal::getBlockName(global_block);
      logerr(shaderBlockValidationMessage, registry.knownNames.getName(nodeId), layer, nodeBlockName, globalBlockName);
    }
  };

  validateBlock(nodeData.shaderBlockLayers.frameLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME), "frame");
  validateBlock(nodeData.shaderBlockLayers.sceneLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE), "scene");
  validateBlock(nodeData.shaderBlockLayers.objectLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT), "object");

  // Only check on platforms that use generic RP implementation
  if (nodeData.renderingRequirements.has_value() && !d3d::get_driver_code().is(d3d::stub) &&
      !d3d::get_driver_desc().caps.hasNativeRenderPassSubPasses)
  {
    const auto &expectedRts = *nodeData.renderingRequirements;

    Driver3dRenderTarget observedRts;
    d3d::get_render_target(observedRts);

    const auto validateRt = [&registry, nodeId](const VirtualSubresourceRef &expected,
                              const eastl::optional<Driver3dRenderTarget::RTState> &maybeObserved, const char *slot_name) {
      const auto &provided = registry.resourceProviderReference.providedResources;
      const auto it = provided.find(expected.nameId);

      BaseTexture *expectedTex = nullptr;
      if (it != provided.end())
        if (auto view = eastl::get_if<ManagedTexView>(&it->second))
          expectedTex = view->getBaseTex();

      const auto expectedName = expectedTex ? expectedTex->getTexName() : "NULL";
      const auto observedName = maybeObserved.has_value() && maybeObserved->tex ? maybeObserved->tex->getTexName() : "NULL";

      // avoid the checks below for backbuffer
      if (maybeObserved.has_value() && maybeObserved->tex == nullptr)
        return;

      auto observed = maybeObserved.value_or(Driver3dRenderTarget::RTState{});

      if (expectedTex != observed.tex)
      {
        logerr("Frame graph node %s expected the render target in slot %s to be bound to %s, "
               "but it was spuriously changed to %s while executing the node! Please remove the spurious "
               "set, or reset it back to the expected value at the end of the function execution!",
          registry.knownNames.getName(nodeId), slot_name, expectedName, observedName);
        return;
      }

      if (expected.layer != observed.layer)
      {
        logerr("Frame graph node %s expected the render target in slot %s to be bound to the %d layer of texture %s, "
               "but it was spuriously changed to the %d layer while executing the node! Please remove the spurious "
               "set, or reset it back to the expected value at the end of the function execution!",
          registry.knownNames.getName(nodeId), slot_name, expected.layer, expectedName, observed.layer);
        return;
      }

      if (expected.mipLevel != observed.level)
      {
        logerr("Frame graph node %s expected the render target in slot %s to be bound to the %d mip level of texture %s, "
               "but it was spuriously changed to the %d mip level while executing the node! Please remove the spurious "
               "set, or reset it back to the expected value at the end of the function execution!",
          registry.knownNames.getName(nodeId), slot_name, expected.mipLevel, expectedName, observed.level);
        return;
      }
    };

    static constexpr const char *SLOT_NAMES[] = {
      "color 0", "color 1", "color 2", "color 3", "color 4", "color 5", "color 6", "color 7"};
    static_assert(countof(SLOT_NAMES) == Driver3dRenderTarget::MAX_SIMRT);
    for (uint32_t rtIdx = 0; rtIdx < Driver3dRenderTarget::MAX_SIMRT; ++rtIdx)
    {
      // We must validate that nothing was set into RT slots that were not requested
      VirtualSubresourceRef expectedRt = rtIdx < expectedRts.colorAttachments.size() ? expectedRts.colorAttachments[rtIdx]
                                                                                     : VirtualSubresourceRef{ResNameId::Invalid, 0, 0};
      eastl::optional<Driver3dRenderTarget::RTState> observedRt{};
      if (observedRts.isColorUsed(rtIdx))
        observedRt = observedRts.getColor(rtIdx);
      validateRt(expectedRt, observedRt, SLOT_NAMES[rtIdx]);
    }

    {
      eastl::optional<Driver3dRenderTarget::RTState> observedDepth{};
      if (observedRts.isDepthUsed())
        observedDepth = observedRts.getDepth();
      validateRt(expectedRts.depthAttachment, observedDepth, "depth");
    }

    if (expectedRts.depthAttachment.nameId != ResNameId::Invalid && expectedRts.depthReadOnly != observedRts.isDepthReadOnly())
    {
      logerr("Frame graph node %s expected to have a %s depth, but it was spuriously changed "
             "to be the opposite. Please, remove the spurious depth target change from the node!",
        registry.knownNames.getName(nodeId), expectedRts.depthReadOnly ? "read-only" : "read-write");
    }
  }
}

} // namespace dabfg