#pragma once

#include "free_list_utils.h"

#if _TARGET_XBOX
#define REPORT_HEAP_INFO 1
#endif

#if REPORT_HEAP_INFO
#define HEAP_LOG(...) debug(__VA_ARGS__)
#else
#define HEAP_LOG(...)
#endif

#include "resource_memory_heap_basic_components.h"
#include "resource_memory_heap_object_components.h"
#include "resource_memory_heap_descriptor_components.h"
#include "resource_memory_heap_heap_components.h"

#if DX12_USE_ESRAM
#include "resource_memory_heap_esram_components_xbox.h"
#else
namespace drv3d_dx12
{
namespace resource_manager
{
using ESRamPageMappingProvider = ResourceMemoryHeapProvider;
} // namespace resource_manager
} // namespace drv3d_dx12
#endif

#include "resource_memory_heap_host_shared_components.h"
#include "resource_memory_heap_buffer_components.h"
#include "resource_memory_heap_rtx_components.h"


namespace drv3d_dx12
{
struct ScratchBuffer
{
  ID3D12Resource *buffer = nullptr;
  size_t offset = 0;
};
namespace resource_manager
{
class TextureImageFactory : public RaytraceAccelerationStructureObjectProvider
{
  using BaseType = RaytraceAccelerationStructureObjectProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<Image *> deletedImages;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    destroyTextures({begin(data.deletedImages), end(data.deletedImages)});
    data.deletedImages.clear();
    BaseType::completeFrameExecution(info, data);
  }

  void freeView(const ImageViewInfo &view);

public:
  ImageCreateResult createTexture(DXGIAdapter *adapter, ID3D12Device *device, const ImageInfo &ii, const char *name);
  Image *adoptTexture(ID3D12Resource *texture, const char *name);

  void destroyTextureOnFrameCompletion(Image *texture)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.deletedImages.push_back(texture); });
  }

protected:
  void destroyTextures(eastl::span<Image *> textures);
};

class AliasHeapProvider : public TextureImageFactory
{
  using BaseType = TextureImageFactory;

  struct AliasHeap
  {
    enum class Flag
    {
      AUTO_FREE,
      COUNT
    };
    ResourceMemory memory;
    eastl::vector<Image *> images;
    eastl::vector<BufferGlobalId> buffers;
    TypedBitSet<Flag> flags;
#if _TARGET_XBOX
    HANDLE heapRegHandle = nullptr;
#endif

    explicit operator bool() const { return static_cast<bool>(memory); }

    void reset() { memory = {}; }

    bool shouldBeFreed() const { return flags.test(Flag::AUTO_FREE) && images.empty() && buffers.empty(); }
  };

  static D3D12_RESOURCE_DESC as_desc(const BasicTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const TextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const VolTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ArrayTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const CubeTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ArrayCubeTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const BufferResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ResourceDescription &desc);
  static DeviceMemoryClass get_memory_class(const ResourceDescription &desc);

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<::ResourceHeap *> deletedResourceHeaps;
  };

  ContainerMutexWrapper<eastl::vector<AliasHeap>, OSSpinlock> aliasHeaps;

  uint32_t adoptMemoryAsAliasHeap(ResourceMemory memory);

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    bool needsAutoFreeHandling = detachTextures({begin(data.deletedImages), end(data.deletedImages)});

    for (auto &&freeInfo : data.deletedBuffers)
    {
      needsAutoFreeHandling = needsAutoFreeHandling || detachBuffer(freeInfo.buffer);
    }

    BaseType::completeFrameExecution(info, data);
    for (auto heap : data.deletedResourceHeaps)
    {
      freeUserHeap(info.device, heap);
    }
    data.deletedResourceHeaps.clear();
    if (needsAutoFreeHandling)
    {
      processAutoFree();
    }
  }

