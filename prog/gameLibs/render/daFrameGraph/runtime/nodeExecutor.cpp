// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeExecutor.h"

#include <id/idRange.h>
#include <debug/backendDebug.h>
#include <frontend/multiplexingInternal.h>
#include <backend/blobBindingHelpers.h>

#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>
#include <generic/dag_enumerate.h>

#include <math/dag_dxmath.h>
#include <math/dag_color.h>
#include <math/dag_hlsl_floatx.h>


namespace dafg
{

template <class F, class G, class H, class I, class B>
void populate_resource_provider(ResourceProvider &provider, // passed by ref to avoid reallocs
  const InternalRegistry &registry, const NameResolver &resolver, NodeNameId node_id, const F &texture_provider,
  const G &buffer_provider, const H &blob_provider, const I &is_available, const B &is_banned)
{
  provider.providedResources.reserve(registry.nodes[node_id].resourceRequests.size());
  provider.providedHistoryResources.reserve(registry.nodes[node_id].historyResourceReadRequests.size());

  auto processRes = [&](ResNameId unresolvedResNameId, bool history, bool request_was_optional) {
    G_UNUSED(request_was_optional);

    auto setRes = [unresolvedResNameId, history, &provider](auto resource) {
      auto &storage = history ? provider.providedHistoryResources : provider.providedResources;
      storage[unresolvedResNameId] = resource;
    };

    ResNameId resId = resolver.resolve(unresolvedResNameId);

    G_ASSERT(registry.resources.isMapped(resId)); // Sanity check, nodeTracker makes sure this doesn't happen

    if (!is_available(resId))
    {
      G_ASSERT(request_was_optional); // Sanity check
      setRes(MissingOptionalResource{});
      return;
    }

    // The user is not allowed to access banned resources directly, e.g. the back buffer
    if (is_banned(resId))
      return;

    switch (registry.resources[resId].type)
    {
      case ResourceType::Texture: setRes(texture_provider(history, resId)); break;
      case ResourceType::Buffer: setRes(buffer_provider(history, resId)); break;
      case ResourceType::Blob: setRes(blob_provider(history, resId)); break;
      case ResourceType::Invalid: G_ASSERT_FAIL("Impossible situation!"); break;
    }
  };

  for (const auto &[resId, req] : registry.nodes[node_id].resourceRequests)
    processRes(resId, false, req.optional);
  for (const auto &[resId, req] : registry.nodes[node_id].historyResourceReadRequests)
    processRes(resId, true, req.optional);
}


struct CallstackData
{
  NodeExecutor *executor;
  NodeNameId nodeId;
  multiplexing::Index multiplexingIndex;
};

static constexpr size_t CALLSTACK_DATA_SIZE_IN_POINTERS = (sizeof(CallstackData) + sizeof(void *) - 1) / sizeof(void *);

stackhelp::ext::CallStackResolverCallbackAndSizePair NodeExecutor::captureCallstackData(stackhelp::CallStackInfo stack, void *context)
{
  auto *ctx = static_cast<CallstackData *>(context);

  if (stack.stackSize < CALLSTACK_DATA_SIZE_IN_POINTERS)
    return {};

  memcpy(&stack.stack[0], ctx, sizeof(CallstackData));

  return {&NodeExecutor::resolveCallStackData, CALLSTACK_DATA_SIZE_IN_POINTERS};
}

unsigned NodeExecutor::resolveCallStackData(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack)
{
  if (stack.stackSize < CALLSTACK_DATA_SIZE_IN_POINTERS)
    return 0;

  CallstackData data;
  memcpy(&data, stack.stack, sizeof(data));

  unsigned writtenChars = 0;
  {
    auto ret = snprintf(buf, max_buf, "While executing node %s with sub sample %d, super sample %d, viewport %d and subCamera %d\n",
      data.executor->registry.knownNames.getName(data.nodeId), data.multiplexingIndex.subSample, data.multiplexingIndex.superSample,
      data.multiplexingIndex.viewport, data.multiplexingIndex.subCamera);
    if (ret > 0)
      writtenChars += ret;
  }

  return eastl::min(writtenChars, max_buf);
}

#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
static eastl::fixed_string<char, 128, false, framemem_allocator> generate_node_name_for_profiling(const char *node_name,
  multiplexing::Mode node_multiplex_mode, multiplexing::Extents multiplexing_extents, const multiplexing::Index &multi_idx)
{
  const char *namePtr = node_name;
  if (namePtr[0] == '/')
    namePtr++;

  static char superSamplingTagBase[] = " | superSample:XX";
  static char subSamplingTagBase[] = " | subSample:XX";
  static char viewportTagBase[] = " | viewport:XX";
  static char camcamTagBase[] = " | camcam:XX";

  auto getMultiplexTagName = [node_multiplex_mode](char *tag, const size_t N, multiplexing::Mode check_mode,
                               const uint32_t multiplex_extent, uint32_t multiplex_idx) -> const char * {
    G_ASSERT(multiplex_idx <= 99);
    const bool hasMultiplexMode = (node_multiplex_mode & check_mode) != multiplexing::Mode::None;
    const bool multiplexModeActive = multiplex_extent > 1;

    if (hasMultiplexMode && multiplexModeActive)
    {
      if (multiplex_idx > 9)
      {
        tag[N - 1] = '\0';
        tag[N - 2] = '0' + multiplex_idx % 10;
        tag[N - 3] = '0' + multiplex_idx / 10;
      }
      else
      {
        tag[N - 2] = '\0';
        tag[N - 3] = '0' + multiplex_idx;
      }

      return tag;
    }

    return "";
  };

  const char *superSamplingTag = getMultiplexTagName(superSamplingTagBase, eastl::size(superSamplingTagBase),
    multiplexing::Mode::SuperSampling, multiplexing_extents.superSamples, multi_idx.superSample);

  const char *subSamplingTag = getMultiplexTagName(subSamplingTagBase, eastl::size(subSamplingTagBase),
    multiplexing::Mode::SubSampling, multiplexing_extents.subSamples, multi_idx.subSample);

  const char *viewportTag = getMultiplexTagName(viewportTagBase, eastl::size(viewportTagBase), multiplexing::Mode::Viewport,
    multiplexing_extents.viewports, multi_idx.viewport);

  const char *camcamTag = getMultiplexTagName(camcamTagBase, eastl::size(camcamTagBase), multiplexing::Mode::CameraInCamera,
    multiplexing_extents.subCameras, multi_idx.subCamera);

  using TmpString = eastl::fixed_string<char, 128, false, framemem_allocator>;
  return TmpString(TmpString::CtorSprintf{}, "%s%s%s%s%s", namePtr, camcamTag, viewportTag, subSamplingTag, superSamplingTag);
}
#endif


void NodeExecutor::execute(int prev_frame, int curr_frame, multiplexing::Extents multiplexing_extents,
  const BarrierScheduler::FrameEventsRef &events, const sd::NodeStateDeltas &state_deltas)
{
  // We permit changing the concrete instance of Texture used as an external
  // resources on a per-frame basis. This may be useful for double-buffering
  // things like the swapchain
  gatherExternalResources(multiplexing_extents);
  validation_of_external_resources_duplication(externalResources, graph.resourceNames);

  currentlyProvidedResources.resolutions.resize(registry.knownNames.nameCount<AutoResTypeNameId>());
  for (auto [unresolvedId, resolution] : currentlyProvidedResources.resolutions.enumerate())
    if (auto resolvedId = nameResolver.resolve(unresolvedId); resolvedId != AutoResTypeNameId::Invalid)
    {
      eastl::visit(
        [&res = resolution](const auto &values) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
            res = values.dynamicResolution;
        },
        registry.autoResTypes[resolvedId].values);
    }

