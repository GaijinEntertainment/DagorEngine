// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>
#include <cstdint>

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

#include <3d/dag_maskedOcclusionCulling.h>
#include <scene/dag_occlusion.h>
#include <math/dag_frustum.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_threadPool.h>
#include <vecmath/dag_vecMathDecl.h>

class Occlusion;

class ParallelOcclusionRasterizer
{
public:
  struct RasterizationTaskData
  {
    mat44f viewproj;
    vec3f bmin;
    vec3f bmax;
    const float *verts;
    const uint16_t *faces;
    uint32_t tri_count;
  };

private:
  static constexpr int MAX_JOBS_COUNT = 6;
  struct MOCDeleter
  {
    void operator()(MaskedOcclusionCulling *p) { p ? MaskedOcclusionCulling::Destroy(p) : ((void)0); }
  };
  eastl::array<eastl::unique_ptr<MaskedOcclusionCulling, MOCDeleter>, MAX_JOBS_COUNT> occlusionTargets;

  struct TasksContext
  {
    std::atomic<uint32_t> processedTasks;
    eastl::vector<RasterizationTaskData> tasks;
  } rasterizersContext;
  eastl::vector<RasterizationTaskData> tasksNext;

  struct RasterizationJob final : public cpujobs::IJob
  {
    // Note: low prio to give `visibility_reproject_job` precedence (as it starts later but takes more time)
    static inline auto prio = threadpool::PRIO_LOW;
    MaskedOcclusionCulling *occlusion = nullptr;
    TasksContext *context = nullptr;
    uint32_t trianglesCount = 0;
    void start(uint32_t &qpos)
    {
      trianglesCount = 0;
      threadpool::add(this, prio, qpos, threadpool::AddFlags::None);
    }
    virtual void doJob() override
    {
      TIME_PROFILE(rasterizer_all_tasks)
      uint32_t taskId = context->processedTasks.fetch_add(1);
      while (taskId < context->tasks.size())
      {
        const RasterizationTaskData &task = context->tasks[taskId];
        Frustum frustum(task.viewproj);
        int ret = frustum.testBox(task.bmin, task.bmax);
        if (ret)
        {
          auto clipPlanes =
            (ret == Frustum::INTERSECT) ? MaskedOcclusionCulling::CLIP_PLANE_ALL : MaskedOcclusionCulling::CLIP_PLANE_NONE;
          occlusion->RenderTriangles(task.verts, task.faces, task.tri_count, (float *)&task.viewproj,
            MaskedOcclusionCulling::BACKFACE_CW, clipPlanes);
          trianglesCount += task.tri_count;
        }
        taskId = context->processedTasks.fetch_add(1);
      }
    }
  };

  struct ClearingJob final : public cpujobs::IJob
  {
    // Low prio to give priority to `fx_start_update` job which is added with normal prio
    static constexpr auto prio = threadpool::PRIO_LOW;
    MaskedOcclusionCulling *occlusion = nullptr;
    void start(uint32_t &qpos) { threadpool::add(this, prio, qpos, threadpool::AddFlags::None); }
    virtual void doJob() override
    {
      TIME_PROFILE_DEV(clear_rasterizer_occlusion);
      occlusion->ClearBuffer();
    }
  };

  struct MergingJob final : public cpujobs::IJob
  {
    Occlusion *targetOcclusion = nullptr;
    MaskedOcclusionCulling **occlusions = nullptr;
    uint32_t occlusionsCount = 0;
    uint32_t initialTile = 0;
    uint32_t endTile = 0;
    uint32_t quant = 256;
    std::atomic<uint32_t> &mergedTiles;

    MergingJob(Occlusion &target,
      MaskedOcclusionCulling **occl,
      uint32_t occl_count,
      uint32_t end_tile,
      uint32_t quant_,
      std::atomic<uint32_t> &t) :
      mergedTiles(t)
    {
      targetOcclusion = &target;
      occlusions = occl;
      occlusionsCount = occl_count;
      endTile = end_tile;
      quant = quant_;
      initialTile = mergedTiles.fetch_add(quant, std::memory_order_relaxed);
    }
    virtual void doJob() override
    {
      TIME_PROFILE(merge_occlusion)
      for (uint32_t startTile = initialTile; startTile < endTile; startTile = mergedTiles.fetch_add(quant))
        targetOcclusion->mergeOcclusionsZmin(occlusions, occlusionsCount, startTile,
          startTile + eastl::min(quant, endTile - startTile));
    }
  };

  eastl::array<RasterizationJob, MAX_JOBS_COUNT> rasterJobs;
  eastl::array<ClearingJob, MAX_JOBS_COUNT> clearJobs;
  uint32_t usedOcclusions = 0;
  uint32_t rasterJobsTPQPos = 0;
  uint32_t clearJobsTPQPos = 0;
  std::atomic<uint32_t> mergedTiles;

public:
  ParallelOcclusionRasterizer(uint32_t width, uint32_t height);
  // Send meshes on rasterization
  void rasterizeMeshes(eastl::vector<RasterizationTaskData> &&tasks, float z_near);
  // Merge rasterized geometry
  uint32_t mergeRasterizationResults(Occlusion &occlusion);

  void waitClearJobs()
  {
    threadpool::barrier_active_wait_for_job(&clearJobs[max((int)usedOcclusions - 1, 0)], clearJobs[0].prio, clearJobsTPQPos);
    for (uint32_t i = 0; i < usedOcclusions; ++i)
      threadpool::wait(&clearJobs[i]);
  }

  eastl::vector<RasterizationTaskData> &acquireTasksNext()
  {
    waitClearJobs();
    tasksNext.clear();
    return tasksNext;
  }

  void runCleaners()
  {
    waitClearJobs();
    for (uint32_t i = 0; i < usedOcclusions; ++i)
      clearJobs[i].start(clearJobsTPQPos);
  }
};
