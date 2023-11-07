#include "nodeExecutor.h"

#include <id/idRange.h>
#include <debug/backendDebug.h>
#include <multiplexingInternal.h>

#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>

#include <math/dag_dxmath.h>
#include <math/dag_color.h>
#include <math/dag_hlsl_floatx.h>


namespace dabfg
{

template <class F, class G, class H>
void populate_resource_provider(ResourceProvider &provider, // passed by ref to avoid reallocs
  const InternalRegistry &registry, const NameResolver &resolver, NodeNameId nodeId, const F &textureProvider, const G &bufferProvider,
  const H &blobProvider)
{
  provider.providedResources.reserve(registry.nodes[nodeId].resourceRequests.size());
  provider.providedHistoryResources.reserve(registry.nodes[nodeId].historyResourceReadRequests.size());

  auto processRes = [&](ResNameId unresolvedResNameId, bool history, bool optional) {
    auto setRes = [unresolvedResNameId, history, &provider](auto resource) {
      auto &storage = history ? provider.providedHistoryResources : provider.providedResources;
      storage[unresolvedResNameId] = resource;
    };

    ResNameId resId = resolver.resolve(unresolvedResNameId);

    if (!registry.resources.isMapped(resId))
    {
      if (EASTL_UNLIKELY(!optional))
        logerr("Encountered an active node with an unmapped mandatory resource!"
               " This is an impossible situation, likely a bug in frame graph!");
      return;
    }

    switch (registry.resources[resId].type)
    {
      case ResourceType::Texture: setRes(textureProvider(history, resId)); break;

      case ResourceType::Buffer: setRes(bufferProvider(history, resId)); break;

      case ResourceType::Blob: setRes(blobProvider(history, resId)); break;

      case ResourceType::Invalid: G_ASSERT(optional); break;
    }
  };

  for (const auto &[resId, req] : registry.nodes[nodeId].resourceRequests)
    processRes(resId, false, req.optional);
  for (const auto &[resId, req] : registry.nodes[nodeId].historyResourceReadRequests)
    processRes(resId, true, req.optional);
}

void NodeExecutor::execute(int prev_frame, int curr_frame, multiplexing::Extents multiplexing_extents,
  const ResourceScheduler::FrameEventsRef &events, const NodeStateDeltas &state_deltas)
{
  currentlyProvidedResources.resolutions.resize(registry.knownNames.nameCount<AutoResTypeNameId>());
  for (auto [unresolvedId, resolution] : currentlyProvidedResources.resolutions.enumerate())
    if (auto resolvedId = nameResolver.resolve(unresolvedId); resolvedId != AutoResTypeNameId::Invalid)
      resolution = registry.autoResTypes[resolvedId].dynamicResolution;

  for (auto i : IdRange<intermediate::NodeIndex>(graph.nodes.size()))
  {
    const intermediate::Node &irNode = graph.nodes[i];
    processEvents(events[i]);

    const multiplexing::Index multiIdx = multiplexing_index_from_ir(irNode.multiplexingIndex, multiplexing_extents);
    gatherExternalResources(irNode.frontendNode, irNode.multiplexingIndex, multiIdx, graph.resources);
    populate_resource_provider(
      currentlyProvidedResources, registry, nameResolver, irNode.frontendNode,
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex](bool history, ResNameId res_id) -> ManagedTexView {
        return getManagedTexView(res_id, history ? prev_frame : curr_frame, multiIndex);
      },
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex](bool history, ResNameId res_id) -> ManagedBufView {
        return getManagedBufView(res_id, history ? prev_frame : curr_frame, multiIndex);
      },
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex](bool history, ResNameId res_id) -> BlobView {
        return getBlobView(res_id, history ? prev_frame : curr_frame, multiIndex);
      });


    validation_set_current_node(registry, irNode.frontendNode);
    applyState(state_deltas[i], curr_frame, prev_frame);
    if (const auto &node = registry.nodes[irNode.frontendNode]; node.enabled && node.sideEffect != SideEffects::None)
    {
      {
        TIME_D3D_PROFILE_NAME(FramegraphNode, registry.knownNames.getName(irNode.frontendNode));

        if (auto &exec = registry.nodes[irNode.frontendNode].execute)
          exec(multiIdx);
        else
          logerr("Somehow, a node with an empty execution callback was "
                 "attempted to be executed. This is a bug in framegraph!");
      }
      validate_global_state(registry, irNode.frontendNode);
    }
    validation_set_current_node(registry, NodeNameId::Invalid);

    // Clean up resource references inside the provider, just in case
    currentlyProvidedResources.clear();
  }
  processEvents(events.back());
  applyState(state_deltas.back(), curr_frame, prev_frame);
  validation_of_external_resources_duplication(graph.resources, graph.resourceNames);
}

