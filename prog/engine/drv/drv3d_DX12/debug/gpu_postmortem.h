// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include <pipeline.h>

#include <EASTL/unordered_map.h>
#include <EASTL/variant.h>

#include "trace_checkpoint.h"
#include "trace_run_status.h"
#include "trace_status.h"
#include "gpu_postmortem_null_trace.h"

#if _TARGET_PC_WIN

#if HAS_GF_AFTERMATH
#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#endif

#if HAS_GF_AFTERMATH
#include "gpu_postmortem_nvidia_aftermath.h"
#endif
#include "gpu_postmortem_dagor_trace.h"
#include "gpu_postmortem_microsoft_dred.h"
#if HAS_AMD_GPU_SERVICES
#include "gpu_postmortem_ags_trace.h"
#endif

#endif


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{

namespace gpu_postmortem
{
// Null modules that is only created when one of its matching types is active in the vendor extension module set,
// otherwise it will not be created
// Note that all invocations to this alias module will be no-ops so the vendor one that may be active has to be
// invoked to be processed. This alias module ensures that when a vendor module is used for more than only the
// vendor extended module set all its methods are not invoked multiple times.
template <typename... Ts>
struct VendorAliasModules
{
  template <typename K, typename... Is>
  bool onDeviceCreated(Is &&..., const K &vendor)
  {
    return (... || vendor.template isActiveModule<Ts>());
  }
  template <typename U, typename K>
  static bool load(const Configuration &, const Direct3D12Enviroment &, const K &vendor, U &&target)
  {
    if ((... || vendor.template isActiveModule<Ts>()))
    {
      target.template emplace<VendorAliasModules>();
      return true;
    }
    return false;
  }
};

// Module type that will never initialize
struct NeverInitModule : NullTrace
{
  template <typename T>
  static bool load(const Configuration &, const Direct3D12Enviroment &, T &&)
  {
    return false;
  }
};

// Add dummy types to make following code easier to work with
#if !HAS_GF_AFTERMATH
namespace nvidia
{
struct Aftermath : NeverInitModule
{};
} // namespace nvidia
#endif

#if !HAS_AMD_GPU_SERVICES
namespace ags
{
struct AgsTrace : NeverInitModule
{};
} // namespace ags
#endif
} // namespace gpu_postmortem

namespace detail
{
template <typename NoT, typename... Ts>
class TracerSet
{
public:
  using StorageType = eastl::variant<NoT, Ts...>;
  using DefaultType = NoT;

  template <typename... Is>
  static bool load(StorageType &store, Is &&...is)
  {
    for (auto loader : {try_load<Ts, Is...>...})
    {
      if (loader(is..., store))
      {
        return true;
      }
    }

    store = DefaultType{};
    return false;
  }

  template <typename... Is>
  static bool on_device_setup(StorageType &store, D3DDevice *device, Is &&...is)
  {
    if (eastl::visit([&](auto &tracer) { return tracer.onDeviceSetup(device, is...); }, store))
    {
      return true;
    }

    using LoaderEntry = bool (*)(Is &&..., StorageType &);
    // Note that entry 0 in the variant i NoT and so we have to adjust for that.
    static LoaderEntry loaderTable[sizeof...(Ts)] = {try_load<Ts, Is...>...};
    auto index = store.index();
    // We can imply start with index adjusted as it will index to loader of the next one in the list.
    for (; index < sizeof...(Ts); ++index)
    {
      auto loader = loaderTable[index];
      if (loader(is..., store))
      {
        if (eastl::visit([&](auto &tracer) { return tracer.onDeviceSetup(device, is...); }, store))
        {
          return true;
        }
      }
    }

    store = DefaultType{};
    return false;
  }

private:
  template <typename T, typename... Is>
  static bool try_load(Is &&...is, StorageType &store)
  {
    return T::load(is..., store);
  }
};

template <typename A, typename B>
struct ConcatVariant;

template <typename... As, typename B>
struct ConcatVariant<eastl::variant<As...>, B>
{
  using Type = eastl::variant<As..., B>;
};

template <typename T>
struct IsAliasWrapper
{
  static inline constexpr bool value = false;
};

template <typename... Ts>
struct IsAliasWrapper<gpu_postmortem::VendorAliasModules<Ts...>>
{
  static inline constexpr bool value = true;
};

template <typename... Ts>
struct CountAliasWrappers
{
  static inline constexpr uint32_t value = (... + uint32_t(IsAliasWrapper<Ts>::value));
};

template <typename... Ts>
struct ExtractAliasModule;

template <typename... Ts, typename... As>
struct ExtractAliasModule<gpu_postmortem::VendorAliasModules<As...>, Ts...>
{
  using Type = gpu_postmortem::VendorAliasModules<As...>;
};

template <typename T, typename... Ts>
struct ExtractAliasModule<T, Ts...>
{
  using Type = typename ExtractAliasModule<Ts...>::Type;
};

template <typename T>
struct ExtractAliasModule<T>
{
  using Type = gpu_postmortem::VendorAliasModules<>;
};

template <typename... Ts>
struct ExtractNonAliasModules;

template <typename... Ts, typename... As>
struct ExtractNonAliasModules<gpu_postmortem::VendorAliasModules<As...>, Ts...>
{
  using Variant = typename ExtractNonAliasModules<Ts...>::Variant;
};

template <typename T, typename... Ts>
struct ExtractNonAliasModules<T, Ts...>
{
  using Variant = typename ConcatVariant<typename ExtractNonAliasModules<Ts...>::Variant, T>::Type;
};

template <typename T>
struct ExtractNonAliasModules<T>
{
  using Variant = eastl::variant<T>;
};

/// Allows to override default values for postmortem modules when no module is loaded
template <typename T>
inline T get_post_mortem_module_default_value()
{
  if constexpr (eastl::is_same<eastl::decay_t<T>, TraceRunStatus>::value)
  {
    return TraceRunStatus::NoTraceData;
  }
  else if constexpr (eastl::is_same<eastl::decay_t<T>, TraceStatus>::value)
  {
    return TraceStatus::NotLaunched;
  }
  else if constexpr (eastl::is_same<eastl::decay_t<T>, TraceCheckpoint>::value)
  {
    return TraceCheckpoint::make_invalid();
  }
  else
  {
    return {};
  }
}

template <typename... Ts>
class PostMortemModuleContainer
{
  static_assert(CountAliasWrappers<Ts...>::value < 2, "Can only have one gpu_postmortem::VendorAliasModules, if you want alias "
                                                      "multiple vendor modules, add them to the one alias wrapper");
  using AliasModules = typename ExtractAliasModule<Ts...>::Type;
  using VariantOfNonAliasModules = typename ExtractNonAliasModules<Ts...>::Variant;
  using EmptyStateType = eastl::monostate;
  using ContainerType = eastl::variant<EmptyStateType, AliasModules, VariantOfNonAliasModules>;
  ContainerType container = EmptyStateType{};

  struct AliasLoadAdapter
  {
    ContainerType &target;

    template <typename E, typename... Ps>
    AliasModules &emplace(Ps &&...ps)
    {
      static_assert(eastl::is_same<E, AliasModules>::value, "E should be the same as AliasModules");
      return target.template emplace<AliasModules>(eastl::forward<Ps>(ps)...);
    }
  };

  struct LoadAdapter
  {
    ContainerType &target;

    template <typename E, typename... Ps>
    E &emplace(Ps &&...ps)
    {
      return eastl::get<E>(target.template emplace<VariantOfNonAliasModules>(eastl::in_place<E>, eastl::forward<Ps>(ps)...));
    }
  };

  template <typename T, typename... Is>
  static bool try_load_module(Is &&...is, ContainerType &c)
  {
    if constexpr (eastl::is_same<T, AliasModules>::value)
    {
      return T::load(is..., AliasLoadAdapter{.target = c});
    }
    else
    {
      return T::load(is..., LoadAdapter{.target = c});
    }
  }

  bool canVisit() const { return eastl::holds_alternative<VariantOfNonAliasModules>(container); }

public:
  template <typename T>
  auto invoke(T &&clb)
  {
    using ResultType = decltype(eastl::visit(clb, eastl::get<VariantOfNonAliasModules>(container)));
    if constexpr (eastl::is_same<ResultType, void>::value)
    {
      if (canVisit())
      {
        eastl::visit(clb, eastl::get<VariantOfNonAliasModules>(container));
      }
    }
    else
    {
      return canVisit() ? eastl::visit(clb, eastl::get<VariantOfNonAliasModules>(container))
                        : get_post_mortem_module_default_value<ResultType>();
    }
  }

  bool hasActiveModule() const { return !eastl::holds_alternative<EmptyStateType>(container); }

  template <typename... Is>
  bool onRuntimeLoad(Is &&...is)
  {
    for (auto moduleLoader : {try_load_module<Ts, Is...>...})
    {
      if (moduleLoader(is..., container))
      {
        return true;
      }
    }

    container = EmptyStateType{};
    return false;
  }

  template <typename... Is>
  bool onDeviceCreated(D3DDevice *device, Is &&...is)
  {
    // empty state nothing to do
    if (!hasActiveModule())
    {
      return true;
    }

    if (invoke([&](auto &module) { return module.onDeviceCreated(device, is...); }))
    {
      return true;
    }

    const bool comingFromAlias = eastl::holds_alternative<AliasModules>(container);
    uint32_t moduleIndex = 0;
    if (eastl::holds_alternative<VariantOfNonAliasModules>(container))
    {
      moduleIndex = eastl::get<VariantOfNonAliasModules>(container).index();
    }

    // reset is not strictly needed by this container, but may be required to clean up stuff before trying other modules
    reset();

    using LoaderTableEntryType = bool (*)(Is &&...is, ContainerType &c);
    static const LoaderTableEntryType loaderTable[sizeof...(Ts)] = {try_load_module<Ts, Is...>...};
    static const bool isTheAliasEntry[sizeof...(Ts)] = {eastl::is_same<Ts, AliasModules>::value...};
    // have to count index for the VariantOfNonAliasModules variant
    uint32_t nextToCompareModuleIndex = 0;
    // keeps track of we are still looking for the alias one or not
    bool skipNonAlias = comingFromAlias;
    for (uint32_t tsIndex = 0; tsIndex < sizeof...(Ts); ++tsIndex)
    {
      if (isTheAliasEntry[tsIndex])
      {
        if (skipNonAlias)
        {
          skipNonAlias = false;
          // need to update moduleIndex to the index of the last non aliasing entry so the on next entry we try to load the module
          moduleIndex = nextToCompareModuleIndex;
          continue;
        }
        if (!loaderTable[tsIndex](is..., container))
        {
          continue;
        }
      }
      else
      {
        auto moduleIndexToCompare = nextToCompareModuleIndex++;
        if (skipNonAlias)
        {
          continue;
        }
        if (moduleIndexToCompare <= moduleIndex)
        {
          continue;
        }
        if (!loaderTable[tsIndex](is..., container))
        {
          continue;
        }
      }
      if (!invoke([&](auto &module) { return module.onDeviceCreated(device, is...); }))
      {
        reset();
        continue;
      }
      return true;
    }

    return false;
  }

  // Allows E of a type that is not part of Ts
  template <typename E>
  bool isActiveModule() const
  {
    if constexpr (eastl::is_same<E, AliasModules>::value)
    {
      return eastl::holds_alternative<E>(container);
    }
    else if constexpr ((... || eastl::is_same<E, Ts>::value))
    {
      if (eastl::holds_alternative<VariantOfNonAliasModules>(container))
      {
        return eastl::holds_alternative<E>(eastl::get<VariantOfNonAliasModules>(container));
      }
    }
    return false;
  }

  void reset() { container = EmptyStateType{}; }
};

template <typename... Ts>
class PostMortemModuleContainer<gpu_postmortem::VendorAliasModules<Ts...>>
{
  bool isActive = false;

  using InvocationType = gpu_postmortem::VendorAliasModules<Ts...>;

  struct LoadAdapter
  {
    bool &isActive;
    template <typename E, typename... Ps>
    void emplace(Ps &&...)
    {
      isActive = true;
    }
  };

public:
  template <typename T>
  auto invoke(T &&clb)
  {
    using ResultType = decltype(clb(InvocationType{}));
    return get_post_mortem_module_default_value<ResultType>();
  }

  bool hasActiveModule() const { return isActive; }

  template <typename... Is>
  bool onRuntimeLoad(Is &&...is)
  {
    return InvocationType::load(eastl::forward<Is>(is)..., LoadAdapter{.isActive = isActive});
  }

  template <typename... Is>
  bool onDeviceCreated(D3DDevice *, Is &&...)
  {
    return isActive;
  }

  // Allows E to be a type other than InvocationType
  template <typename E>
  bool isActiveModule() const
  {
    return eastl::is_same<InvocationType, E>::value && hasActiveModule();
  }

  void reset() { isActive = false; }
};

template <typename T>
class PostMortemModuleContainer<T>
{
  using ContainerType = eastl::optional<T>;
  ContainerType container = eastl::nullopt;

  struct LoadAdapter
  {
    ContainerType &target;

    template <typename E, typename... Ps>
    E &emplace(Ps &&...ps)
    {
      G_STATIC_ASSERT((eastl::is_same<E, T>::value));
      return target.emplace(eastl::forward<Ps>(ps)...);
    }
  };

public:
  template <typename C>
  auto invoke(C &&clb)
  {
    using ResultType = decltype(clb(*container));
    if constexpr (eastl::is_same<ResultType, void>::value)
    {
      if (container)
      {
        clb(*container);
      }
    }
    else
    {
      return container ? clb(*container) : get_post_mortem_module_default_value<ResultType>();
    }
  }

  bool hasActiveModule() const { return static_cast<bool>(container); }

  template <typename... Is>
  bool onRuntimeLoad(Is &&...is)
  {
    return T::load(eastl::forward<Is>(is)..., LoadAdapter{.target = container});
  }

  template <typename... Is>
  bool onDeviceCreated(D3DDevice *device, Is &&...is)
  {
    if (invoke([&](auto &module) { return module.onDeviceCreated(device, is...); }))
    {
      return true;
    }

    container = eastl::nullopt;
    return false;
  }

  // Allows E to be a type other than T
  template <typename E>
  bool isActiveModule() const
  {
    return eastl::is_same<T, E>::value && hasActiveModule();
  }

  void reset() { container = eastl::nullopt; }
};
} // namespace detail

#if _TARGET_PC_WIN
class GpuPostmortem
{
  // We have one optional trace recording module
  using TraceRecorderModule = detail::PostMortemModuleContainer<gpu_postmortem::dagor::Trace>;

  // Modules of this type record meta data for page fault information, this includes GPU markers and object names
  using PageFaultRecorderModule =
    detail::PostMortemModuleContainer<gpu_postmortem::VendorAliasModules<gpu_postmortem::nvidia::Aftermath>,
      gpu_postmortem::microsoft::DeviceRemovedExtendedData>;

  // Modules of this type record extra data with vendor specific debug libraries, this includes GPU markers and object names
  using VendorExtendedDataModule = detail::PostMortemModuleContainer<gpu_postmortem::nvidia::Aftermath, gpu_postmortem::ags::AgsTrace,
    gpu_postmortem::NeverInitModule>;

  TraceRecorderModule traceRecorder;
  PageFaultRecorderModule pageFaultRecorder;
  VendorExtendedDataModule vendorExtendedRecorder;

public:
  void setup(const Configuration &config, const Direct3D12Enviroment &d3d_env)
  {
    // do vendor extended stuff first as other recorders may depend on it
    vendorExtendedRecorder.onRuntimeLoad(config, d3d_env);
    traceRecorder.onRuntimeLoad(config, d3d_env, vendorExtendedRecorder);
    pageFaultRecorder.onRuntimeLoad(config, d3d_env, vendorExtendedRecorder);
  }
  void setupDevice(D3DDevice *device, const Configuration &config, const Direct3D12Enviroment &d3d_env)
  {
    // do vendor extended stuff first as other recorders may depend on it
    vendorExtendedRecorder.onDeviceCreated(device, config, d3d_env);
    traceRecorder.onDeviceCreated(device, config, d3d_env, vendorExtendedRecorder);
    pageFaultRecorder.onDeviceCreated(device, config, d3d_env, vendorExtendedRecorder);
  }
  void teardown()
  {
    pageFaultRecorder.reset();
    traceRecorder.reset();
    vendorExtendedRecorder.reset();
  }
  void onDeviceShutdown()
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.onDeviceShutdown(); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.onDeviceShutdown(); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.onDeviceShutdown(); });
  }
  void beginCommandBuffer(D3DDevice *device, D3DGraphicsCommandList *cmd)
  {
    traceRecorder.invoke([=](auto &recorder) { recorder.beginCommandBuffer(device, cmd); });
    pageFaultRecorder.invoke([=](auto &recorder) { recorder.beginCommandBuffer(device, cmd); });
    vendorExtendedRecorder.invoke([=](auto &recorder) { recorder.beginCommandBuffer(device, cmd); });
  }
  void endCommandBuffer(D3DGraphicsCommandList *cmd)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.endCommandBuffer(cmd); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.endCommandBuffer(cmd); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.endCommandBuffer(cmd); });
  }
  void beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text, const eastl::string &full_path)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.beginEvent(cmd, text, full_path); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.beginEvent(cmd, text, full_path); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.beginEvent(cmd, text, full_path); });
  }
  void endEvent(D3DGraphicsCommandList *cmd, const eastl::string &full_path)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.endEvent(cmd, full_path); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.endEvent(cmd, full_path); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.endEvent(cmd, full_path); });
  }
  void marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.marker(cmd, text); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.marker(cmd, text); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.marker(cmd, text); });
  }
  void draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traceRecorder.invoke([&](auto &recorder) {
      recorder.draw(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance, topology);
    });
  }
  void drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traceRecorder.invoke([&](auto &recorder) {
      recorder.drawIndexed(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, index_start, vertex_base,
        first_instance, topology);
    });
  }
  void drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.drawIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer); });
  }
  void drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traceRecorder.invoke(
      [&](auto &recorder) { recorder.drawIndexedIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer); });
  }
  void dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.dispatchIndirect(debug_info, cmd, state, pipeline, buffer); });
  }
  void dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &stage,
    ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.dispatch(debug_info, cmd, stage, pipeline, x, y, z); });
  }
  void dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.dispatchMesh(debug_info, cmd, vs, ps, pipeline_base, pipeline, x, y, z); });
  }
  void dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
  {
    traceRecorder.invoke([&](auto &recorder) {
      recorder.dispatchMeshIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, args, count, max_count);
    });
  }
  void blit(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.blit(debug_info, cmd); });
  }

