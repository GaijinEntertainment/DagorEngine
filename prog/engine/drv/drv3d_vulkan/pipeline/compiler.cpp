// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compiler.h"
#include "main_pipelines.h"
#include "timelines.h"

using namespace drv3d_vulkan;

namespace
{
// keep compiler as global in module, to not have overhead on storing its pointer in multiple places
static PipelineCompiler *g_compiler = nullptr;
} // namespace

void PipelineCompileQueueItem::compile()
{
  switch (type)
  {
    case PipelineCompileQueueItemType::CS: cs->compile(); break;
    case PipelineCompileQueueItemType::GR: gr->compile(); break;
    default: G_ASSERTF(0, "vulkan: unknown compile queue item type %u", (uint8_t)type); break;
  }
  g_compiler->onItemCompiled();
}

bool PipelineCompileQueueItem::completed()
{
  switch (type)
  {
    case PipelineCompileQueueItemType::CS: return !is_null(cs->getCompiledHandle()); break;
    case PipelineCompileQueueItemType::GR: return !is_null(gr->getCompiledHandle()); break;
    default: G_ASSERTF(0, "vulkan: unknown compile queue item type %u", (uint8_t)type); break;
  }
  return false;
}

void PipelineCompilerWork::init() {}

void PipelineCompilerWork::submit() {}

void PipelineCompilerWork::acquire(size_t) {}

void PipelineCompilerWork::wait() { g_compiler->waitSecondaryWorkers(); }

void PipelineCompilerWork::cleanup() { queue.clear(); }

void PipelineCompilerWork::process() { g_compiler->compileBlock(queue); }

void PipelineCompilerWork::shutdown()
{
  queue.clear();
  queue.shrink_to_fit();
}

void PipelineCompiler::PrimaryWorkerThread::execute()
{
#if VULKAN_PROFILE_PIPELINE_COMPILER_THREADS
  TIME_PROFILE_THREAD(getCurrentThreadName());
#endif
  auto &compileQueue = Globals::timelines.get<PipelineCompileTimeline>();
  // replicate logic for android WORKER_THREADS_AFFINITY_USE
  applyThisThreadAffinity(cpujobs::get_core_count() >= 4 ? ~MAIN_THREAD_AFFINITY : ~0ull);
  while (!isThreadTerminating())
  {
    // wait for at least one work item to be processed
    if (compileQueue.waitSubmit(1, 1))
      compileQueue.advance();
  }
}

void PipelineCompiler::SecondaryWorkerThread::execute()
{
#if VULKAN_PROFILE_PIPELINE_COMPILER_THREADS
  TIME_PROFILE_THREAD(getCurrentThreadName());
#endif
  // replicate logic for android WORKER_THREADS_AFFINITY_USE
  applyThisThreadAffinity(cpujobs::get_core_count() >= 4 ? ~MAIN_THREAD_AFFINITY : ~0ull);

  // initial startup
  compiler.notifyWorkerCompleted();

  while (!isThreadTerminating())
  {
    if (os_event_wait(&wakeEvent, OS_WAIT_INFINITE) != OS_WAIT_OK)
      continue;

    compiler.asyncCompileLoop();
    compiler.notifyWorkerCompleted();
  }
}

void PipelineCompiler::loadConfig()
{
  int coreCount = cpujobs::get_core_count();
  const DataBlock *cfgBlk = Globals::cfg.getPerDriverPropertyBlock("pipelineCompiler");
  cfg.maxSecondaryThreads = min(coreCount, cfgBlk->getInt("maxSecondaryThreads", max(2, coreCount - 2)));
  cfg.maxSecondaryThreads = min(cfg.maxSecondaryThreads, MAX_THREADS);
  cfg.secondarySpawnThreshold = cfgBlk->getInt("secondarySpawnThreshold", 16);
  cfg.minItemsPerSecondary = cfgBlk->getInt("minItemsPerSecondary", 4);
  cfg.disable = cfgBlk->getBool("disable", false);
  debug("vulkan: pipeline compiler: %u threads %u secondary threshold %u min items per secondary", cfg.maxSecondaryThreads,
    cfg.secondarySpawnThreshold, cfg.minItemsPerSecondary);
}

PipelineCompiler::PipelineCompiler(TimelineManager &tl_man) : timeBlock(tl_man), primaryWorker(*this) { g_compiler = this; }

void PipelineCompiler::init()
{
  loadConfig();

  os_event_create(&secondaryWorkersFinishEvent);

  timeBlock.start();
  primaryWorker.start();
}

void PipelineCompiler::shutdown()
{
  processQueuedBlocked();
  primaryWorker.terminate(true /*wait*/);
  endItem = nullptr;
  currentItem = nullptr;
  if (secondaryThreadsRunning)
    shutdownSecondaryWorkers();

  os_event_destroy(&secondaryWorkersFinishEvent);

  cfg.maxSecondaryThreads = 0;
  auto &compileQueue = Globals::timelines.get<PipelineCompileTimeline>();
  while (compileQueue.waitSubmit(1, 1))
    compileQueue.advance();
  timeBlock.end();

  // mark compiler disabled, so we can keep using it for blocking compilations if requested
  cfg.disable = true;
}

void PipelineCompiler::queue(ComputePipeline *compute_pipe)
{
  if (cfg.disable)
  {
    compute_pipe->compile();
    return;
  }

  PipelineCompileQueueItem qi{PipelineCompileQueueItemType::CS};
  qi.cs = compute_pipe;
  timeBlock->queue.push_back(qi);
  ++queueLength;
}

