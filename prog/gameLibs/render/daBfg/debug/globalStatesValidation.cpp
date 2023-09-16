#include <api/internalRegistry.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>


namespace dabfg
{

constexpr auto shaderVarValidationMessage = "Frame graph node %s expected the shader variable %s to be bound to the %s %s, "
                                            "but it was spuriously changed to %s while executing the node! Please remove the spurious "
                                            "set, or reset it back to the expected value at the end of the function execution!";

constexpr auto shaderBlockValidationMessage =
  "Frame graph node %s expected the %s layer to contain the shader block %s, "
  "but it was spuriously changed to %s while executing the node! Please remove the spurious set, "
  "or reset it back to the expected value at the end of the function execution!";

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
      const auto it = provided.find(resolve_res_id(registry, res.resource));

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
          continue;
        }
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
      logerr(shaderVarValidationMessage, registry.knownNodeNames.getName(nodeId), VariableMap::getVariableName(shVar), resTypeStr,
        get_managed_res_name(expectedId), get_managed_res_name(observedId));
  }

  auto validateBlock = [&registry, nodeId](int node_block, int global_block, const char *layer) {
    if (node_block != global_block)
    {
      const char *nodeBlockName = ShaderGlobal::getBlockName(node_block);
      const char *globalBlockName = ShaderGlobal::getBlockName(global_block);
      logerr(shaderBlockValidationMessage, registry.knownNodeNames.getName(nodeId), layer, nodeBlockName, globalBlockName);
    }
  };

  validateBlock(nodeData.shaderBlockLayers.frameLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME), "frame");
  validateBlock(nodeData.shaderBlockLayers.sceneLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE), "scene");
  validateBlock(nodeData.shaderBlockLayers.objectLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT), "object");
}

} // namespace dabfg