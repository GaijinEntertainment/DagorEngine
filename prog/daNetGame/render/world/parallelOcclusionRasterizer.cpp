// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "parallelOcclusionRasterizer.h"
#include <generic/dag_staticTab.h>


ParallelOcclusionRasterizer::ParallelOcclusionRasterizer(uint32_t width, uint32_t height)
{
  // Note: -1 for readback_hzb/occlusion_reproject/fx_start_update job
  usedOcclusions = min((int)occlusionTargets.size(), max(threadpool::get_num_workers() - 1, 1));
  for (uint32_t i = 0; i < usedOcclusions; ++i)
  {
    occlusionTargets[i].reset(MaskedOcclusionCulling::Create());
    occlusionTargets[i]->SetResolution(width, height);
    rasterJobs[i].occlusion = occlusionTargets[i].get();
    clearJobs[i].occlusion = occlusionTargets[i].get();
    rasterJobs[i].context = &rasterizersContext;
  }

  if (threadpool::get_num_workers() <= 1) // Not enough workers to give precedence to `visibility_reproject_job`
    rasterJobs[0].prio = threadpool::PRIO_NORMAL;
}

void ParallelOcclusionRasterizer::rasterizeMeshes(eastl::vector<RasterizationTaskData> &&tasks, float z_near)
{
  if (tasks.empty())
    return;
  TIME_PROFILE(run_raster_jobs)

  // Store collection of jobs
  int maxJobs = max(threadpool::get_num_workers() - 1, 1); // -1 either for `readback_hzb` or `occlusion_reproject`
  usedOcclusions = eastl::min({tasks.size(), occlusionTargets.size(), (eastl_size_t)maxJobs});
  rasterizersContext.tasks.swap(tasks);
  rasterizersContext.processedTasks.store(0);

  for (uint32_t i = 0; i < usedOcclusions; ++i)
    occlusionTargets[i]->SetNearClipPlane(z_near);

  // Run rasterization jobs
  for (uint32_t i = 0; i < usedOcclusions; ++i)
    rasterJobs[i].start(rasterJobsTPQPos);
}


uint32_t ParallelOcclusionRasterizer::mergeRasterizationResults(Occlusion &occlusion)
{
  for (uint32_t i = 0; i < usedOcclusions; ++i)
  {
#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
    if (interlocked_acquire_load(rasterJobs[i].done))
      continue;
    TIME_PROFILE_DEV(wait_raster_jobs);
    for (; i < usedOcclusions; ++i)
#endif
      if (rasterJobs[i].prio == threadpool::PRIO_LOW) // To avoid executing long running tasks like PFUD, etc...
        threadpool::wait(&rasterJobs[i]);
      else
        threadpool::barrier_active_wait_for_job(&rasterJobs[i], rasterJobs[i].prio, rasterJobsTPQPos);
  }

  StaticTab<MaskedOcclusionCulling *, MAX_JOBS_COUNT> nonEmptyOcclusions;
  uint32_t triangles = 0;
  for (uint32_t i = 0; i < usedOcclusions; ++i)
    if (rasterJobs[i].trianglesCount)
    {
      nonEmptyOcclusions.push_back(rasterJobs[i].occlusion);
      triangles += rasterJobs[i].trianglesCount;
    }

  // Run merge jobs
  if (!nonEmptyOcclusions.empty())
  {
    TIME_PROFILE(merge_occlusion_master)

    mergedTiles.store(0u, std::memory_order_relaxed);
    const int quant = 256; // around 64 actual jobs
    uint32_t tilesCount = nonEmptyOcclusions[0]->getTilesCount(), tpqpos = 0;
    int nMergeJobs = clamp(threadpool::get_num_workers(), 1, MAX_JOBS_COUNT);
    MergingJob *mergeJobs = (MergingJob *)alloca(nMergeJobs * sizeof(MergingJob));
    for (int i = 0; i < nMergeJobs; ++i)
    {
      new (&mergeJobs[i], _NEW_INPLACE)
        MergingJob(occlusion, nonEmptyOcclusions.data(), nonEmptyOcclusions.size(), tilesCount, quant, mergedTiles);
      if (i)
        threadpool::add(&mergeJobs[i], threadpool::PRIO_NORMAL, tpqpos, threadpool::AddFlags::None);
    }
    threadpool::wake_up_all();
    mergeJobs[0].doJob();
    {
      TIME_PROFILE_DEV(wait_merge_occlusion);
      threadpool::barrier_active_wait_for_job(&mergeJobs[nMergeJobs - 1], threadpool::PRIO_NORMAL, tpqpos); // Wait for last
      for (int i = nMergeJobs - 2; i >= 0; --i)
        threadpool::wait(&mergeJobs[i]);
    }

    occlusion.finalizeSWOcclusionMerge();
  }

  return triangles;
}