public:
  ::ResourceHeap *newUserHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeapGroup *group, size_t size,
    ResourceHeapCreateFlags flags);
  // assumes mutex is locked
  void freeUserHeap(ID3D12Device *device, ::ResourceHeap *ptr);
  ResourceAllocationProperties getResourceAllocationProperties(ID3D12Device *device, const ResourceDescription &desc);
  ImageCreateResult placeTextureInHeap(ID3D12Device *device, ::ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
    const ResourceAllocationProperties &alloc_info, const char *name);
  ResourceHeapGroupProperties getResourceHeapGroupProperties(::ResourceHeapGroup *heap_group);

  ResourceMemory getUserHeapMemory(::ResourceHeap *heap);

protected:
  bool detachTextures(eastl::span<Image *> textures);
  void processAutoFree();

public:
  BufferState placeBufferInHeap(ID3D12Device *device, ::ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
    const ResourceAllocationProperties &alloc_info, const char *name);
  bool detachBuffer(const BufferState &buf);

  ImageCreateResult aliasTexture(ID3D12Device *device, const ImageInfo &ii, Image *base, const char *name);

  void freeUserHeapOnFrameCompletion(::ResourceHeap *ptr)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.deletedResourceHeaps.push_back(ptr); });
  }
};

class ScratchBufferProvider : public AliasHeapProvider
{
  using BaseType = AliasHeapProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<BasicBuffer> deletedScratchBuffers;
  };

  struct TempScratchBufferState
  {
    BasicBuffer buffer = {};
    size_t allocatedSpace = 0;

    bool ensureSize(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment, ScratchBufferProvider *heap)
    {
      auto spaceNeeded = align_value(allocatedSpace, alignment) + size;
      if (spaceNeeded <= buffer.bufferMemory.size())
      {
        return true;
      }
      // default to times 2 so we stop to realloc at some point
      auto nextSize = max<size_t>(scartch_buffer_min_size, buffer.bufferMemory.size() * 2);
      // out of space
      if (buffer)
      {
        heap->accessRecodingPendingFrameCompletion<ScratchBufferProvider::PendingForCompletedFrameData>(
          [&](auto &data) { data.deletedScratchBuffers.push_back(eastl::move(buffer)); });
      }

      while (nextSize < size)
      {
        nextSize += nextSize;
      }

      nextSize = align_value<size_t>(nextSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

      D3D12_RESOURCE_DESC desc{};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = nextSize;
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo{};
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
        D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
      auto memory = heap->allocate(adapter, device, memoryProperties, allocInfo, {});

      auto errorCode = buffer.create(device, desc, memory, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, false);
      if (is_oom_error_code(errorCode))
      {
        heap->reportOOMInformation();
      }

      allocatedSpace = 0;
      if (DX12_CHECK_OK(errorCode))
      {
        heap->recordScratchBufferAllocated(desc.Width);
        heap->updateMemoryRangeUse(memory, ScratchBufferReference{buffer.buffer.Get()});
      }
      else
      {
        heap->free(memory);
      }

      return DX12_CHECK_OK(errorCode);
    }

    ScratchBuffer getSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment, ScratchBufferProvider *heap)
    {
      ScratchBuffer result{};
      ensureSize(adapter, device, size, alignment, heap);
      if (buffer)
      {
        result.buffer = buffer.buffer.Get();
        result.offset = align_value(allocatedSpace, alignment);
        // keep allocatedSpace as it is only used for one operation and can be reused on the next
      }
      return result;
    }

    ScratchBuffer getPersistentSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment,
      ScratchBufferProvider *heap)
    {
      auto space = getSpace(adapter, device, size, alignment, heap);
      // alter allocatedSpace to turn this transient space into persistent
      allocatedSpace = space.offset + size;
      return space;
    }

    void reset(ScratchBufferProvider *heap)
    {
      if (buffer)
      {
        buffer.reset(heap);
      }
    }
  };
  ContainerMutexWrapper<TempScratchBufferState, OSSpinlock> tempScratchBufferState;

  static constexpr size_t scartch_buffer_min_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

