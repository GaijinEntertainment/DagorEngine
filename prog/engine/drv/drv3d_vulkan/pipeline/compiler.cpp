#include "compiler.h"
#include "main_pipelines.h"
#include "timelines.h"
#include "device.h"

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
    case PipelineCompileQueueItemType::CS: return cs->checkCompiled(); break;
    case PipelineCompileQueueItemType::GR: return gr->checkCompiled(); break;
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
  TIME_PROFILE_THREAD(getCurrentThreadName());
  compiler.startSecondaryWorkers();
  auto &compileQueue = get_device().timelineMan.get<PipelineCompileTimeline>();
  while (!interlocked_acquire_load(terminating))
  {
    // wait for at least one work item to be processed
    if (compileQueue.waitSubmit(1, 1))
      compileQueue.advance();
  }
}

void PipelineCompiler::SecondaryWorkerThread::execute()
{
  TIME_PROFILE_THREAD(getCurrentThreadName());

  // initial startup
  compiler.notifyWorkerCompleted();

  while (!interlocked_acquire_load(terminating))
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
  const DataBlock *cfgBlk = get_device().getPerDriverPropertyBlock("pipelineCompiler");
  cfg.maxSecondaryThreads = min(coreCount, cfgBlk->getInt("maxSecondaryThreads", max(2, coreCount - 2)));
  cfg.maxSecondaryThreads = min(cfg.maxSecondaryThreads, MAX_THREADS);
  cfg.secondarySpawnThreshold = cfgBlk->getInt("secondarySpawnThreshold", 16);
  cfg.minItemsPerSecondary = cfgBlk->getInt("minItemsPerSecondary", 4);
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
  primaryWorker.setAffinity(-1);
}

void PipelineCompiler::shutdown()
{
  processQueuedBlocked();
  primaryWorker.terminate(true /*wait*/);
  shutdownSecondaryWorkers();

  os_event_destroy(&secondaryWorkersFinishEvent);

  cfg.maxSecondaryThreads = 0;
  auto &compileQueue = get_device().timelineMan.get<PipelineCompileTimeline>();
  while (compileQueue.waitSubmit(1, 1))
    compileQueue.advance();
  timeBlock.end();
}

void PipelineCompiler::queue(ComputePipeline *compute_pipe)
{
  PipelineCompileQueueItem qi{PipelineCompileQueueItemType::CS};
  qi.cs = compute_pipe;
  timeBlock->queue.push_back(qi);
  ++queueLength;
}

void PipelineCompiler::queue(GraphicsPipeline *graphics_pipe)
{
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
  logerr("vulkan: pipe compiler: blocked wait for CS %s", compute_pipe->printDebugInfoBuffered());
#endif
  TIME_PROFILE(vulkan_cs_pipe_wait);
  processQueuedBlocked();
  ComputePipeline *pipe = compute_pipe;
  spin_wait([pipe]() { return !pipe->checkCompiled(); });
}

void PipelineCompiler::waitFor(GraphicsPipeline *graphics_pipe)
{
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  // if we waiting for graphics pipeline that is async compiled - we doing something wrong in user code
  // name is not known here unfortunately, but should be relatively visible in profiler
  logerr("vulkan: pipe compiler: blocked wait for graphics pipeline");
#endif
  TIME_PROFILE(vulkan_gr_pipe_wait);
  processQueuedBlocked();
  GraphicsPipeline *pipe = graphics_pipe;
  spin_wait([pipe]() { return !pipe->checkCompiled(); });
}

void PipelineCompiler::processQueuedBlocked()
{
  if (!processQueued())
  {
    TIME_PROFILE(vulkan_pipe_compiler_queue_block);
    auto &compileQueue = get_device().timelineMan.get<PipelineCompileTimeline>();
    constexpr size_t maxWaitCycles = 1000000;
    while (!compileQueue.waitAcquireSpace(maxWaitCycles))
      logwarn("vulkan: long pipe compiler wait");
    if (!processQueued())
      fatal("vulkan: can't complete blocked wait for compiler timeline");
  }
}

bool PipelineCompiler::processQueued()
{
  if (!timeBlock->queue.size())
    return true;

  auto &compileQueue = get_device().timelineMan.get<PipelineCompileTimeline>();
  if (!compileQueue.waitAcquireSpace(0))
    return false;
  timeBlock.restart();
  return true;
}

size_t PipelineCompiler::getQueueLength() { return queueLength.load(); }

void PipelineCompiler::compileBlock(eastl::vector<PipelineCompileQueueItem> &block)
{
  endItem = block.end();
  currentItem = block.begin();
  if (block.size() > cfg.secondarySpawnThreshold && cfg.maxSecondaryThreads)
  {
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
    secondaryWorkers[i]->setAffinity(-1);
  }
  // wait threads to start
  waitSecondaryWorkers();
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
    fatal("vulkan: pipe compiler secondary wait failed");
  shouldWaitThreads = false;
}

void PipelineCompiler::shutdownSecondaryWorkers()
{
  endItem = nullptr;
  currentItem = nullptr;
  setPendingWorkers(cfg.maxSecondaryThreads);
  for (int i = 0; i < cfg.maxSecondaryThreads; ++i)
    secondaryWorkers[i]->wakeAndTerminate();
  waitSecondaryWorkers();
  for (int i = 0; i < cfg.maxSecondaryThreads; ++i)
    secondaryWorkers[i].reset();
}