#if D3D_HAS_RAY_TRACING
  void dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.dispatchRays(debug_info, cmd, dispatch_parameters, rbt, rdp); });
  }

  void dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.dispatchRaysIndirect(debug_info, cmd, dispatch_parameters, rbt, rdip); });
  }
#endif

  void onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &reporter)
  {
    traceRecorder.invoke([&](auto &recorder) { recorder.onDeviceRemoved(device, reason, reporter); });
    pageFaultRecorder.invoke([&](auto &recorder) { recorder.onDeviceRemoved(device, reason, reporter); });
    vendorExtendedRecorder.invoke([&](auto &recorder) { recorder.onDeviceRemoved(device, reason, reporter); });
  }
  bool isAnyActive() const
  {
    return traceRecorder.hasActiveModule() || pageFaultRecorder.hasActiveModule() || vendorExtendedRecorder.hasActiveModule();
  }
  template <typename T>
  bool isActive() const
  {
    return traceRecorder.template isActiveModule<T>() || pageFaultRecorder.template isActiveModule<T>() ||
           vendorExtendedRecorder.template isActiveModule<T>();
  }
  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
    HLSLVendorExtensions &extensions)
  {
    return vendorExtendedRecorder.invoke(
      [&](auto &recorder) { return recorder.tryCreateDevice(adapter, uuid, minimum_feature_level, ptr, extensions); });
  }
  bool sendGPUCrashDump(const char *type, const void *data, uintptr_t size)
  {
    return vendorExtendedRecorder.invoke([&](auto &recorder) { return recorder.sendGPUCrashDump(type, data, size); });
  }

  void nameResource(ID3D12Resource *resource, eastl::string_view name)
  {
    pageFaultRecorder.invoke([=](auto &module) { module.nameResource(resource, name); });
    vendorExtendedRecorder.invoke([=](auto &module) { module.nameResource(resource, name); });
  }

  void nameResource(ID3D12Resource *resource, eastl::wstring_view name)
  {
    pageFaultRecorder.invoke([=](auto &module) { module.nameResource(resource, name); });
    vendorExtendedRecorder.invoke([=](auto &module) { module.nameResource(resource, name); });
  }

  TraceCheckpoint getTraceCheckpoint()
  {
    return traceRecorder.invoke([=](auto &module) { return module.getCheckpoint(); });
  }

  TraceRunStatus getTraceRunStatusFor(const TraceCheckpoint &cp)
  {
    return traceRecorder.invoke([=](auto &module) { return module.getTraceRunStatusFor(cp); });
  }

  TraceStatus getTraceStatusFor(const TraceCheckpoint &cp)
  {
    return traceRecorder.invoke([=](auto &module) { return module.getTraceStatusFor(cp); });
  }

  void reportTraceDataForRange(const TraceCheckpoint &from, const TraceCheckpoint &to, call_stack::Reporter &reporter)
  {
    traceRecorder.invoke([=, &from, &to, &reporter](auto &module) { module.reportTraceDataForRange(from, to, reporter); });
  }
};
#else
class GpuPostmortem
{
public:
  void setup(const Configuration &, const Direct3D12Enviroment &) {}
  void setupDevice(D3DDevice *, const Configuration &, const Direct3D12Enviroment &) {}
  void teardown() {}
  void onDeviceShutdown() {}
  void beginCommandBuffer(D3DDevice *, D3DGraphicsCommandList *) {}
  void endCommandBuffer(D3DGraphicsCommandList *) {}
  void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>, const eastl::string &) {}
  void endEvent(D3DGraphicsCommandList *, const eastl::string &) {}
  void marker(D3DGraphicsCommandList *, eastl::span<const char>) {}
  void draw(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &, const PipelineStageStateBase &,
    BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, uint32_t, D3D12_PRIMITIVE_TOPOLOGY)
  {}
  void drawIndexed(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t, int32_t, uint32_t,
    D3D12_PRIMITIVE_TOPOLOGY)
  {}
  void drawIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  void drawIndexedIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &)
  {}
  void dispatchIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &, ComputePipeline &,
    const BufferResourceReferenceAndOffset &)
  {}
  void dispatch(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &, ComputePipeline &, uint32_t,
    uint32_t, uint32_t)
  {}
  void dispatchMesh(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, uint32_t, uint32_t, uint32_t)
  {}
  void dispatchMeshIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const PipelineStageStateBase &,
    const PipelineStageStateBase &, BasePipeline &, PipelineVariant &, const BufferResourceReferenceAndOffset &,
    const BufferResourceReferenceAndOffset &, uint32_t)
  {}
  void blit(const call_stack::CommandData &, D3DGraphicsCommandList *) {}