protected:
  void shutdown()
  {
    tempScratchBufferState.access()->reset(this);
    BaseType::shutdown();
  }

  void preRecovery()
  {
    tempScratchBufferState.access()->reset(this);
    BaseType::preRecovery();
  }

public:
  // Memory is used for one operation an can be reused by the next (for inter resource copy for
  // example)
  ScratchBuffer getTempScratchBufferSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
  {
    auto result = tempScratchBufferState.access()->getSpace(adapter, device, size, alignment, this);
    recordScratchBufferTempUse(size);
    return result;
  }
  // Memory can be used anytime by multiple distinct operations until the frame has ended.
  ScratchBuffer getPersistentScratchBufferSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
  {
    auto result = tempScratchBufferState.access()->getPersistentSpace(adapter, device, size, alignment, this);
    recordScratchBufferPersistentUse(size);
    return result;
  }
  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    for (auto &buffer : data.deletedScratchBuffers)
    {
      recordScratchBufferFreed(buffer.bufferMemory.size());
      buffer.reset(this);
    }
    data.deletedScratchBuffers.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    BaseType::completeFrameRecording(info);
    tempScratchBufferState.access()->allocatedSpace = 0;
  }
};

class SamplerDescriptorProvider : public ScratchBufferProvider
{
  using BaseType = ScratchBufferProvider;

  struct SamplerInfo
  {
    D3D12_CPU_DESCRIPTOR_HANDLE sampler;
    SamplerState state;
  };

  struct State
  {
    eastl::vector<SamplerInfo> samplers;
    DescriptorHeap<SamplerStagingPolicy> descriptors;

    eastl::vector<SamplerInfo>::const_iterator get(ID3D12Device *device, SamplerState state)
    {
      auto ref = eastl::find_if(begin(samplers), end(samplers), [state](auto &info) { return state == info.state; });
      if (ref == end(samplers))
      {
        SamplerInfo sampler{descriptors.allocate(device), state};

        D3D12_SAMPLER_DESC desc = state.asDesc();
        device->CreateSampler(&desc, sampler.sampler);

        ref = samplers.insert(ref, sampler);
      }
      return ref;
    }
  };
  ContainerMutexWrapper<State, OSSpinlock> samplers;

  template <typename T>
  void accessSampler(d3d::SamplerHandle handle, T clb)
  {
    auto index = uint64_t(handle) - 1;
    auto access = samplers.access();
    clb(access->samplers[index]);
  }

protected:
  template <typename T>
  void visitSamplerObjects(T clb)
  {
    auto access = samplers.access();
    for (auto &info : access->samplers)
    {
      auto handle = uint64_t((&info - access->samplers.data()) + 1);
      clb(handle, const_cast<const SamplerInfo &>(info));
    }
  }
  void setup(const SetupInfo &setup)
  {
    BaseType::setup(setup);

    auto access = samplers.access();
    access->descriptors.init(setup.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
    // if we are restoring try to restore all samplers
    for (auto &info : access->samplers)
    {
      D3D12_SAMPLER_DESC desc = info.state.asDesc();
      info.sampler = access->descriptors.allocate(setup.device);
      setup.device->CreateSampler(&desc, info.sampler);
    }
  }

  void shutdown()
  {
    BaseType::shutdown();

    auto access = samplers.access();
    access->samplers.clear();
    access->descriptors.shutdown();
  }

  void preRecovery()
  {
    BaseType::preRecovery();

    auto access = samplers.access();
    access->descriptors.shutdown();
    // keep sampler list as we can restore it later
  }

  d3d::SamplerHandle createSampler(ID3D12Device *device, SamplerState state)
  {
    state.normalizeSelf();

    auto access = samplers.access();
    auto ref = access->get(device, state);
    uint64_t index = ref - begin(access->samplers) + 1;
    return d3d::SamplerHandle(index);
  }
  D3D12_CPU_DESCRIPTOR_HANDLE getSampler(d3d::SamplerHandle handle)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{};
    accessSampler(handle, [&descriptor](auto &info) { descriptor = info.sampler; });
    return descriptor;
  }
  D3D12_CPU_DESCRIPTOR_HANDLE getSampler(ID3D12Device *device, SamplerState state)
  {
    state.normalizeSelf();

    auto access = samplers.access();
    auto ref = access->get(device, state);
    return ref->sampler;
  }