void NodeExecutor::gatherExternalResources(NodeNameId nameId, intermediate::MultiplexingIndex ir_multi_idx,
  multiplexing::Index multi_idx, IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource> &resources)
{
  for (const auto &[resNameIdUnresolved, request] : registry.nodes[nameId].resourceRequests)
  {
    const ResNameId resNameId = nameResolver.resolve(resNameIdUnresolved);
    const auto &resData = registry.resources.get(resNameId);
    if (auto provider = eastl::get_if<ExternalResourceProvider>(&resData.creationInfo))
    {
      intermediate::ResourceIndex resIdx = mapping.mapRes(resNameId, ir_multi_idx);
      const ExternalResource res = (*provider)(multi_idx);
      if (auto *tex = eastl::get_if<ManagedTexView>(&res))
        G_ASSERTF(*tex, "External texture %s wasn't provided by node %s", registry.knownNames.getName(resNameId),
          registry.knownNames.getName(nameId));
      else if (auto *buf = eastl::get_if<ManagedBufView>(&res))
        G_ASSERTF(*buf, "External buffer %s wasn't provided by node %s", registry.knownNames.getName(resNameId),
          registry.knownNames.getName(nameId));

      resources[resIdx].resource = res;
    }
  }
}

void NodeExecutor::processEvents(ResourceScheduler::NodeEventsRef events) const
{
  // NOTE: split barriers are not properly implemented on vulkan
  // and lead to RP breaks without giving any benefit
  const auto ignoreBarrier = [drvCode = d3d::get_driver_code()](int rb) {
    return drvCode.is(d3d::vulkan) && (rb & RB_FLAG_SPLIT_BARRIER_BEGIN) != 0;
  };

  for (const auto &evt : events)
  {
    const intermediate::Resource &iRes = graph.resources[evt.resource];
    auto resType = iRes.getResType();

    switch (resType)
    {
      case ResourceType::Texture:
      {
        ManagedTexView tex;
        if (iRes.isExternal())
          tex = iRes.getExternalTex();
        else
          tex = resourceScheduler.getTexture(evt.frameResourceProducedOn, evt.resource);

        G_ASSERT_CONTINUE(tex);

        if (auto *data = eastl::get_if<ResourceScheduler::Event::Activation>(&evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          d3d::activate_texture(tex.getBaseTex(), data->action, data->clearValue);
        }
        else if (auto *data = eastl::get_if<ResourceScheduler::Event::Barrier>(&evt.data))
        {
          if (!ignoreBarrier(data->barrier))
            d3d::resource_barrier({tex.getBaseTex(), data->barrier, 0, 0});
        }
        else if (eastl::holds_alternative<ResourceScheduler::Event::Deactivation>(evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          d3d::deactivate_texture(tex.getBaseTex());
        }
        else
          G_ASSERTF(false, "Unexpected event type!");
      }
      break;

      case ResourceType::Buffer:
      {
        ManagedBufView buf;
        if (iRes.isExternal())
          buf = iRes.getExternalBuf();
        else
          buf = resourceScheduler.getBuffer(evt.frameResourceProducedOn, evt.resource);

        G_ASSERT_CONTINUE(buf);

        if (auto *data = eastl::get_if<ResourceScheduler::Event::Activation>(&evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          d3d::activate_buffer(buf.getBuf(), data->action, data->clearValue);
        }
        else if (auto *data = eastl::get_if<ResourceScheduler::Event::Barrier>(&evt.data))
        {
          if (!ignoreBarrier(data->barrier))
            d3d::resource_barrier({buf.getBuf(), data->barrier});
        }
        else if (eastl::holds_alternative<ResourceScheduler::Event::Deactivation>(evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          d3d::deactivate_buffer(buf.getBuf());
        }
        else
          G_ASSERTF(false, "Unexpected event type!");
      }
      break;

      case ResourceType::Blob:
      {
        auto blobView = resourceScheduler.getBlob(evt.frameResourceProducedOn, evt.resource);

        if (auto *data = eastl::get_if<ResourceScheduler::Event::CpuActivation>(&evt.data))
        {
          data->func(blobView.data);
        }
        else if (auto *data = eastl::get_if<ResourceScheduler::Event::CpuDeactivation>(&evt.data))
        {
          data->func(blobView.data);
        }
        else
          G_ASSERTF(false, "Unexpected event type!");
      }
      break;

      case ResourceType::Invalid: G_ASSERTF_BREAK(false, "Encountered an event for an invalid resource!")
    }
  }
}

void NodeExecutor::applyState(const NodeStateDelta &state, int frame, int prev_frame) const
{
  if (externalState.wireframeModeEnabled && state.wire)
    d3d::setwire(*state.wire);

  if (externalState.vrsEnabled && state.vrs.has_value())
  {
    if (auto vrs = eastl::get_if<intermediate::VrsState>(&*state.vrs))
    {
      d3d::set_variable_rate_shading(vrs->rateX, vrs->rateY, vrs->vertexCombiner, vrs->pixelCombiner);
      d3d::set_variable_rate_shading_texture(getManagedTexView(vrs->rateTexture, frame).getBaseTex());
    }
    else
    {
      d3d::set_variable_rate_shading(1, 1, VariableRateShadingCombiner::VRS_PASSTHROUGH, VariableRateShadingCombiner::VRS_PASSTHROUGH);
      d3d::set_variable_rate_shading_texture(nullptr);
    }
  }

  if (state.pass)
  {
    if (auto pass = eastl::get_if<intermediate::RenderPass>(&*state.pass))
    {
      dag::RelocatableFixedVector<RenderTarget, 8> colorTargets;
      for (const auto &attachment : pass->colorAttachments)
      {
        // If the attachment is optional and its resource is missing
        // then the corresponding color target is nullptr.
        if (attachment.has_value())
        {
          const auto tex = getManagedTexView(attachment->resource, frame).getBaseTex();
          colorTargets.emplace_back(RenderTarget{tex, attachment->mipLevel, attachment->layer});
        }
        else
          colorTargets.emplace_back(RenderTarget{nullptr, 0, 0});
      }

      RenderTarget depthTarget;
      if (pass->depthAttachment)
        depthTarget = {getManagedTexView(pass->depthAttachment->resource, frame).getBaseTex(), pass->depthAttachment->mipLevel,
          pass->depthAttachment->layer};
      else
        depthTarget = {nullptr, 0, 0};

      d3d::set_render_target(depthTarget, pass->depthReadOnly ? DepthAccess::SampledRO : DepthAccess::RW,
        dag::ConstSpan<RenderTarget>(colorTargets.data(), colorTargets.size()));
    }
    else
    {
      // NOTE: this will become end_render_pass once we move to native RPs,
      // which is very different from a pass with no attachments!
      d3d::set_render_target({nullptr, 0, 0}, DepthAccess::RW, {});
    }
  }

  applyBindings(state.bindings, frame, prev_frame);

  if (state.shaderBlockLayers.frameLayer)
    ShaderGlobal::setBlock(state.shaderBlockLayers.frameLayer.value(), ShaderGlobal::LAYER_FRAME);
  if (state.shaderBlockLayers.sceneLayer)
    ShaderGlobal::setBlock(state.shaderBlockLayers.sceneLayer.value(), ShaderGlobal::LAYER_SCENE);
  if (state.shaderBlockLayers.objectLayer)
    ShaderGlobal::setBlock(state.shaderBlockLayers.objectLayer.value(), ShaderGlobal::LAYER_OBJECT);

  if (state.shaderOverrides)
  {
    auto shaderOverride = state.shaderOverrides->get();

    // We have to reset the state every time first or the set will
    // fail. The overrides API needs to be changed for this
    // redundant reset call to be removable.
    if (shaderOverride)
      shaders::overrides::reset();
    shaders::overrides::set(shaderOverride);
  }
}


#define SHV_BIND_BLOB_LIST                                                                          \
  SHV_CASE(SHVT_INT)                                                                                \
  TAG_CASE(int, static_cast<bool (*)(int, int)>(&ShaderGlobal::set_int))                            \
  SHV_CASE_END(SHVT_INT)                                                                            \
  SHV_CASE(SHVT_REAL)                                                                               \
  TAG_CASE(float, static_cast<bool (*)(int, float)>(&ShaderGlobal::set_real))                       \
  SHV_CASE_END(SHVT_REAL)                                                                           \
  SHV_CASE(SHVT_COLOR4)                                                                             \
  TAG_CASE(Color4, static_cast<bool (*)(int, const Color4 &)>(&ShaderGlobal::set_color4))           \
  TAG_CASE(Point4, static_cast<bool (*)(int, const Point4 &)>(&ShaderGlobal::set_color4))           \
  TAG_CASE(E3DCOLOR, static_cast<bool (*)(int, E3DCOLOR)>(&ShaderGlobal::set_color4))               \
  TAG_CASE(XMFLOAT4, static_cast<bool (*)(int, const XMFLOAT4 &)>(&ShaderGlobal::set_color4))       \
  TAG_CASE(FXMVECTOR, static_cast<bool (*)(int, FXMVECTOR)>(&ShaderGlobal::set_color4))             \
  SHV_CASE_END(SHVT_COLOR4)                                                                         \
  SHV_CASE(SHVT_INT4)                                                                               \
  TAG_CASE(IPoint4, static_cast<bool (*)(int, const IPoint4 &)>(&ShaderGlobal::set_int4))           \
  SHV_CASE_END(SHVT_INT4)                                                                           \
  SHV_CASE(SHVT_FLOAT4X4)                                                                           \
  TAG_CASE(TMatrix4, static_cast<bool (*)(int, const TMatrix4 &)>(&ShaderGlobal::set_float4x4))     \
  TAG_CASE(XMFLOAT4X4, static_cast<bool (*)(int, const XMFLOAT4X4 &)>(&ShaderGlobal::set_float4x4)) \
  TAG_CASE(FXMMATRIX, static_cast<bool (*)(int, FXMMATRIX)>(&ShaderGlobal::set_float4x4))           \
  SHV_CASE_END(SHVT_FLOAT4X4)

static void bindViewSetter(int, const TMatrix4 &data) { d3d::settm(TM_VIEW, &data); };
static void bindViewSetter(int, const TMatrix &data) { d3d::settm(TM_VIEW, data); };
static void bindProjSetter(int, const TMatrix4 &data) { d3d::settm(TM_PROJ, &data); };

void NodeExecutor::applyBindings(const intermediate::BindingsMap &bindings, int frame, int prev_frame) const
{
  for (const auto &[bindIdx, bindInfo] : bindings)
  {
    // At this point, the bindings are guaranteed to be correct,
    // but (optional) resource might've still gone missing.
    switch (bindInfo.type)
    {
      case BindingType::ShaderVar:
      {
        bindShaderVar(bindIdx, bindInfo, frame, prev_frame);
        break;
      }
      case BindingType::ViewMatrix:
      {
        G_ASSERTF_BREAK(bindInfo.projectedTag == tag_for<TMatrix4>() || bindInfo.projectedTag == tag_for<TMatrix>(),
          "Binding a blob as VIEW whose projected type is not a matrix.");
        if (bindInfo.projectedTag == tag_for<TMatrix4>())
        {
          bindBlob<TMatrix4, static_cast<void (*)(int, const TMatrix4 &)>(&bindViewSetter)>(bindIdx, bindInfo, frame);
        }
        else if (bindInfo.projectedTag == tag_for<TMatrix>())
        {
          bindBlob<TMatrix, static_cast<void (*)(int, const TMatrix &)>(&bindViewSetter)>(bindIdx, bindInfo, frame);
        }
        break;
      }
      case BindingType::ProjMatrix:
      {
        G_ASSERTF_BREAK(bindInfo.projectedTag == tag_for<TMatrix4>() || bindInfo.projectedTag == tag_for<TMatrix4_vec4>(),
          "Binding a blob as PROJ whose projected type is not a matrix.");
        bindBlob<TMatrix4, &bindProjSetter>(bindIdx, bindInfo, frame);
        break;
      }
      case BindingType::Invalid: G_ASSERT(false); break;
    };
  }
}

void NodeExecutor::bindShaderVar(int bind_idx, const intermediate::Binding &binding, int frame, int prev_frame) const
{
  const auto svType = static_cast<ShaderVarType>(ShaderGlobal::get_var_type(bind_idx));
  const int frameToGet = binding.history ? prev_frame : frame;

  switch (svType)
  {
#define SHV_CASE(shVarType) case shVarType:
#define SHV_CASE_END(shVarType)                                                                                  \
  G_ASSERTF(!binding.resource.has_value(), "Binding a blob whose projected type do not match shader var type."); \
  break;
#define TAG_CASE(projType, shVarSetter)                        \
  if (binding.projectedTag == tag_for<projType>())             \
  {                                                            \
    bindBlob<projType, shVarSetter>(bind_idx, binding, frame); \
    break;                                                     \
  }
    SHV_BIND_BLOB_LIST
#undef SHV_BIND_BLOB_LIST
#undef SHV_CASE
#undef SHV_CASE_END
#undef TAG_CASE
    case SHVT_TEXTURE:
    {
      const auto texId = binding.resource ? getManagedTexView(*binding.resource, frameToGet).getTexId() : BAD_D3DRESID;
      ShaderGlobal::set_texture(bind_idx, texId);
      break;
    }
    case SHVT_BUFFER:
    {
      const auto bufId = binding.resource ? getManagedBufView(*binding.resource, frameToGet).getBufId() : BAD_D3DRESID;
      ShaderGlobal::set_buffer(bind_idx, bufId);
      break;
    }
  }
}

template <typename ProjectedType, auto bindSetter>
void NodeExecutor::bindBlob(int bind_idx, const intermediate::Binding &binding, int frame) const
{
  if (binding.resource.has_value())
  {
    // TODO: should we try and reset things to zero when unbinding?
    const auto blob = getBlobView(*binding.resource, frame);
    bindSetter(bind_idx, *static_cast<eastl::remove_reference_t<ProjectedType> *>((binding.projector)(blob.data)));
  }
}


ManagedTexView NodeExecutor::getManagedTexView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  if (res_name_id == ResNameId::Invalid || !mapping.wasResMapped(res_name_id, multi_index))
    return {};

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getManagedTexView(resIdx, frame);
}

ManagedTexView NodeExecutor::getManagedTexView(intermediate::ResourceIndex res_idx, int frame) const
{
  if (graph.resources[res_idx].isExternal())
  {
    auto tex = graph.resources[res_idx].getExternalTex();
    G_ASSERT(tex);
    return tex;
  }
  else
  {
    return resourceScheduler.getTexture(frame, res_idx);
  }
}

ManagedBufView NodeExecutor::getManagedBufView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  if (res_name_id == ResNameId::Invalid || !mapping.wasResMapped(res_name_id, multi_index))
    return {};

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getManagedBufView(resIdx, frame);
}

ManagedBufView NodeExecutor::getManagedBufView(intermediate::ResourceIndex res_idx, int frame) const
{
  if (graph.resources[res_idx].isExternal())
  {
    auto buf = graph.resources[res_idx].getExternalBuf();
    G_ASSERT(buf);
    return buf;
  }
  else
  {
    return resourceScheduler.getBuffer(frame, res_idx);
  }
}

BlobView NodeExecutor::getBlobView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  if (res_name_id == ResNameId::Invalid || !mapping.wasResMapped(res_name_id, multi_index))
    return {};

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getBlobView(resIdx, frame);
}

BlobView NodeExecutor::getBlobView(intermediate::ResourceIndex res_idx, int frame) const
{
  return resourceScheduler.getBlob(frame, res_idx);
}


} // namespace dabfg