  const auto historyMultiplexingExtents = extents_for_node(registry.defaultHistoryMultiplexingMode, multiplexing_extents);
  for (auto i : IdRange<intermediate::NodeIndex>(graph.nodes.size()))
  {
    const intermediate::Node &irNode = graph.nodes[i];
    if (!irNode.frontendNode)
    {
      applyStateBeforeEvents(state_deltas[i], curr_frame, prev_frame);
      processEvents(events[i], curr_frame);
      applyState(state_deltas[i], curr_frame, prev_frame);
      continue;
    }

    const auto nodeId = *irNode.frontendNode;
    const multiplexing::Index multiIdx = multiplexing_index_from_ir(irNode.multiplexingIndex, multiplexing_extents);
#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
    const char *namePtr = registry.knownNames.getName(nodeId);
    const multiplexing::Mode nodeMultiplexMode = registry.nodes[nodeId].multiplexingMode.value_or(multiplexing::Mode::None);

    const auto nodeName = generate_node_name_for_profiling(namePtr, nodeMultiplexMode, multiplexing_extents, multiIdx);

    TIME_D3D_PROFILE_NAME(FramegraphNode, nodeName.c_str());
    DA_PROFILE_TAG(Viewport, "%d", multiIdx.viewport);
    DA_PROFILE_TAG(SubSample, "%d", multiIdx.subSample);
    DA_PROFILE_TAG(SuperSample, "%d", multiIdx.superSample);
    DA_PROFILE_TAG(SubCamera, "%d", multiIdx.subCamera);
#endif

    applyStateBeforeEvents(state_deltas[i], curr_frame, prev_frame);
    processEvents(events[i], curr_frame);

    const intermediate::MultiplexingIndex historyMultiIdx = multiplexing_index_to_ir(
      clamp(multiplexing_index_from_ir(irNode.multiplexingIndex, multiplexing_extents), historyMultiplexingExtents),
      multiplexing_extents);

    populate_resource_provider(
      currentlyProvidedResources, registry, nameResolver, nodeId,
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex, historyMultiIdx](bool history,
        ResNameId res_id) -> ManagedTexView {
        return getManagedTexView(res_id, history ? prev_frame : curr_frame, history ? historyMultiIdx : multiIndex);
      },
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex, historyMultiIdx](bool history,
        ResNameId res_id) -> ManagedBufView {
        return getManagedBufView(res_id, history ? prev_frame : curr_frame, history ? historyMultiIdx : multiIndex);
      },
      [this, prev_frame, curr_frame, multiIndex = irNode.multiplexingIndex, historyMultiIdx](bool history, ResNameId res_id)
        -> BlobView { return getBlobView(res_id, history ? prev_frame : curr_frame, history ? historyMultiIdx : multiIndex); },
      [this, multiIndex = irNode.multiplexingIndex](ResNameId res_id) {
        G_ASSERT(res_id != ResNameId::Invalid);
        return mapping.wasResMapped(res_id, multiIndex);
      },
      [this, multiIndex = irNode.multiplexingIndex](ResNameId res_id) {
        // Checking against the registry won't work due to renames
        const auto idx = mapping.mapRes(res_id, multiIndex);
        return graph.resources[idx].isDriverDeferredTexture();
      });

    validation_set_current_node(registry, nodeId);
    {
      CallstackData context{this, nodeId, multiIdx};
      stackhelp::ext::ScopedCallStackContext callstackCtxScope{&captureCallstackData, &context};

      applyState(state_deltas[i], curr_frame, prev_frame);
      if (const auto &node = registry.nodes[nodeId]; node.enabled && node.sideEffect != SideEffects::None)
      {
        {
          if (auto &exec = registry.nodes[nodeId].execute)
            exec(multiIdx);
          else
            logerr("daFG: Somehow, a node with an empty execution callback was "
                   "attempted to be executed. This is a bug in framegraph!");
        }
        validate_global_state(registry, nodeId);
      }
    }
    validation_set_current_node(registry, NodeNameId::Invalid);

    // Clean up resource references inside the provider, just in case
    currentlyProvidedResources.clear();
  }
  applyStateBeforeEvents(state_deltas.back(), curr_frame, prev_frame);
  processEvents(events.back(), curr_frame);
  applyState(state_deltas.back(), curr_frame, prev_frame);
}