  void deleteSampler(d3d::SamplerHandle handle) { G_UNUSED(handle); }
};

class FrameFinalizer : public SamplerDescriptorProvider
{
  using BaseType = SamplerDescriptorProvider;
  PendingForCompletedFrameData finalizerData[FRAME_FRAME_BACKLOG_LENGTH]{};

protected:
  template <typename T>
  void visitAllFrameFinilazierData(T &visitor)
  {
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    for (auto &f : finalizerData)
    {
      visitor(f);
    }
  }
  void setup(const SetupInfo &info)
  {
    // ensure we never try to access a null pointer
    setRecodingPendingFrameCompletion(&finalizerData[0]);

    BaseType::setup(info);
  }

  void shutdown(DXGIAdapter *adapter, frontend::BindlessManager *bindless_manager)
  {
    // On shutdown we run over all finalizer data to be sure everything has been cleaned up
    CompletedFrameExecutionInfo info;
    info.progressIndex = 0xFFFFFFFFFFFFFFFFull;
    info.historyIndex = 0;
    info.bindlessManager = bindless_manager;
#if _TARGET_PC_WIN
    info.adapter = adapter;
#else
    G_UNUSED(adapter);
#endif
    for (; info.historyIndex < array_size(finalizerData); ++info.historyIndex)
    {
      BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
    }
    BaseType::shutdown();

    setRecodingPendingFrameCompletion(&finalizerData[0]);
  }

  void preRecovery(DXGIAdapter *adapter, frontend::BindlessManager *bindless_manager)
  {
    // Also here we run over all data to be sure its properly cleaned up
    CompletedFrameExecutionInfo info;
    info.progressIndex = 0xFFFFFFFFFFFFFFFFull;
    info.historyIndex = 0;
    info.bindlessManager = bindless_manager;
#if _TARGET_PC_WIN
    info.adapter = adapter;
#else
    G_UNUSED(adapter);
#endif
    for (; info.historyIndex < array_size(finalizerData); ++info.historyIndex)
    {
      BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
    }
    BaseType::preRecovery();

    setRecodingPendingFrameCompletion(&finalizerData[0]);
  }

  struct BeginFrameRecordingInfo
  {
    uint32_t historyIndex;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info)
  {
    // TODO mutex may not be necessary
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
  }

  void beginFrameRecording(const BeginFrameRecordingInfo &info)
  {
    setRecodingPendingFrameCompletion(&finalizerData[info.historyIndex]);
  }
};

#if DAGOR_DBGLEVEL > 0
class DebugViewBase : public FrameFinalizer
{
  using BaseType = FrameFinalizer;

protected:
  enum class StatusFlag
  {
    PIN_MIN_EVENT_FRAME_RANGE_TO_MAX,
    PIN_MAX_EVENT_FRAME_RANGE_TO_MAX,
    LOAD_VIEWS_FROM_CONFIG,
    STORE_VIEWS_TO_CONFIG,
    COUNT
  };
  using StatusFlags = TypedBitSet<StatusFlag>;

private:
  struct MetricsVisualizerState
  {
    GraphDisplayInfo graphDisplayInfos[static_cast<uint32_t>(Graph::COUNT)];
    ValueRange<uint64_t> shownFrames{0, ~uint64_t(0)};
    StatusFlags statusFlags;
    MetricBits shownBits{};
    char eventObjectNameFilter[MAX_OBJECT_NAME_LENGTH]{};
  };
  MetricsVisualizerState metricsVisualizerState;

protected:
  void setup(const SetupInfo &setup)
  {
    metricsVisualizerState.statusFlags.set(StatusFlag::PIN_MAX_EVENT_FRAME_RANGE_TO_MAX);
    metricsVisualizerState.statusFlags.set(StatusFlag::LOAD_VIEWS_FROM_CONFIG);

    BaseType::setup(setup);
  }

