// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <container_mutex_wrapper.h>
#include <d3d12_utils.h>
#include <driver.h>
#include <extents.h>
#include <format_store.h>
#include <free_list_utils.h>
#include <image_global_subresource_id.h>
#include <tagged_types.h>
#include <typed_bit_set.h>

#include <debug/dag_assert.h>
#include <EASTL/numeric.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>

#include <windows.h>
#include <psapi.h> // psapi.h is not self-contained, it needs windows.h

// We try to work without this
#define DX12_USE_ACTIVITY_LOCKING 0

namespace drv3d_dx12::resource_manager
{
class ConcurrentAccessControler
{
protected:
  struct SetupInfo
  {};

  struct CompletedFrameExecutionInfo
  {};

  struct CompletedFrameRecordingInfo
  {};

  struct PendingForCompletedFrameData
  {};

#if DX12_USE_ACTIVITY_LOCKING
  std::atomic<uint32_t> suspendState{0};
  std::atomic<uint32_t> activityState{0};
#endif
  void *recodingPendingFrameCompletion DAG_TS_GUARDED_BY(recodingPendingFrameCompletionMutex) = nullptr;
  OSSpinlock recodingPendingFrameCompletionMutex;

  ConcurrentAccessControler() = default;
  ~ConcurrentAccessControler() = default;
  ConcurrentAccessControler(const ConcurrentAccessControler &) = delete;
  ConcurrentAccessControler &operator=(const ConcurrentAccessControler &) = delete;
  ConcurrentAccessControler(ConcurrentAccessControler &&) = delete;
  ConcurrentAccessControler &operator=(ConcurrentAccessControler &&) = delete;

  void shutdown() {}

  void preRecovery() {}

  void completeFrameRecording(const CompletedFrameRecordingInfo &) {}

  void completeFrameExecution(const CompletedFrameExecutionInfo &, PendingForCompletedFrameData &) {}

  void setup(const SetupInfo &) {}

  template <typename T>
  bool allTemporaryUsesCompleted(T)
  {
    return true;
  }

protected:
  void setRecodingPendingFrameCompletion(void *ptr)
  {
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    recodingPendingFrameCompletion = ptr;
  }

  // Hacky way to make PendingForCompletedFrameData for each module available, the
  // enable_if with convertible check ensures that no bad type can be used!
  template <typename T, typename U>
  typename eastl::enable_if<eastl::is_convertible<T &, PendingForCompletedFrameData &>::value>::type
  accessRecodingPendingFrameCompletion(U accessor)
  {
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    accessor(*static_cast<T *>(recodingPendingFrameCompletion));
  }

  void beginActivity()
  {
#if DX12_USE_ACTIVITY_LOCKING
    for (;;)
    {
      auto currentSuspend = suspendState.load();
      if (0 != currentSuspend)
      {
        wait(suspendState, currentSuspend);
      }
      else
      {
        ++activityState;
        // have to check again
        if (0 == suspendState.load())
        {
          break;
        }
        --activityState;
      }
    }
#endif
  }

  void endActivity()
  {
#if DX12_USE_ACTIVITY_LOCKING
    auto currentActivity = --activityState;
    if (!currentActivity)
    {
      notify_one(suspendState);
    }
#endif
  }

  struct ActivityLock
  {
    ConcurrentAccessControler &manager;

  public:
    ActivityLock(ConcurrentAccessControler &m) : manager{m} { manager.beginActivity(); }
    ~ActivityLock() { manager.endActivity(); }
  };

public:
  void enterSuspended()
  {
#if DX12_USE_ACTIVITY_LOCKING
    ++suspendState;
    for (;;)
    {
      auto currentActivity = activityState.load();
      if (0 == currentActivity)
      {
        break;
      }
      wait(activityState, currentActivity);
    }
#endif
  }

  void leaveSuspended()
  {
#if DX12_USE_ACTIVITY_LOCKING
    --suspendState;
    notify_all(activityState);
#endif
  }
};

// This reporter has to be pretty low in the stack, as it is used by lots of components in the middle of the stack.
class OutOfMemoryRepoter : public ConcurrentAccessControler
{
  using BaseType = ConcurrentAccessControler;

  bool didReportOOM = false;

protected:
  void setup(const SetupInfo &info)
  {
    didReportOOM = false;
    BaseType::setup(info);
  }

  class OomReportData
  {
  public:
    OomReportData(const char *method_name, const char *resource_name = nullptr, const eastl::optional<uint64_t> &requested_size = {},
      const eastl::optional<uint32_t> &allocation_flags = {}, const eastl::optional<uint32_t> &memory_group = {}) :
      methodName{method_name},
      resourceName{resource_name},
      requestedSize{requested_size},
      allocationFlags{allocation_flags},
      memoryGroup{memory_group}
    {}

    const char *getMethodName() const { return methodName; }

    eastl::string toString() const
    {
      eastl::string oomReport{"method: "};
      oomReport += methodName;
      if (resourceName)
      {
        oomReport += ", resource: ";
        oomReport += resourceName;
      }
      if (requestedSize)
      {
        oomReport += ", requested size: ";
        oomReport += eastl::to_string(*requestedSize);
      }
      if (allocationFlags)
      {
        oomReport += ", allocation flags: ";
        oomReport += eastl::to_string(*allocationFlags);
      }
      if (memoryGroup)
      {
        oomReport += ", requested memory group: ";
        oomReport += eastl::to_string(*memoryGroup);
      }
      return oomReport;
    }

  private:
    const char *methodName = nullptr;
    const char *resourceName = nullptr;
    eastl::optional<uint64_t> requestedSize;
    eastl::optional<uint32_t> allocationFlags;
    eastl::optional<uint32_t> memoryGroup;
  };

  void reportOOMInformation(DXGIAdapter *adapter);

  bool checkForOOM(DXGIAdapter *adapter, bool was_okay, const OomReportData &report_data);

  template <typename C>
  class ScopeExitChecker
  {
    DXGIAdapter *adapter;
    OutOfMemoryRepoter &self;
    C checker;
    OomReportData reportData;