void NodeExecutor::gatherExternalResources(multiplexing::Extents extents)
{
  externalResources.clear();
  externalResources.resize(graph.resources.size());

  for (auto [resIdx, res] : graph.resources.enumerate())
  {
    if (!res.isExternal())
      continue;

    const ResNameId resNameId = res.frontendResources.front();
    const ExternalResourceProvider &provider = eastl::get<ExternalResourceProvider>(registry.resources[resNameId].creationInfo);

    const ExternalResource extRes = provider(multiplexing_index_from_ir(res.multiplexingIndex, extents));
    static_assert(eastl::variant_size_v<decltype(extRes)> == 2); // paranoid check
    if (auto *tex = eastl::get_if<ManagedTexView>(&extRes))
    {
      G_ASSERTF(*tex, "daFG: External texture '%s' was not provided by it's registering node!",
        registry.knownNames.getName(resNameId));

#if DAGOR_DBGLEVEL > 0
      if (*tex)
      {
        TextureInfo actualInfo;
        tex->getBaseTex()->getinfo(actualInfo);
        TextureInfo expectedInfo = eastl::get<TextureInfo>(res.asExternal().info);
        // TODO: should we require resolutions to match too?
        G_ASSERTF(actualInfo.cflg == expectedInfo.cflg,
          "daFG: External texture '%s' changed it's properties "
          "mid-execution! This is not allowed!",
          registry.knownNames.getName(resNameId));
      }
#endif
    }
    else if (auto *buf = eastl::get_if<ManagedBufView>(&extRes))
    {
      G_ASSERTF(*buf, "daFG: External buffer '%s' was not provided by it's registering node!", registry.knownNames.getName(resNameId));

#if DAGOR_DBGLEVEL > 0
      if (*buf)
      {
        intermediate::BufferInfo actualInfo;
        actualInfo.flags = buf->getBuf()->getFlags();
        auto expectedInfo = eastl::get<intermediate::BufferInfo>(res.asExternal().info);
        G_ASSERTF(actualInfo.flags == expectedInfo.flags,
          "daFG: External buffer '%s' changed it's properties "
          "mid-execution! This is not allowed!",
          registry.knownNames.getName(resNameId));
      }
#endif
    }

    externalResources[resIdx] = extRes;
  }
}