  char *getEventObjectNameFilterBasePointer() { return metricsVisualizerState.eventObjectNameFilter; }

  size_t getEventObjectNameFilterMaxLength() { return array_size(metricsVisualizerState.eventObjectNameFilter); }

  bool checkStatusFlag(StatusFlag flag) const { return metricsVisualizerState.statusFlags.test(flag); }

  void setStatusFlag(StatusFlag flag, bool value) { metricsVisualizerState.statusFlags.set(flag, value); }

  const GraphDisplayInfo &getGraphDisplayInfo(Graph graph)
  {
    return metricsVisualizerState.graphDisplayInfos[static_cast<uint32_t>(graph)];
  }

  void setGraphDisplayInfo(Graph graph, const GraphDisplayInfo &info)
  {
    auto &target = metricsVisualizerState.graphDisplayInfos[static_cast<uint32_t>(graph)];
    if (target != info)
    {
      setStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG, true);
      target = info;
    }
  }

  template <typename T>
  void iterateGraphDisplayInfos(T clb)
  {
    for (size_t i = 0; i < array_size(metricsVisualizerState.graphDisplayInfos); ++i)
    {
      clb(static_cast<Graph>(i), metricsVisualizerState.graphDisplayInfos[i]);
    }
  }

  ValueRange<uint64_t> getShownMetricFrameRange() const { return metricsVisualizerState.shownFrames; }

  void setShownMetricFrameRange(ValueRange<uint64_t> range) { metricsVisualizerState.shownFrames = range; }

  bool isShownMetric(Metric metric) const { return metricsVisualizerState.shownBits.test(metric); }

  void setShownMetric(Metric metric, bool value) { metricsVisualizerState.shownBits.set(metric, value); }

  void storeViewSettings();
  void restoreViewSettings();
};

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
class MetricsVisualizer : public DebugViewBase
{
  using BaseType = DebugViewBase;

private:
  struct PlotData
  {
    ConcurrentMetricsStateAccessToken &self;
    ValueRange<uint64_t> viewRange;
    uint64_t selectedFrameIndex;

    const PerFrameData &getCurrentFrame() const { return self->getLastFrameMetrics(); }

    const PerFrameData &getFrame(int idx) const { return self->getFrameMetrics(viewRange.front() + idx); }

    const PerFrameData &getSelectedFrame() const { return self->getFrameMetrics(selectedFrameIndex); }

    bool selectFrame(uint64_t index)
    {
      const auto &baseFrame = self->getFrameMetrics(0);
      selectedFrameIndex = index - baseFrame.frameIndex;
      return selectedFrameIndex < self->getMetricsHistoryLength();
    }

    bool hasAnyData() const { return viewRange.size() > 0; }

    uint64_t getViewSize() const { return viewRange.size(); }
    void resetViewRange(uint64_t from, uint64_t to) { viewRange.reset(from, to); }
    uint64_t getFirstViewIndex() const { return viewRange.front(); }
  };

  template <typename T, typename U>
  static T getPlotPointFrameValue(void *data, int idx)
  {
    auto info = reinterpret_cast<PlotData *>(data);
    const auto &frame = info->getFrame(idx);
    return //
      {double(frame.frameIndex), double(U::get(frame))};
  }

  template <typename T>
  static T getPlotPointFrameBase(void *data, int idx)
  {
    struct GetZero
    {
      static double get(const PerFrameData &) { return 0; }
    };
    return getPlotPointFrameValue<T, GetZero>(data, idx);
  }

