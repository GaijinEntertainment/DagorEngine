// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_threads.h>
#include <EASTL/vector.h>
#include <atomic>
#include <timeline.h>
#include "compiler_scratch_data.h"
#include <EASTL/unique_ptr.h>

namespace drv3d_vulkan
{

class ComputePipeline;
class GraphicsPipeline;
class TimelineManager;
class PipelineCompiler;

enum class PipelineCompileQueueItemType : uint8_t
{
  CS,
  GR
};

struct PipelineCompileQueueItem
{
  PipelineCompileQueueItemType type;
  union
  {
    ComputePipeline *cs;
    GraphicsPipeline *gr;
  };
  void compile();
  bool completed();
};

struct PipelineCompilerWork
{
  static constexpr size_t RING_SIZE = 128;
  eastl::vector<PipelineCompileQueueItem> queue;

  void init();
  void submit();
  void acquire(size_t timeline_abs_idx);
  void wait();
  void cleanup();
  void process();
  void shutdown();
};

struct PipelineCompileTimelineSync : public TimelineSyncPartLockFree,
                                     public TimelineSyncPartSingleWriterSingleReader,
                                     public TimelineSyncPartEventWaitable
{};

typedef Timeline<PipelineCompilerWork, PipelineCompileTimelineSync, PipelineCompilerWork::RING_SIZE> PipelineCompileTimeline;

class PipelineCompiler
{
  static constexpr size_t pipeline_compiler_stack = 256 << 10; // 512KB, 256KB is not enough on Radeon (yes, 256 is 512)
  struct PrimaryWorkerThread : public DaThread
  {
    PrimaryWorkerThread(PipelineCompiler &c) : DaThread("VkPipeComp", pipeline_compiler_stack), compiler(c) {}
    void execute() override;

  private:
    PipelineCompiler &compiler;
  };

  struct SecondaryWorkerThread : public DaThread
  {
    static const char *getWorkerName(int idx);

    SecondaryWorkerThread(PipelineCompiler &c, int idx) :
      DaThread(String(16, "VkPipeComp%u", idx), pipeline_compiler_stack), compiler(c)
    {
      os_event_create(&wakeEvent);
    }
    ~SecondaryWorkerThread() { os_event_destroy(&wakeEvent); }
    void wakeAndTerminate() { terminate(true /*wait*/, -1, &wakeEvent); }
    void execute() override;
    void wake() { os_event_set(&wakeEvent); }

  private:
    PipelineCompiler &compiler;
    os_event_t wakeEvent; //-V730_NOINIT
  };

  struct Config
  {
    bool disable;
    uint8_t maxSecondaryThreads;
    size_t secondarySpawnThreshold;
    size_t minItemsPerSecondary;
  };

  Config cfg = {false, 0, 0, 0};
  PrimaryWorkerThread primaryWorker;
  static constexpr uint8_t MAX_THREADS = 16;

  bool secondaryThreadsRunning = false;
  eastl::unique_ptr<SecondaryWorkerThread> secondaryWorkers[MAX_THREADS] = {};
  TimelineSpan<PipelineCompileTimeline> timeBlock;
  std::atomic<PipelineCompileQueueItem *> currentItem{nullptr};
  PipelineCompileQueueItem *endItem = nullptr;
  bool shouldWaitThreads = false;
  std::atomic<size_t> pendingWorkers{0};
  std::atomic<size_t> queueLength = 0;
  os_event_t secondaryWorkersFinishEvent; //-V730_NOINIT
  void loadConfig();

public:
  PipelineCompiler(TimelineManager &tl_man);
  ~PipelineCompiler() = default;
  PipelineCompiler(const PipelineCompiler &) = delete;

  void init();
  void shutdown();

  void queue(ComputePipeline *compute_pipe);
  void queue(GraphicsPipeline *graphics_pipe);
  void onItemCompiled();
  void waitFor(ComputePipeline *compute_pipe);
  void waitFor(GraphicsPipeline *graphics_pipe);
  void processQueuedBlocked();
  bool processQueued();

  size_t getQueueLength();

  void compileBlock(eastl::vector<PipelineCompileQueueItem> &block);
  void asyncCompileLoop();

  void startSecondaryWorkers();
  void setPendingWorkers(size_t v);
  void notifyWorkerCompleted();
  void waitSecondaryWorkers();
  void shutdownSecondaryWorkers();
};

} // namespace drv3d_vulkan