  public:
    ScopeExitChecker(DXGIAdapter *a, OutOfMemoryRepoter &s, C &&c, const OomReportData &report_data) :
      adapter{a}, self{s}, checker{c}, reportData{report_data}
    {}

    ScopeExitChecker(const ScopeExitChecker &) = delete;
    ScopeExitChecker(ScopeExitChecker &&) = delete;
    ScopeExitChecker &operator=(const ScopeExitChecker &) = delete;
    ScopeExitChecker &operator=(ScopeExitChecker &&) = delete;

    ~ScopeExitChecker() { self.checkForOOM(adapter, this->checker(), reportData); }
  };

  template <typename C>
  ScopeExitChecker<C> checkForOOMOnExit(DXGIAdapter *adapter, C &&checker, const OomReportData &report_data)
  {
    return {adapter, *this, eastl::forward<C>(checker), report_data};
  }
};

class MetricsProviderBase : public OutOfMemoryRepoter
{
  using BaseType = OutOfMemoryRepoter;

public:
  enum class Metric
  {
    EVENT_LOG,
    METRIC_LOG,
    MEMORY_USE,
    HEAPS,
    MEMORY,
    BUFFERS,
    TEXTURES,
    CONST_RING,
    TEMP_RING,
    TEMP_MEMORY,
    PERSISTENT_UPLOAD,
    PERSISTENT_READ_BACK,
    PERSISTENT_BIDIRECTIONAL,
    SCRATCH_BUFFER,
    RAYTRACING,
    COUNT,
  };
  using MetricBits = TypedBitSet<Metric>;
  struct SetupInfo : BaseType::SetupInfo
  {
    MetricBits collectedMetrics;
  };

protected:
  struct MetricNameTableEntry
  {
    Metric metric;
    const char *name;
  };
  static const MetricNameTableEntry metric_name_table[static_cast<uint32_t>(Metric::COUNT)];
  enum class Graph : uint32_t
  {
    SYSTEM,
    CONST_RING,
    UPLOAD_RING,
    TEMP_MEMORY,
    UPLOAD_MEMORY,
    READ_BACK_MEMORY,
    BIDIRECTIONAL_MEMORY,
    BUFFERS,
    HEAPS,
    TEXTURES,
    USER_HEAPS,
    SCRATCH_BUFFER,
    RAYTRACING,
    COUNT
  };
  static const char *graph_name_table[static_cast<uint32_t>(Graph::COUNT)];
  enum class GraphMode : int
  {
    BLOCK_SCROLLING,
    CONTIGUOUS_SCROLLING,
    FULL_RANGE,
    DISABLED,
  };
  static const char *graph_mode_name_table[4];
  static constexpr uint64_t default_graph_window_size = 500;
  static constexpr uint64_t min_graph_window_size = default_graph_window_size / 8;
  static constexpr uint64_t max_graph_window_size = default_graph_window_size * 8;
  struct GraphDisplayInfo
  {
    GraphMode mode = GraphMode::BLOCK_SCROLLING;
    uint64_t windowSize = default_graph_window_size;

    friend bool operator==(const GraphDisplayInfo &l, const GraphDisplayInfo &r)
    {
      return l.mode == r.mode && l.windowSize == r.windowSize;
    }

    friend bool operator!=(const GraphDisplayInfo &l, const GraphDisplayInfo &r)
    {
      return l.mode != r.mode || l.windowSize != r.windowSize;
    }
  };
  struct CounterWithSizeInfo
  {
    uint64_t count;
    uint64_t size;

    CounterWithSizeInfo &operator+=(const CounterWithSizeInfo &other)
    {
      count += other.count;
      size += other.size;
      return *this;
    }

    CounterWithSizeInfo &max_size(const CounterWithSizeInfo &other)
    {
      if (other.size > size)
      {
        count = other.count;
        size = other.size;
      }
      return *this;
    }

    friend CounterWithSizeInfo operator-(const CounterWithSizeInfo &l, const CounterWithSizeInfo &r)
    {
      return {l.count - r.count, l.size - r.size};
    }

    friend CounterWithSizeInfo operator+(const CounterWithSizeInfo &l, const CounterWithSizeInfo &r)
    {
      return {l.count + r.count, l.size + r.size};
    }
  };

  struct PeakCounterWithSizeInfo : CounterWithSizeInfo
  {
    uint64_t index;