void NodeExecutor::processEvents(BarrierScheduler::NodeEventsRef events, int frame) const
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

    // No barriers on the back buffer
    if (iRes.isDriverDeferredTexture())
      continue;

    switch (resType)
    {
      case ResourceType::Texture:
      {
        const ManagedTexView tex = getManagedTexView(evt.resource, evt.frameResourceProducedOn);

        G_ASSERT_CONTINUE(tex);

        if (auto *data = eastl::get_if<BarrierScheduler::Event::Activation>(&evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          ResourceClearValue clearValue{};
          if (auto value = eastl::get_if<ResourceClearValue>(&data->clearValue))
            clearValue = *value;
          else if (auto dynValue = eastl::get_if<intermediate::DynamicParameter>(&data->clearValue))
            clearValue = getDynamicParameter<ResourceClearValue>(*dynValue, frame);
          d3d::activate_texture(tex.getBaseTex(), data->action, clearValue);
        }
        else if (auto *data = eastl::get_if<BarrierScheduler::Event::Barrier>(&evt.data))
        {
          if (!ignoreBarrier(data->barrier))
            d3d::resource_barrier({tex.getBaseTex(), data->barrier, 0, 0});
        }
        else if (eastl::holds_alternative<BarrierScheduler::Event::Deactivation>(evt.data))
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
        const ManagedBufView buf = getManagedBufView(evt.resource, evt.frameResourceProducedOn);

        G_ASSERT_CONTINUE(buf);

        if (auto *data = eastl::get_if<BarrierScheduler::Event::Activation>(&evt.data))
        {
          G_ASSERT(!iRes.isExternal());
          d3d::activate_buffer(buf.getBuf(), data->action);
        }
        else if (auto *data = eastl::get_if<BarrierScheduler::Event::Barrier>(&evt.data))
        {
          if (!ignoreBarrier(data->barrier))
            d3d::resource_barrier({buf.getBuf(), data->barrier});
        }
        else if (eastl::holds_alternative<BarrierScheduler::Event::Deactivation>(evt.data))
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

        if (auto *data = eastl::get_if<BarrierScheduler::Event::CpuActivation>(&evt.data))
          data->func(blobView.data);
        else if (auto *data = eastl::get_if<BarrierScheduler::Event::CpuDeactivation>(&evt.data))
          data->func(blobView.data);
        else
          G_ASSERTF(false, "Unexpected event type!");
      }
      break;

      case ResourceType::Invalid: G_ASSERTF_BREAK(false, "Encountered an event for an invalid resource!")
    }
  }
}