  void drawPersistentBufferBasicMetricsTable(const char *id, const CounterWithSizeInfo &usage_counter,
    const CounterWithSizeInfo &buffer_counter, const PeakCounterWithSizeInfo &peak_memory_counter,
    const PeakCounterWithSizeInfo &peak_buffer_counter);
  void drawRingMemoryBasicMetricsTable(const char *id, const CounterWithSizeInfo &current_counter,
    const CounterWithSizeInfo &summary_counter, const PeakCounterWithSizeInfo &peak_counter,
    const PeakCounterWithSizeInfo &peak_buffer_counter);

  void drawBufferMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index);
  void drawTextureEvent(const MetricsState::ActionInfo &event, uint64_t index);
  void drawHostSharedMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index);
  bool drawEvent(const MetricsState::ActionInfo &event, uint64_t index);
  GraphDisplayInfo drawGraphViewControls(Graph graph);
  void setupPlotY2CountRange(uint64_t max_value, bool has_data);
  void setupPlotYMemoryRange(uint64_t max_value, bool has_data);
  PlotData setupPlotXRange(ConcurrentMetricsStateAccessToken &access, const GraphDisplayInfo &display_state, bool has_data);

protected:
  void drawMetricsCaptureControls();
  void drawMetricsEventsView();
  void drawMetricsEvnetsViewFilterControls();
  void drawSystemCurrentMemoryUseTable();
  void drawSystemMemoryUsePlot();
  void drawHeapsPlot();
  void drawHeapsSummaryTable();
  void drawScratchBufferTable();
  void drawScratchBufferPlot();
  void drawTexturePlot();
  void drawBuffersPlot();
  void drawConstRingMemoryPlot();
  void drawConstRingBasicMetricsTable();
  void drawUploadRingMemoryPlot();
  void drawUploadRingBasicMetricsTable();
  void drawTempuraryUploadMemoryPlot();
  void drawTemporaryUploadMemoryBasicMetrics();
  void drawRaytracePlot();
  void drawUserHeapsPlot();
  void drawPersistenUploadMemoryMetricsTable();
  void drawPersistenReadBackMemoryMetricsTable();
  void drawPersistenBidirectioanlMemoryMetricsTable();
  void drawPersistenUploadMemoryPlot();
  void drawPersistenReadBackMemoryPlot();
  void drawPersistenBidirectioanlMemoryPlot();
  void drawRaytraceSummaryTable();
};
#else
class MetricsVisualizer : public DebugViewBase
{
  using BaseType = DebugViewBase;

protected:
  void drawMetricsCaptureControls();
  void drawMetricsEventsView();
  void drawMetricsEvnetsViewFilterControls();
  void drawSystemCurrentMemoryUseTable();
  void drawSystemMemoryUsePlot();
  void drawHeapsPlot();
  void drawHeapsSummaryTable();
  void drawScratchBufferTable();
  void drawScratchBufferPlot();
  void drawTexturePlot();
  void drawBuffersPlot();
  void drawConstRingMemoryPlot();
  void drawConstRingBasicMetricsTable();
  void drawUploadRingMemoryPlot();
  void drawUploadRingBasicMetricsTable();
  void drawTempuraryUploadMemoryPlot();
  void drawTemporaryUploadMemoryBasicMetrics();
  void drawRaytracePlot();
  void drawUserHeapsPlot();
  void drawPersistenUploadMemoryMetricsTable();
  void drawPersistenReadBackMemoryMetricsTable();
  void drawPersistenBidirectioanlMemoryMetricsTable();
  void drawPersistenUploadMemoryPlot();
  void drawPersistenReadBackMemoryPlot();
  void drawPersistenBidirectioanlMemoryPlot();
  void drawRaytraceSummaryTable();
};
#endif

class DebugView : public MetricsVisualizer
{
  using BaseType = MetricsVisualizer;