    void update(uint64_t idx, const CounterWithSizeInfo &value)
    {
      if (size < value.size)
      {
        static_cast<CounterWithSizeInfo &>(*this) = value;
        index = idx;
      }
    }
  };
  // Defines a set of rawCounters and their relation to each other:
  // ALLOCATE_FREE_COUNTERS is a set of rawCounters to track allocate and free pairs of a specific
  // type, storing the net value in a third counter.
  //
  // ALLOCATE_ALIAS_COUNTERS is a counter that is an dedicated counter that contributes positively
  // to its net value that is used by a previously define allocate free rawCounters set.
  //
  // COUNTER a single counter with no other relations.
#define COUNTER_SET_DEFINITION                                                                                                      \
  ALLOCATE_FREE_COUNTERS(newCPUHeaps, deletedCPUHeaps, cpuHeaps)                                                                    \
  ALLOCATE_FREE_COUNTERS(newGPUHeaps, deletedGPUHeaps, gpuHeaps)                                                                    \
  ALLOCATE_FREE_COUNTERS(allocatedCPUMemory, freedCPUMemory, cpuMemory)                                                             \
  ALLOCATE_FREE_COUNTERS(allocatedGPUMemory, freedGPUMemory, gpuMemory)                                                             \
  ALLOCATE_FREE_COUNTERS(allocatedCPUBufferHeaps, freedCPUBufferHeaps, cpuBufferHeaps)                                              \
  ALLOCATE_FREE_COUNTERS(allocatedGPUBufferHeaps, freedGPUBufferHeaps, gpuBufferHeaps)                                              \
  ALLOCATE_FREE_COUNTERS(allocatedTextures, freedTextures, textures)                                                                \
  ALLOCATE_ALIAS_COUNTERS(aliasedTextures, textures)                                                                                \
  ALLOCATE_ALIAS_COUNTERS(adoptedTextures, textures)                                                                                \
  ALLOCATE_ALIAS_COUNTERS(placedTextures, textures)                                                                                 \
  ALLOCATE_ALIAS_COUNTERS(placedCpuBufferHeaps, cpuBufferHeaps)                                                                     \
  ALLOCATE_ALIAS_COUNTERS(placedGpuBufferHeaps, gpuBufferHeaps)                                                                     \
  ALLOCATE_FREE_COUNTERS(allocatedConstRing, freedConstRing, constRing)                                                             \
  COUNTER(usedConstRing)                                                                                                            \
  ALLOCATE_FREE_COUNTERS(allocatedUploadRing, freedUploadRing, uploadRing)                                                          \
  COUNTER(usedUploadRing)                                                                                                           \
  ALLOCATE_FREE_COUNTERS(allocatedTempBuffer, freedTempBuffer, tempBuffer)                                                          \
  COUNTER(usedTempBuffer)                                                                                                           \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentUploadBuffer, freedPersistentUploadBuffer, persistentUploadBuffer)                      \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentUploadMemory, freedPersistentUploadMemory, persistentUploadMemory)                      \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentReadBackBuffer, freedPersistentReadBackBuffer, persistentReadBackBuffer)                \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentReadBackMemory, freedPersistentReadBackMemory, persistentReadBackMemory)                \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentBidirectionalBuffer, freedPersistentBidirectionalBuffer, persistentBidirectionalBuffer) \
  ALLOCATE_FREE_COUNTERS(allocatedPersistentBidirectionalMemory, freedPersistentBidirectionalMemory, persistentBidirectionalMemory) \
  COUNTER(discaredBufferMemory)                                                                                                     \
  COUNTER(hitDiscardedBufferMemory)                                                                                                 \
  COUNTER(driverDiscardedBufferMemory)                                                                                              \
  COUNTER(hitDriverDiscardedBufferMemory)                                                                                           \
  ALLOCATE_FREE_COUNTERS(newUserResourceHeaps, deletedUserResourceHeaps, userResourceHeaps)                                         \
  ALLOCATE_FREE_COUNTERS(allocatedScratchBuffer, freedScratchBuffer, scratchBuffer)                                                 \
  COUNTER(scratchBufferTempUse)                                                                                                     \
  COUNTER(scratchBufferPersistentUse)                                                                                               \
  ALLOCATE_FREE_COUNTERS(allocatedRaytraceAccelStructHeap, freedRaytraceAccelStructHeap, raytraceAccelStructHeap)                   \
  ALLOCATE_FREE_COUNTERS(allocatedRaytraceBottomStructure, freedRaytraceBottomStructure, raytraceBottomStructure)                   \
  ALLOCATE_FREE_COUNTERS(allocatedRaytraceTopStructure, freedRaytraceTopStructure, raytraceTopStructure)

  struct RawCounterSet
  {
#define COUNTER(name)                                           CounterWithSizeInfo name;
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) CounterWithSizeInfo allocateName, freeName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)          CounterWithSizeInfo allocateName;
    COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS

    RawCounterSet &operator+=(const RawCounterSet &other)
    {
#define COUNTER(name) name += other.name;
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) \
  allocateName += other.allocateName;                           \
  freeName += other.freeName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName) allocateName += other.allocateName;
      COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS
      return *this;
    }

    // per member max
    RawCounterSet &max(const RawCounterSet &other)
    {
#define COUNTER(name) name.max_size(other.name);
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) \
  allocateName.max_size(other.allocateName);                    \
  freeName.max_size(other.freeName);
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName) allocateName.max_size(other.allocateName);
      COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS
      return *this;
    }
  };

  struct PeakRawCounterSet
  {
#define COUNTER(name)                                           PeakCounterWithSizeInfo name;
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) PeakCounterWithSizeInfo allocateName, freeName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)          PeakCounterWithSizeInfo allocateName;
    COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS

    void update(uint64_t idx, const RawCounterSet &values)
    {
#define COUNTER(name) name.update(idx, values.name);
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) \
  allocateName.update(idx, values.allocateName);                \
  freeName.update(idx, values.freeName);
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName) allocateName.update(idx, values.allocateName);
      COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS
    }
  };

  struct NetCounterSet
  {
#define COUNTER(name)
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) CounterWithSizeInfo netName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)
    COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS

    NetCounterSet &update(const RawCounterSet &raw)
    {
#define COUNTER(name)
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) netName = netName + raw.allocateName - raw.freeName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)          netName += raw.allocateName;
      COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS
      return *this;
    }
  };

  struct PeakNetCounterSet
  {
#define COUNTER(name)
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) PeakCounterWithSizeInfo netName;
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)
    COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS

    void update(uint64_t idx, const NetCounterSet &other)
    {
#define COUNTER(name)
#define ALLOCATE_FREE_COUNTERS(allocateName, freeName, netName) netName.update(idx, other.netName);
#define ALLOCATE_ALIAS_COUNTERS(allocateName, netName)
      COUNTER_SET_DEFINITION
#undef COUNTER
#undef ALLOCATE_FREE_COUNTERS
#undef ALLOCATE_ALIAS_COUNTERS
    }
  };

  struct PerFrameData
  {
    uint64_t frameIndex;
    // on consoles there is only process memory use as its UMA
#if _TARGET_PC_WIN
    DXGI_QUERY_VIDEO_MEMORY_INFO deviceMemoryInUse;
    DXGI_QUERY_VIDEO_MEMORY_INFO systemMemoryInUse;
    DXGI_QUERY_VIDEO_MEMORY_INFO deviceMemoryInUsePeak;
    DXGI_QUERY_VIDEO_MEMORY_INFO systemMemoryInUsePeak;
#endif
    uint64_t processMemoryInUse;
    uint64_t processMemoryInUsePeak;
    RawCounterSet rawCounters;
    RawCounterSet rawCountersSummary;
    PeakRawCounterSet rawCountersPeak;
    NetCounterSet netCounters;
    PeakNetCounterSet netCountersPeak;
  };
  struct MetricsState
  {
    struct ActionInfo
    {
      enum class Type
      {
        ALLOCATE_HEAP,
        FREE_HEAP,
        ALLOCATE_MEMORY,
        FREE_MEMORY,
        ALLOCATE_BUFFER_HEAP,
        FREE_BUFFER_HEAP,
        ALLOCATE_TEXTURE,
        ALIAS_TEXTURE,
        ADOPT_TEXTURE,
        ESRAM_TEXTURE,
        FREE_TEXTURE,
        ALLOCATE_CONST_RING,
        FREE_CONST_RING,
        ALLOCATE_TEMP_RING,
        FREE_TEMP_RING,
        ALLOCATE_TEMP_BUFFER,
        FREE_TEMP_BUFFER,
        USE_TEMP_BUFFER,
        ALLOCATE_PERSISTENT_UPLOAD_BUFFER,
        FREE_PERSISTENT_UPLOAD_BUFFER,
        ALLOCATE_PERSISTENT_UPLOAD_MEMORY,
        FREE_PERSISTENT_UPLOAD_MEMORY,
        ALLOCATE_PERSISTENT_READ_BACK_BUFFER,
        FREE_PERSISTENT_READ_BACK_BUFFER,
        ALLOCATE_PERSISTENT_READ_BACK_MEMORY,
        FREE_PERSISTENT_READ_BACK_MEMORY,
        ALLOCATE_PERSISTENT_BIDIRECTIONAL_BUFFER,
        FREE_PERSISTENT_BIDIRECTIONAL_BUFFER,
        ALLOCATE_PERSISTENT_BIDIRECTIONAL_MEMORY,
        FREE_PERSISTENT_BIDIRECTIONAL_MEMORY,
        ALLOCATE_USER_RESOURCE_HEAP,
        FREE_USER_RESOURCE_HEAP,
        PLACE_BUFFER_HEAP_IN_USER_RESOURCE_HEAP,
        PLACE_TEXTURE_IN_USER_RESOURCE_HEAP,
        SCRATCH_BUFFER_ALLOCATE,
        SCRATCH_BUFFER_FREE,
        RAYTRACE_ACCEL_STRUCT_HEAP_ALLOCATE,
        RAYTRACE_ACCEL_STRUCT_HEAP_FREE,
        RAYTRACE_BOTTOM_STRUCTURE_ALLOCATE,
        RAYTRACE_BOTTOM_STRUCTURE_FREE,
        RAYTRACE_TOP_STRUCTURE_ALLOCATE,
        RAYTRACE_TOP_STRUCTURE_FREE,
        COUNT
      };
      Type type;
      uint64_t frameIndex;
      int timeIndex;
      eastl::string eventPath;
      eastl::string objectName;
      uint32_t size;
      union
      {
        struct
        {
          bool isGPUMemory;
        };
        struct
        {
          DXGI_FORMAT format;
          Extent3D extent;
          uint32_t mips;
          uint32_t arrays;
        };
      };
    };
    PerFrameData currentFrame{};
    eastl::string curentEventPath;
    dag::Vector<size_t> eventPathBacktrace;
    dag::Vector<ActionInfo> actionHistory;
    dag::Vector<PerFrameData> frameHistory;

    const PerFrameData &getLastFrameMetrics() const
    {
      if (frameHistory.empty())
      {
        return currentFrame;
      }
      return frameHistory.back();
    }

    const PerFrameData &getFrameMetrics(uint32_t index) const
    {
      if (frameHistory.size() <= index)
      {
        return currentFrame;
      }
      return frameHistory[index];
    }

    size_t getMetricsHistoryLength() const { return frameHistory.size(); }

    template <typename T>
    void iterateMetricsActionLog(T clb)
    {
      for (auto &action : actionHistory)
      {
        clb(action);
      }
    }

    size_t getMetricsActionLogLength() { return actionHistory.size(); }

    uint64_t getMetricsFrameMax()
    {
      if (frameHistory.empty())
      {
        return currentFrame.frameIndex;
      }
      return frameHistory.back().frameIndex;
    }

    size_t calculateMetricsMemoryUsage()
    {
      size_t size = sizeof(currentFrame);
      size += curentEventPath.capacity();
      size += eastl::accumulate(begin(actionHistory), end(actionHistory), size_t(0),
        [](auto l, auto &r) //
        { return l + (sizeof(ActionInfo) + r.eventPath.capacity() + r.objectName.capacity()); });
      size += (actionHistory.capacity() - actionHistory.size()) * sizeof(ActionInfo);
      size += frameHistory.capacity() * sizeof(PerFrameData);
      return size;
    }

    template <typename T>
    void iterateMetricsHistory(T clb)
    {
      for (auto &e : frameHistory)
      {
        clb(e);
      }
    }

    void recordMemoryEvent(ActionInfo::Type type, size_t size, bool is_gpu)
    {
      ActionInfo msg;
      msg.type = type;
      msg.frameIndex = currentFrame.frameIndex;
      msg.eventPath = curentEventPath;
      msg.size = size;
      msg.isGPUMemory = is_gpu;
      msg.timeIndex = get_time_msec();
      actionHistory.push_back(msg);
    }

    void recordBufferEvent(ActionInfo::Type type, size_t size, bool is_gpu, const char *name)
    {
      ActionInfo msg;
      msg.type = type;
      msg.frameIndex = currentFrame.frameIndex;
      msg.eventPath = curentEventPath;
      msg.size = size;
      msg.isGPUMemory = is_gpu;
      msg.timeIndex = get_time_msec();
      if (name)
      {
        msg.objectName = name;
      }
      actionHistory.push_back(msg);
    }

    void recordTextureEvent(ActionInfo::Type type, MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, uint32_t size,
      FormatStore format, const char *name)
    {
      ActionInfo msg;
      msg.type = type;
      msg.frameIndex = currentFrame.frameIndex;
      msg.eventPath = curentEventPath;
      msg.format = format.asDxGiTextureCreateFormat();
      msg.size = size;
      msg.extent = extent;
      msg.mips = mips.count();
      msg.arrays = arrays.count();
      msg.timeIndex = get_time_msec();
      if (name)
      {
        msg.objectName = name;
      }
      actionHistory.push_back(msg);
    }
  };

  static const Metric event_to_metric_type_table[static_cast<uint32_t>(MetricsState::ActionInfo::Type::COUNT)];
  static const char *event_type_name_table[static_cast<uint32_t>(MetricsState::ActionInfo::Type::COUNT)];
};

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
class MetricsProvider : public MetricsProviderBase
{
  using BaseType = MetricsProviderBase;

protected:
  using ConcurrentMetricsState = ContainerMutexWrapper<MetricsState, WinCritSec>;
  using ConcurrentMetricsStateAccessToken = ConcurrentMetricsState::AccessToken;

private:
  MetricBits metricsCollectedBits{};
  ConcurrentMetricsState metrics;