inline void resetVariableRateShading()
{
  d3d::set_variable_rate_shading_texture(nullptr);
  d3d::set_variable_rate_shading(1, 1, VariableRateShadingCombiner::VRS_PASSTHROUGH, VariableRateShadingCombiner::VRS_PASSTHROUGH);
}

inline void setVariableRateTexture(BaseTexture *tex)
{
  d3d::set_variable_rate_shading_texture(tex);
  d3d::set_variable_rate_shading(1, 1, VariableRateShadingCombiner::VRS_PASSTHROUGH, VariableRateShadingCombiner::VRS_OVERRIDE);
}

void NodeExecutor::applyStateBeforeEvents(const sd::NodeStateDelta &state, int, int) const
{
  // Gotta close render passes before doing barriers, duh.
  if (state.pass)
  {
    if (auto passChange = eastl::get_if<sd::PassChange>(&*state.pass))
    {
      if (eastl::holds_alternative<sd::FinishRenderPass>(passChange->endAction))
      {
        d3d::end_render_pass();
        if (shouldSwitchVrsTex())
          resetVariableRateShading();
      }
      else if (eastl::holds_alternative<sd::LegacyBackbufferPass>(passChange->endAction))
      {
        d3d::set_render_target({}, DepthAccess::RW, {});
        if (shouldSwitchVrsTex())
          resetVariableRateShading();
      }
    }
  }
}

void NodeExecutor::applyState(const sd::NodeStateDelta &state, int frame, int prev_frame) const
{
  if (state.asyncPipelines)
  {
    if (*state.asyncPipelines)
      d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN, nullptr, (void *)-1);
    else
      d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END);
  }

  if (externalState.wireframeModeEnabled && state.wire)
    d3d::setwire(*state.wire);

  if (state.vrs)
    d3d::set_variable_rate_shading(state.vrs->rateX, state.vrs->rateY, state.vrs->vertexCombiner, state.vrs->pixelCombiner);

  if (state.pass)
  {
    if (eastl::holds_alternative<sd::NextSubpass>(*state.pass))
    {
      d3d::next_subpass();
    }
    else if (auto passChange = eastl::get_if<sd::PassChange>(&*state.pass))
    {
      if (auto begin = eastl::get_if<sd::BeginRenderPass>(&passChange->beginAction))
      {
        if (shouldSwitchVrsTex() && begin->vrsRateAttachment.has_value())
        {
          const auto &att = *begin->vrsRateAttachment;
          auto tex = getManagedTexView(att.res, frame).getBaseTex();
          setVariableRateTexture(tex);
        }

        dag::Vector<RenderPassTarget, framemem_allocator> targets;
        for (const auto &att : begin->attachments)
        {
          ResourceClearValue clearValue{};
          if (auto value = eastl::get_if<ResourceClearValue>(&att.clearValue))
            clearValue = *value;
          else if (auto dynValue = eastl::get_if<intermediate::DynamicParameter>(&att.clearValue))
            clearValue = getDynamicParameter<ResourceClearValue>(*dynValue, frame);
          else
            G_ASSERT_FAIL("Impossible situation!");

          targets.push_back(
            RenderPassTarget{RenderTarget{getManagedTexView(att.res, frame).getBaseTex(), att.mipLevel, att.layer}, clearValue});
        }

        // TODO: this is a dirty hack, we need to upgrade API to properly tackle this
        TextureInfo info;
        targets.front().resource.tex->getinfo(info);
        d3d::begin_render_pass(begin->rp, {0, 0, info.w, info.h, 0, 1}, targets.data());
      }
      else if (eastl::holds_alternative<sd::LegacyBackbufferPass>(passChange->beginAction))
      {
        if (shouldSwitchVrsTex() && begin->vrsRateAttachment.has_value())
        {
          const auto &att = *begin->vrsRateAttachment;
          auto tex = getManagedTexView(att.res, frame).getBaseTex();
          setVariableRateTexture(tex);
        }
        // Despite the name, this sets the backbuffer as the only MRT
        d3d::set_render_target();
      }
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
  for (auto [stream, vertexSource] : enumerate(state.vertexSources))
    if (vertexSource)
    {
      Sbuffer *vertexBuffer =
        vertexSource->buffer.has_value() ? getManagedBufView(vertexSource->buffer.value(), frame).getBuf() : nullptr;
      d3d::setvsrc(stream, vertexBuffer, vertexSource->stride);
    }
  if (state.indexSource)
  {
    Sbuffer *indexBuffer =
      state.indexSource->buffer.has_value() ? getManagedBufView(state.indexSource->buffer.value(), frame).getBuf() : nullptr;
    d3d::setind(indexBuffer);
  }
}

static void bindViewSetter(int, const TMatrix4 &data) { d3d::settm(TM_VIEW, &data); };
static void bindViewSetter(int, const TMatrix &data) { d3d::settm(TM_VIEW, data); };
static void bindProjSetter(int, const TMatrix4 &data) { d3d::settm(TM_PROJ, &data); };

void NodeExecutor::applyBindings(const sd::BindingsMap &bindings, int frame, int prev_frame) const
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
  const int rawSvType = ShaderGlobal::get_var_type(bind_idx);
  if (rawSvType < 0)
    return;

  const auto svType = static_cast<ShaderVarType>(rawSvType);
  const int frameToGet = binding.history ? prev_frame : frame;

  switch (svType)
  {
#define SHV_CASE(shVarType) case shVarType:
#define SHV_CASE_END(shVarType)                                                                                  \
  G_ASSERTF(!binding.resource.has_value(), "Binding a blob whose projected type do not match shader var type."); \
  break;
#define TAG_CASE(projType, shVarSetter, shVarGetter)                \
  if (binding.projectedTag == tag_for<projType>())                  \
  {                                                                 \
    bindBlob<projType, shVarSetter>(bind_idx, binding, frameToGet); \
    break;                                                          \
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
    case SHVT_TLAS: logerr("daFG: set_tlas is not implemented yet"); break;
    default: logerr("daFG: Invalid shader var type '%d'!", svType); break;
  }
}

template <typename ProjectedType, auto bindSetter>
void NodeExecutor::bindBlob(int bind_idx, const intermediate::Binding &binding, int frame) const
{
  if (binding.resource.has_value())
  {
    // TODO: should we try and reset things to zero when unbinding?
    const auto blob = getBlobView(*binding.resource, frame);
    bindSetter(bind_idx, *static_cast<const eastl::remove_reference_t<ProjectedType> *>((binding.projector)(blob.data)));
  }
}

ManagedTexView NodeExecutor::getManagedTexView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  G_ASSERT_RETURN(res_name_id != ResNameId::Invalid && mapping.wasResMapped(res_name_id, multi_index), {});

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getManagedTexView(resIdx, frame);
}