#if D3D_HAS_RAY_TRACING
  void dispatchRays(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchParameters &)
  {}

  void dispatchRaysIndirect(const call_stack::CommandData &, D3DGraphicsCommandList *, const RayDispatchBasicParameters &,
    const ResourceBindingTable &, const RayDispatchIndirectParameters &)
  {}
#endif

  void onDeviceRemoved(D3DDevice *, HRESULT, call_stack::Reporter &) {}
  bool isAnyActive() const { return false; }
  template <typename T>
  bool isActive() const
  {
    return false;
  }
  bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; }
  bool sendGPUCrashDump(const char *, const void *, uintptr_t) { return false; }

  void nameResource(ID3D12Resource *, eastl::string_view) {}
  void nameResource(ID3D12Resource *, eastl::wstring_view) {}

  TraceCheckpoint getTraceCheckpoint() { return detail::get_post_mortem_module_default_value<TraceCheckpoint>(); }
  TraceRunStatus getTraceRunStatusFor(const TraceCheckpoint &)
  {
    return detail::get_post_mortem_module_default_value<TraceRunStatus>();
  }
  TraceStatus getTraceStatusFor(const TraceCheckpoint &) { return detail::get_post_mortem_module_default_value<TraceStatus>(); }
  void reportTraceDataForRange(const TraceCheckpoint &, const TraceCheckpoint &, call_stack::Reporter &) {}
};
#endif
} // namespace debug
} // namespace drv3d_dx12