  void recordMemoryEvent(ConcurrentMetricsState::AccessToken &at, MetricsState::ActionInfo::Type type, size_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::EVENT_LOG))
      return;
    at->recordMemoryEvent(type, size, is_gpu);
  }

  void recordBufferEvent(ConcurrentMetricsState::AccessToken &at, MetricsState::ActionInfo::Type type, size_t size,
    bool is_gpu = false, const char *name = nullptr)
  {
    if (!isCollectingMetric(Metric::EVENT_LOG))
      return;
    at->recordBufferEvent(type, size, is_gpu, name);
  }

  void recordTextureEvent(ConcurrentMetricsState::AccessToken &at, MetricsState::ActionInfo::Type type, MipMapCount mips,
    ArrayLayerCount arrays, const Extent3D &extent, uint32_t size, FormatStore format, const char *name)
  {
    if (!isCollectingMetric(Metric::EVENT_LOG))
      return;
    at->recordTextureEvent(type, mips, arrays, extent, size, format, name);
  }

protected:
  bool isCollectingMetric(Metric metric) const { return metricsCollectedBits.test(metric); }
  bool isAnyCounterCollecting() const
  {
    return isCollectingMetric(Metric::HEAPS) || isCollectingMetric(Metric::MEMORY) || isCollectingMetric(Metric::BUFFERS) ||
           isCollectingMetric(Metric::TEXTURES) || isCollectingMetric(Metric::CONST_RING) || isCollectingMetric(Metric::TEMP_RING) ||
           isCollectingMetric(Metric::TEMP_MEMORY) || isCollectingMetric(Metric::PERSISTENT_UPLOAD) ||
           isCollectingMetric(Metric::PERSISTENT_READ_BACK) || isCollectingMetric(Metric::PERSISTENT_BIDIRECTIONAL) ||
           isCollectingMetric(Metric::SCRATCH_BUFFER) || isCollectingMetric(Metric::RAYTRACING);
  }
  bool isCollectingAnyMetric() const { return metricsCollectedBits.any(); }

  void setCollectingMetric(Metric metric, bool value) { metricsCollectedBits.set(metric, value); }

  void clearMetricsActionLog() { metrics.access()->actionHistory.clear(); }

  void clearMetrics()
  {
    auto metricsAccess = metrics.access();
    metricsAccess->actionHistory.clear();
    metricsAccess->frameHistory.clear();
  }

  auto accessMetrics() { return metrics.access(); }

  void recordHeapAllocated(uint32_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::HEAPS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = is_gpu ? metricsAccess->currentFrame.rawCounters.newGPUHeaps : metricsAccess->currentFrame.rawCounters.newCPUHeaps;
    target.count++;
    target.size += size;

    recordMemoryEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_HEAP, size, is_gpu);
  }

  void recordHeapFreed(uint32_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::HEAPS))
      return;
    auto metricsAccess = metrics.access();
    auto &target =
      is_gpu ? metricsAccess->currentFrame.rawCounters.deletedGPUHeaps : metricsAccess->currentFrame.rawCounters.deletedCPUHeaps;
    target.count++;
    target.size += size;

    recordMemoryEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_HEAP, size, is_gpu);
  }

  void recordMemoryAllocated(uint32_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::MEMORY))
      return;
    auto metricsAccess = metrics.access();
    auto &target =
      is_gpu ? metricsAccess->currentFrame.rawCounters.allocatedGPUMemory : metricsAccess->currentFrame.rawCounters.allocatedCPUMemory;
    target.count++;
    target.size += size;

    recordMemoryEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_MEMORY, size, is_gpu);
  }

  void recordMemoryFreed(uint32_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::MEMORY))
      return;
    auto metricsAccess = metrics.access();
    auto &target =
      is_gpu ? metricsAccess->currentFrame.rawCounters.freedGPUMemory : metricsAccess->currentFrame.rawCounters.freedCPUMemory;
    target.count++;
    target.size += size;

    recordMemoryEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_MEMORY, size, is_gpu);
  }

  void recordBufferHeapAllocated(uint32_t size, bool is_gpu, const char *name)
  {
    if (!isCollectingMetric(Metric::BUFFERS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = is_gpu ? metricsAccess->currentFrame.rawCounters.allocatedGPUBufferHeaps
                          : metricsAccess->currentFrame.rawCounters.allocatedCPUBufferHeaps;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_BUFFER_HEAP, size, is_gpu, name);
  }
  void recordBufferHeapFreed(uint32_t size, bool is_gpu, const char *name)
  {
    if (!isCollectingMetric(Metric::BUFFERS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = is_gpu ? metricsAccess->currentFrame.rawCounters.freedGPUBufferHeaps
                          : metricsAccess->currentFrame.rawCounters.freedCPUBufferHeaps;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_BUFFER_HEAP, size, is_gpu, name);
  }

  void recordTextureAllocated(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, uint32_t size, FormatStore format,
    const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedTextures;
    target.count++;
    target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_TEXTURE, mips, arrays, extent, size, format, name);
  }

  void recordTextureAliased(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, FormatStore format, const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.aliasedTextures;
    target.count++;
    // target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::ALIAS_TEXTURE, mips, arrays, extent, 0, format, name);
  }

  void recordTextureAdopted(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, FormatStore format, const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.adoptedTextures;
    target.count++;
    // target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::ADOPT_TEXTURE, mips, arrays, extent, 0, format, name);
  }

  void recordTextureESRamAllocated(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, uint32_t size, FormatStore format,
    const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedTextures;
    target.count++;
    // ESRam does not count as regular memory, textures using ESRam are just a view into the static
    // memory region target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::ESRAM_TEXTURE, mips, arrays, extent, size, format, name);
  }

  void recordTextureFreed(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, uint32_t size, FormatStore format,
    const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedTextures;
    target.count++;
    target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_TEXTURE, mips, arrays, extent, size, format, name);
  }

  void recordConstantRingAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::CONST_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedConstRing;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_CONST_RING, size);
  }

  void recordConstantRingFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::CONST_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedConstRing;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_CONST_RING, size);
  }

  void recordConstantRingUsed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::CONST_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.usedConstRing;
    target.count++;
    target.size += size;
  }

  void recordUploadRingAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedUploadRing;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_TEMP_RING, size);
  }

  void recordUploadRingFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedUploadRing;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_TEMP_RING, size);
  }

  void recordUploadRingUsed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_RING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.usedUploadRing;
    target.count++;
    target.size += size;
    // does not generate events, there are way too many
  }

  void recordTempBufferAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_MEMORY))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedTempBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_TEMP_BUFFER, size);
  }

  void recordTempBufferFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_MEMORY))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedTempBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_TEMP_BUFFER, size);
  }

  void recordTempBufferUsed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::TEMP_MEMORY))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.usedTempBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::USE_TEMP_BUFFER, size);
  }

  void recordPersistentUploadBufferAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_UPLOAD))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentUploadBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_UPLOAD_BUFFER, size);
  }

  void recordPersistentUploadBufferFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_UPLOAD))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentUploadBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_UPLOAD_BUFFER, size);
  }

  void recordPersistentUploadMemoryAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_UPLOAD))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentUploadMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_UPLOAD_MEMORY, size);
  }

  void recordPersistentUploadMemoryFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_UPLOAD))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentUploadMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_UPLOAD_MEMORY, size);
  }

  void recordPersistentReadBackBufferAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_READ_BACK))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentReadBackBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_READ_BACK_BUFFER, size);
  }

  void recordPersistentReadBackBufferFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_READ_BACK))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentReadBackBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_READ_BACK_BUFFER, size);
  }

  void recordPersistentReadBackMemoryAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_READ_BACK))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentReadBackMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_READ_BACK_MEMORY, size);
  }

  void recordPersistentReadBackMemoryFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_READ_BACK))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentReadBackMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_READ_BACK_MEMORY, size);
  }

  void recordPersistentBidirectionalBufferAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_BIDIRECTIONAL))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentBidirectionalBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_BIDIRECTIONAL_BUFFER, size);
  }

  void recordPersistentBidirectionalBufferFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_BIDIRECTIONAL))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentBidirectionalBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_BIDIRECTIONAL_BUFFER, size);
  }

  void recordPersistentBidirectionalMemoryAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_BIDIRECTIONAL))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedPersistentBidirectionalMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_BIDIRECTIONAL_MEMORY, size);
  }

  void recordPersistentBidirectionalMemoryFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::PERSISTENT_BIDIRECTIONAL))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedPersistentBidirectionalMemory;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_PERSISTENT_BIDIRECTIONAL_MEMORY, size);
  }

  void recordNewUserResourceHeap(size_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::HEAPS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.newUserResourceHeaps;
    target.count++;
    target.size += size;

    recordMemoryEvent(metricsAccess, MetricsState::ActionInfo::Type::ALLOCATE_USER_RESOURCE_HEAP, size, is_gpu);
  }

  void recordDeletedUserResouceHeap(size_t size, bool is_gpu)
  {
    if (!isCollectingMetric(Metric::HEAPS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.deletedUserResourceHeaps;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::FREE_USER_RESOURCE_HEAP, size, is_gpu);
  }

  void recordTexturePlacedInUserResourceHeap(MipMapCount mips, ArrayLayerCount arrays, const Extent3D &extent, uint32_t size,
    FormatStore format, const char *name)
  {
    if (!isCollectingMetric(Metric::TEXTURES))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.placedTextures;
    target.count++;
    // target.size += size;

    recordTextureEvent(metricsAccess, MetricsState::ActionInfo::Type::PLACE_TEXTURE_IN_USER_RESOURCE_HEAP, mips, arrays, extent, size,
      format, name);
  }

  void recordBufferPlacedInUserResouceHeap(uint32_t size, bool is_gpu, const char *name)
  {
    if (!isCollectingMetric(Metric::BUFFERS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = is_gpu ? metricsAccess->currentFrame.rawCounters.placedGpuBufferHeaps
                          : metricsAccess->currentFrame.rawCounters.placedCpuBufferHeaps;
    target.count++;
    // target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::PLACE_BUFFER_HEAP_IN_USER_RESOURCE_HEAP, size, is_gpu, name);
  }

  void setup(const SetupInfo &setup);
  void shutdown();

  void preRecovery()
  {
    {
      *metrics.access() = {};
    }

    BaseType::preRecovery();
  }

#if _TARGET_PC_WIN
  struct CompletedFrameRecordingInfo : BaseType::CompletedFrameRecordingInfo
  {
    DXGIAdapter *adapter;
  };
#endif

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    {
      auto metricsAccess = metrics.access();
      if (isCollectingMetric(Metric::MEMORY_USE))
      {
#if _TARGET_XBOX
        size_t gameLimit = 0, gameUsed = 0;
        xbox_get_memory_status(gameUsed, gameLimit);
        metricsAccess->currentFrame.processMemoryInUse = gameUsed;
#else
        info.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &metricsAccess->currentFrame.deviceMemoryInUse);
        info.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &metricsAccess->currentFrame.systemMemoryInUse);

        PROCESS_MEMORY_COUNTERS_EX memUsage{sizeof(PROCESS_MEMORY_COUNTERS_EX)};
        GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memUsage), sizeof(memUsage));
        metricsAccess->currentFrame.processMemoryInUse = memUsage.PrivateUsage;
        // those values are just a snapshot of the current state, summing them up makes no sense
        metricsAccess->currentFrame.deviceMemoryInUsePeak =
          max(metricsAccess->currentFrame.deviceMemoryInUsePeak, metricsAccess->currentFrame.deviceMemoryInUse);
        metricsAccess->currentFrame.systemMemoryInUsePeak =
          max(metricsAccess->currentFrame.systemMemoryInUsePeak, metricsAccess->currentFrame.systemMemoryInUse);
#endif
        metricsAccess->currentFrame.processMemoryInUsePeak =
          max(metricsAccess->currentFrame.processMemoryInUsePeak, metricsAccess->currentFrame.processMemoryInUse);
      }

      if (isAnyCounterCollecting())
      {
        // update derived counters
        metricsAccess->currentFrame.netCounters.update(metricsAccess->currentFrame.rawCounters);
        metricsAccess->currentFrame.rawCountersPeak.update(metricsAccess->currentFrame.frameIndex,
          metricsAccess->currentFrame.rawCounters);
        metricsAccess->currentFrame.rawCountersSummary += metricsAccess->currentFrame.rawCounters;
        metricsAccess->currentFrame.netCountersPeak.update(metricsAccess->currentFrame.frameIndex,
          metricsAccess->currentFrame.netCounters);
      }

      if (isCollectingMetric(Metric::METRIC_LOG))
      {
        metricsAccess->frameHistory.push_back(metricsAccess->currentFrame);
      }

      ++metricsAccess->currentFrame.frameIndex;
      if (isAnyCounterCollecting())
      {
        // rawCounters are per frame so clear them out
        metricsAccess->currentFrame.rawCounters = {};
      }
    }

    BaseType::completeFrameRecording(info);
  }

  void recordScratchBufferAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::SCRATCH_BUFFER))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedScratchBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::SCRATCH_BUFFER_ALLOCATE, size, true);
  }

  void recordScratchBufferFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::SCRATCH_BUFFER))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedScratchBuffer;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::SCRATCH_BUFFER_FREE, size, true);
  }

  void recordScratchBufferTempUse(uint32_t size)
  {
    if (!isCollectingMetric(Metric::SCRATCH_BUFFER))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.scratchBufferTempUse;
    target.count++;
    target.size += size;
    // does not generate events, there are way too many
  }

  void recordScratchBufferPersistentUse(uint32_t size)
  {
    if (!isCollectingMetric(Metric::SCRATCH_BUFFER))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.scratchBufferPersistentUse;
    target.count++;
    target.size += size;
    // does not generate events, there are way too many
  }

  void recordRaytraceAccelerationStructureHeapAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedRaytraceAccelStructHeap;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_ACCEL_STRUCT_HEAP_ALLOCATE, size, true);
  }

  void recordRaytraceAccelerationStructureHeapFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedRaytraceAccelStructHeap;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_ACCEL_STRUCT_HEAP_FREE, size, true);
  }

  void recordRaytraceBottomStructureAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedRaytraceBottomStructure;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_BOTTOM_STRUCTURE_ALLOCATE, size, true);
  }

  void recordRaytraceBottomStructureFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedRaytraceBottomStructure;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_BOTTOM_STRUCTURE_FREE, size, true);
  }

  void recordRaytraceTopStructureAllocated(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.allocatedRaytraceTopStructure;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_TOP_STRUCTURE_ALLOCATE, size, true);
  }

  void recordRaytraceTopStructureFreed(uint32_t size)
  {
    if (!isCollectingMetric(Metric::RAYTRACING))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.freedRaytraceTopStructure;
    target.count++;
    target.size += size;

    recordBufferEvent(metricsAccess, MetricsState::ActionInfo::Type::RAYTRACE_TOP_STRUCTURE_FREE, size, true);
  }