ManagedTexView NodeExecutor::getManagedTexView(intermediate::ResourceIndex res_idx, int frame) const
{
  if (graph.resources[res_idx].isExternal())
    return eastl::get<ManagedTexView>(*externalResources[res_idx]);
  else
    return resourceScheduler.getTexture(frame, res_idx);
}

ManagedBufView NodeExecutor::getManagedBufView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  G_ASSERT_RETURN(res_name_id != ResNameId::Invalid && mapping.wasResMapped(res_name_id, multi_index), {});

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getManagedBufView(resIdx, frame);
}

ManagedBufView NodeExecutor::getManagedBufView(intermediate::ResourceIndex res_idx, int frame) const
{
  if (graph.resources[res_idx].isExternal())
    return eastl::get<ManagedBufView>(*externalResources[res_idx]);
  else
    return resourceScheduler.getBuffer(frame, res_idx);
}

BlobView NodeExecutor::getBlobView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const
{
  G_ASSERT_RETURN(res_name_id != ResNameId::Invalid && mapping.wasResMapped(res_name_id, multi_index), {});

  const intermediate::ResourceIndex resIdx = mapping.mapRes(res_name_id, multi_index);

  return getBlobView(resIdx, frame);
}

BlobView NodeExecutor::getBlobView(intermediate::ResourceIndex res_idx, int frame) const
{
  return resourceScheduler.getBlob(frame, res_idx);
}

template <class T>
const T &NodeExecutor::getDynamicParameter(const intermediate::DynamicParameter &param, int frame) const
{
  G_ASSERT(param.projectedTag == tag_for<T>()); // Paranoic check
  return *static_cast<const T *>(param.projector(getBlobView(param.resource, frame).data));
}

bool NodeExecutor::shouldSwitchVrsTex() const
{
  return externalState.vrsMaskEnabled && d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
}

} // namespace dafg