  void drawPersistenUploadMemorySegmentTable();
  void drawPersistenReadBackMemorySegmentTable();
  void drawPersistenBidirectioanlMemorySegmentTable();
  void drawPersistenUploadMemoryInfoView();
  void drawPersistenReadBackMemoryInfoView();
  void drawPersistenBidirectioanlMemoryInfoView();
  void drawUserHeapsTable();
  void drawUserHeapsSummaryTable();
  void drawRaytraceTopStructureTable();
  void drawRaytraceBottomStructureTable();
  void drawTempuraryUploadMemorySegmentsTable();
  void drawUploadRingSegmentsTable();
  void drawConstRingSegmentsTable();
  void drawTextureTable();
  void drawBuffersTable();
  void drawRaytracingInfoView();
#if DX12_USE_ESRAM
  void drawESRamInfoView();
#endif
  void drawTemporaryUploadMemoryInfoView();
  void drawUploadRingBufferInfoView();
  void drawConstRingBufferInfoView();
  void drawBuffersInfoView();
  void drawTexturesInfoView();
  void drawScratchBufferInfoView();
  void drawUserHeapInfoView();
  void drawHeapsTable();
  void drawHeapInfoView();
  void drawSystemInfoView();
  void drawManagerBehaviorInfoView();
  void drawEventsInfoView();
  void drawMetricsInfoView();
  void drawSamplersInfoView();
  void drawSamplerTable();

public:
  void debugOverlay();
};
#else
using DebugView = FrameFinalizer;
#endif
} // namespace resource_manager

class ResourceMemoryHeap : private resource_manager::DebugView
{
  using BaseType = resource_manager::DebugView;

public:
  ResourceMemoryHeap() = default;
  ~ResourceMemoryHeap() = default;
  ResourceMemoryHeap(const ResourceMemoryHeap &) = delete;
  ResourceMemoryHeap &operator=(const ResourceMemoryHeap &) = delete;
  ResourceMemoryHeap(ResourceMemoryHeap &&) = delete;
  ResourceMemoryHeap &operator=(ResourceMemoryHeap &&) = delete;

  using Metric = BaseType::Metric;
  using FreeReason = BaseType::FreeReason;
  using CompletedFrameExecutionInfo = BaseType::CompletedFrameExecutionInfo;
  using CompletedFrameRecordingInfo = BaseType::CompletedFrameRecordingInfo;
  using SetupInfo = BaseType::SetupInfo;
  using BeginFrameRecordingInfo = BaseType::BeginFrameRecordingInfo;

  using BaseType::preRecovery;
  using BaseType::setup;
  using BaseType::shutdown;

  using BaseType::completeFrameExecution;

  using BaseType::beginFrameRecording;

#if D3D_HAS_RAY_TRACING
  using BaseType::ensureRaytraceScratchBufferSize;
  using BaseType::getRaytraceScratchBuffer;

  using BaseType::deleteRaytraceBottomAccelerationStructureOnFrameCompletion;
  using BaseType::deleteRaytraceTopAccelerationStructureOnFrameCompletion;
  using BaseType::newRaytraceBottomAccelerationStructure;
  using BaseType::newRaytraceTopAccelerationStructure;
#endif

  using BaseType::getSwapchainColorGlobalId;
  using BaseType::getSwapchainSecondaryColorGlobalId;

#if DAGOR_DBGLEVEL > 0
  using BaseType::debugOverlay;
#endif

  using BaseType::freeUserHeapOnFrameCompletion;
  using BaseType::getResourceHeapGroupProperties;
  using BaseType::getUserHeapMemory;
  using BaseType::newUserHeap;
  using BaseType::placeBufferInHeap;
  using BaseType::placeTextureInHeap;
  using BaseType::visitHeaps;

  using BaseType::getResourceAllocationProperties;

  using BaseType::getFramePushRingMemorySize;
  using BaseType::getPersistentBidirectionalMemorySize;
  using BaseType::getPersistentReadBackMemorySize;
  using BaseType::getPersistentUploadMemorySize;
  using BaseType::getTemporaryUploadMemorySize;
  using BaseType::getUploadRingMemorySize;

  using BaseType::getActiveTextureObjectCount;
  using BaseType::getTextureObjectCapacity;
  using BaseType::reserveTextureObjects;

  using BaseType::getActiveBufferObjectCount;
  using BaseType::getBufferObjectCapacity;
  using BaseType::reserveBufferObjects;