public:
  void pushEvent(const char *begin, const char *end)
  {
    auto metricsAccess = metrics.access();
    metricsAccess->eventPathBacktrace.push_back(metricsAccess->curentEventPath.size());
    if (!metricsAccess->curentEventPath.empty())
    {
      metricsAccess->curentEventPath.push_back('/');
    }
    metricsAccess->curentEventPath.append(begin, end);
  }

  void popEvent()
  {
    auto metricsAccess = metrics.access();
    G_ASSERT(!metricsAccess->eventPathBacktrace.empty());
    auto prevEventPathLengh = !metricsAccess->eventPathBacktrace.empty() ? metricsAccess->eventPathBacktrace.back() : 0;
    metricsAccess->curentEventPath.resize(prevEventPathLengh);
    metricsAccess->eventPathBacktrace.pop_back();
  }

  // was_hit indicated that the underlying buffer could provide the discard memory
  void recordBufferDiscard(uint32_t size, bool was_hit)
  {
    if (!isCollectingMetric(Metric::BUFFERS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.discaredBufferMemory;
    target.count++;
    target.size += size;
    if (was_hit)
    {
      auto &target2 = metricsAccess->currentFrame.rawCounters.hitDiscardedBufferMemory;
      target2.count++;
      target2.size += size;
    }
  }

  // was_hit indicated that the underlying buffer could provide the discard memory
  void recordDriverBufferDiscard(uint32_t size, bool was_hit)
  {
    if (!isCollectingMetric(Metric::BUFFERS))
      return;
    auto metricsAccess = metrics.access();
    auto &target = metricsAccess->currentFrame.rawCounters.driverDiscardedBufferMemory;
    target.count++;
    target.size += size;
    if (was_hit)
    {
      auto &target2 = metricsAccess->currentFrame.rawCounters.hitDriverDiscardedBufferMemory;
      target2.count++;
      target2.size += size;
    }
  }
};
#else
class MetricsProvider : public MetricsProviderBase
{
  using BaseType = MetricsProviderBase;

protected:
  WinCritSec *getMetrixMutex() { return nullptr; }

  void recordHeapAllocated(uint32_t, bool) {}
  void recordHeapFreed(uint32_t, bool) {}
  void recordMemoryAllocated(uint32_t, bool) {}
  void recordMemoryFreed(uint32_t, bool) {}
  void recordBufferHeapAllocated(uint32_t, bool, const char *) {}
  void recordBufferHeapFreed(uint32_t, bool, const char *) {}
  void recordTextureAllocated(MipMapCount, ArrayLayerCount, const Extent3D &, uint32_t, FormatStore, const char *) {}
  void recordTextureAliased(MipMapCount, ArrayLayerCount, const Extent3D &, FormatStore, const char *) {}
  void recordTextureAdopted(MipMapCount, ArrayLayerCount, const Extent3D &, FormatStore, const char *) {}
  void recordTextureESRamAllocated(MipMapCount, ArrayLayerCount, const Extent3D &, uint32_t, FormatStore, const char *) {}
  void recordTextureFreed(MipMapCount, ArrayLayerCount, const Extent3D &, uint32_t, FormatStore, const char *) {}
  void recordConstantRingAllocated(uint32_t) {}
  void recordConstantRingFreed(uint32_t) {}
  void recordConstantRingUsed(uint32_t) {}
  void recordUploadRingAllocated(uint32_t) {}
  void recordUploadRingFreed(uint32_t) {}
  void recordUploadRingUsed(uint32_t) {}
  void recordTempBufferAllocated(uint32_t) {}
  void recordTempBufferFreed(uint32_t) {}
  void recordTempBufferUsed(uint32_t) {}
  void recordPersistentUploadBufferAllocated(uint32_t) {}
  void recordPersistentUploadBufferFreed(uint32_t) {}
  void recordPersistentUploadMemoryAllocated(uint32_t) {}
  void recordPersistentUploadMemoryFreed(uint32_t) {}
  void recordPersistentReadBackBufferAllocated(uint32_t) {}
  void recordPersistentReadBackBufferFreed(uint32_t) {}
  void recordPersistentReadBackMemoryAllocated(uint32_t) {}
  void recordPersistentReadBackMemoryFreed(uint32_t) {}
  void recordPersistentBidirectionalBufferAllocated(uint32_t) {}
  void recordPersistentBidirectionalBufferFreed(uint32_t) {}
  void recordPersistentBidirectionalMemoryAllocated(uint32_t) {}
  void recordPersistentBidirectionalMemoryFreed(uint32_t) {}
  void recordNewUserResourceHeap(size_t, bool) {}
  void recordDeletedUserResouceHeap(size_t, bool) {}
  void recordTexturePlacedInUserResourceHeap(MipMapCount, ArrayLayerCount, const Extent3D &, uint32_t, FormatStore, const char *) {}
  void recordBufferPlacedInUserResouceHeap(uint32_t, bool, const char *) {}
  // void setup(const SetupInfo &) {}
  // void shutdown() {}
  // void preRecovery() {}
  // void completeFrameRecording(const CompletedFrameRecordingInfo &) {}
  void recordScratchBufferAllocated(uint32_t) {}
  void recordScratchBufferFreed(uint32_t) {}
  void recordScratchBufferTempUse(uint32_t) {}
  void recordScratchBufferPersistentUse(uint32_t) {}
  void recordRaytraceAccelerationStructureHeapAllocated(uint32_t) {}
  void recordRaytraceAccelerationStructureHeapFreed(uint32_t) {}
  void recordRaytraceBottomStructureAllocated(uint32_t) {}
  void recordRaytraceBottomStructureFreed(uint32_t) {}
  void recordRaytraceTopStructureAllocated(uint32_t) {}
  void recordRaytraceTopStructureFreed(uint32_t) {}

public:
  void pushEvent(const char *, const char *) {}
  void popEvent() {}
  void recordBufferDiscard(uint32_t, bool) {}
  void recordDriverBufferDiscard(uint32_t, bool) {}
};
#endif

class GlobalSubresourceIdProvider : public MetricsProvider
{
  using BaseType = MetricsProvider;

  struct GlobalSubresourceIdProviderState
  {
    dag::Vector<BareBoneImageGlobalSubresourceIdRange> freeRanges;
    ImageGlobalSubresourceId nextFreeSubRes = first_dynamic_texture_global_id;

    void reset()
    {
      freeRanges.clear();
      nextFreeSubRes = first_dynamic_texture_global_id;
    }
  };
  ContainerMutexWrapper<GlobalSubresourceIdProviderState, OSSpinlock> idProviderState;

protected:
  GlobalSubresourceIdProvider() = default;
  ~GlobalSubresourceIdProvider() = default;
  GlobalSubresourceIdProvider(const GlobalSubresourceIdProvider &) = delete;
  GlobalSubresourceIdProvider &operator=(const GlobalSubresourceIdProvider &) = delete;
  GlobalSubresourceIdProvider(GlobalSubresourceIdProvider &&) = delete;
  GlobalSubresourceIdProvider &operator=(GlobalSubresourceIdProvider &&) = delete;

  void shutdown()
  {
    idProviderState.access()->reset();

    BaseType::shutdown();
  }

  void preRecovery()
  {
    idProviderState.access()->reset();

    BaseType::preRecovery();
  }

  uint32_t getGlobalSubresourceIdRangeLimit() { return idProviderState.access()->nextFreeSubRes.index(); }

  uint32_t getFreeGlobalSubresourceIdRangeValues()
  {
    auto stateAccess = idProviderState.access();
    return eastl::accumulate(stateAccess->freeRanges.cbegin(), stateAccess->freeRanges.cend(), 0u,
      [](auto sum, auto &range) { return sum + range.size(); });
  }

public:
  ImageGlobalSubresourceId getSwapchainColorGlobalId() const { return swapchain_color_texture_global_id; }

  ImageGlobalSubresourceId getSwapchainSecondaryColorGlobalId() const { return swapchain_secondary_color_texture_global_id; }

  ImageGlobalSubresourceId allocateGlobalResourceIdRange(SubresourceCount count)
  {
    auto stateAccess = idProviderState.access();

    auto range = free_list_allocate_smallest_fit<ImageGlobalSubresourceId>(stateAccess->freeRanges, count.count());
    if (range.empty())
    {
      auto r = stateAccess->nextFreeSubRes;
      stateAccess->nextFreeSubRes += count.count();
      return r;
    }
    else
    {
      return range.front();
    }
  }

  void freeGlobalResourceIdRange(ValueRange<ImageGlobalSubresourceId> range)
  {
    if (first_dynamic_texture_global_id > range.front())
      return;

    auto stateAccess = idProviderState.access();

    free_list_insert_and_coalesce(stateAccess->freeRanges, range);
  }
};
} // namespace drv3d_dx12::resource_manager