void PipelineCompiler::queue(GraphicsPipeline *graphics_pipe)
{
  if (cfg.disable)
  {
    graphics_pipe->compile();
    return;
  }

  PipelineCompileQueueItem qi{PipelineCompileQueueItemType::GR};
  qi.gr = graphics_pipe;
  timeBlock->queue.push_back(qi);
  ++queueLength;
}

void PipelineCompiler::onItemCompiled() { --queueLength; }

void PipelineCompiler::waitFor(ComputePipeline *compute_pipe)
{
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  // if we waiting for CS pipeline that is async compiled - we doing something wrong in user code
  D3D_ERROR("vulkan: pipe compiler: blocked wait for CS %s", compute_pipe->printDebugInfoBuffered());
#endif
  TIME_PROFILE(vulkan_cs_pipe_wait);
  processQueuedBlocked();
  ComputePipeline *pipe = compute_pipe;
  spin_wait([pipe]() { return pipe->checkCompiled() == PipelineCompileStatus::PENDING; });
}

void PipelineCompiler::waitFor(GraphicsPipeline *graphics_pipe)
{
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  // if we waiting for graphics pipeline that is async compiled - we doing something wrong in user code
  // name is not known here unfortunately, but should be relatively visible in profiler
  D3D_ERROR("vulkan: pipe compiler: blocked wait for graphics pipeline");
#endif
  TIME_PROFILE(vulkan_gr_pipe_wait);
  processQueuedBlocked();
  GraphicsPipeline *pipe = graphics_pipe;
  spin_wait([pipe]() { return pipe->checkCompiled() == PipelineCompileStatus::PENDING; });
}

void PipelineCompiler::processQueuedBlocked()
{
  if (!processQueued())
  {
    TIME_PROFILE(vulkan_pipe_compiler_queue_block);
    auto &compileQueue = Globals::timelines.get<PipelineCompileTimeline>();
    constexpr size_t maxWaitCycles = 1000000;
    while (!compileQueue.waitAcquireSpace(maxWaitCycles))
      logwarn("vulkan: long pipe compiler wait");
    if (!processQueued())
      DAG_FATAL("vulkan: can't complete blocked wait for compiler timeline");
  }
}

bool PipelineCompiler::processQueued()
{
  if (cfg.disable || !timeBlock->queue.size())
    return true;

  auto &compileQueue = Globals::timelines.get<PipelineCompileTimeline>();
  if (!compileQueue.waitAcquireSpace(0))
    return false;
  timeBlock.restart();
  return true;
}

size_t PipelineCompiler::getQueueLength() { return queueLength.load(); }

void PipelineCompiler::compileBlock(dag::Vector<PipelineCompileQueueItem> &block)
{
  endItem = block.end();
  currentItem = block.begin();
  if (block.size() > cfg.secondarySpawnThreshold && cfg.maxSecondaryThreads)
  {
    if (!secondaryThreadsRunning)
      startSecondaryWorkers();

    TIME_PROFILE(vulkan_pipe_compiler_wake);
    size_t threadsToStart = min<size_t>(block.size() / cfg.minItemsPerSecondary, cfg.maxSecondaryThreads);
    setPendingWorkers(threadsToStart);
    for (int i = 0; i < threadsToStart; ++i)
      secondaryWorkers[i]->wake();
  }
  else
    asyncCompileLoop();
}

void PipelineCompiler::asyncCompileLoop()
{
  TIME_PROFILE(vulkan_pipe_compiler_loop);
  PipelineCompileQueueItem *acquiredItem = currentItem.fetch_add(1);
  PipelineCompileQueueItem *end = endItem;
  while (acquiredItem < end)
  {
    G_ASSERTF(!acquiredItem->completed(), "vulkan: trying to compile already completed item");
    acquiredItem->compile();
    acquiredItem = currentItem.fetch_add(1);
  }
}

void PipelineCompiler::startSecondaryWorkers()
{
  setPendingWorkers(cfg.maxSecondaryThreads);
  for (int i = 0; i < cfg.maxSecondaryThreads; ++i)
  {
    secondaryWorkers[i] = eastl::make_unique<SecondaryWorkerThread>(*this, i);
    secondaryWorkers[i]->start();
  }
  // wait threads to start
  waitSecondaryWorkers();
  secondaryThreadsRunning = true;
}

void PipelineCompiler::notifyWorkerCompleted()
{
  if (--pendingWorkers == 0)
    os_event_set(&secondaryWorkersFinishEvent);
}

void PipelineCompiler::setPendingWorkers(size_t v)
{
  if (!v)
    return;
  pendingWorkers = v;
  shouldWaitThreads = true;
}

void PipelineCompiler::waitSecondaryWorkers()
{
  if (!shouldWaitThreads)
    return;

  TIME_PROFILE(vulkan_pipe_compiler_wait_secondary);
  if (os_event_wait(&secondaryWorkersFinishEvent, OS_WAIT_INFINITE) != OS_WAIT_OK)
    DAG_FATAL("vulkan: pipe compiler secondary wait failed");
  shouldWaitThreads = false;
}

void PipelineCompiler::shutdownSecondaryWorkers()
{
  setPendingWorkers(cfg.maxSecondaryThreads);
  for (int i = 0; i < cfg.maxSecondaryThreads; ++i)
    secondaryWorkers[i]->wakeAndTerminate();
  // do not wait as we already waited on terminate
  // and event may be skipped as thread can exit before entering wake event wait
  pendingWorkers = 0;
  shouldWaitThreads = false;

  for (int i = 0; i < cfg.maxSecondaryThreads; ++i)
    secondaryWorkers[i].reset();
  secondaryThreadsRunning = false;
}