  using BaseType::deleteBufferObject;
  using BaseType::getResourceMemoryForBuffer;

  using BaseType::allocatePersistentBidirectional;
  using BaseType::allocatePersistentReadBack;
  using BaseType::allocatePersistentUploadMemory;
  using BaseType::allocatePushMemory;
  using BaseType::allocateTempUpload;
  using BaseType::allocateTempUploadForUploadBuffer;
  using BaseType::allocateUploadRingMemory;
  using BaseType::freeHostDeviceSharedMemoryRegionForUploadBufferOnFrameCompletion;
  using BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion;

  using BaseType::getTempScratchBufferSpace;

  using BaseType::allocateTextureDSVDescriptor;
  using BaseType::allocateTextureRTVDescriptor;
  using BaseType::allocateTextureSRVDescriptor;
  using BaseType::freeTextureDSVDescriptor;
  using BaseType::freeTextureRTVDescriptor;
  using BaseType::freeTextureSRVDescriptor;

  using BaseType::allocateBufferSRVDescriptor;

  using BaseType::adoptTexture;
  using BaseType::aliasTexture;
  using BaseType::createTexture;
  using BaseType::deleteTextureObjectOnFrameCompletion;
  using BaseType::destroyTextureOnFrameCompletion;
  using BaseType::newTextureObject;
  using BaseType::visitImageObjects;
  using BaseType::visitTextureObjects;

  using BaseType::allocateBuffer;
  using BaseType::createBufferRawSRV;
  using BaseType::createBufferRawUAV;
  using BaseType::createBufferStructureSRV;
  using BaseType::createBufferStructureUAV;
  using BaseType::createBufferTextureSRV;
  using BaseType::createBufferTextureUAV;
  using BaseType::discardBuffer;
  using BaseType::freeBufferOnFrameCompletion;
  using BaseType::newBufferObject;
  using BaseType::visitBufferObjects;
  using BaseType::visitBuffers;

  using BaseType::getDeviceLocalAvailablePoolBudget;
  using BaseType::getDeviceLocalBudget;
  using BaseType::getHostLocalAvailablePoolBudget;
  using BaseType::getHostLocalBudget;
  using BaseType::isUMASystem;

  using BaseType::setTempBufferShrinkThresholdSize;

  using BaseType::popEvent;
  using BaseType::pushEvent;

  using BaseType::completeFrameRecording;

#if DX12_USE_ESRAM
  using BaseType::aliasESRamTexture;
  using BaseType::createESRamTexture;
  using BaseType::deselectLayout;
  using BaseType::fetchMovableTextures;
  using BaseType::hasESRam;
  using BaseType::registerMovableTexture;
  using BaseType::resetAllLayouts;
  using BaseType::selectLayout;
  using BaseType::unmapResource;
  using BaseType::writeBackMovableTextures;
#endif

  using BaseType::enterSuspended;
  using BaseType::leaveSuspended;

  void waitForTemporaryUsesToFinish()
  {
    uint32_t retries = 0;
    debug("DX12: Begin waiting for temporary memory uses to be completed...");
    while (!allTemporaryUsesCompleted([this](auto invoked) {
      debug("DX12: Checking for temporary memory uses to be completed...");
      visitAllFrameFinilazierData(invoked);
    }))
    {
      if (++retries > 100)
      {
        debug("DX12: Waiting for too long, exiting, risking crashes...");
        break;
      }
      debug("DX12: Wating 10ms to complete temporary memory uses...");
      sleep_msec(10);
    }
    debug("DX12: Finished waiting for temporary memory uses...");
  }

  void generateResourceAndMemoryReport(uint32_t *num_textures, uint64_t *total_mem, String *out_text);

  using BaseType::reportOOMInformation;

  using BaseType::isImageAlive;

  using BaseType::createSampler;
  using BaseType::deleteSampler;
  using BaseType::getSampler;
};
} // namespace drv3d_dx12
