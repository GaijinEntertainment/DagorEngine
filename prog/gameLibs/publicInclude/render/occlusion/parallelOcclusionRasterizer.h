//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_function.h>

#include <3d/dag_maskedOcclusionCulling.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <scene/dag_occlusion.h>
#include <math/dag_frustum.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_threadPool.h>
#include <vecmath/dag_vecMathDecl.h>

class Occlusion;

class ParallelOcclusionRasterizer
{
public:
  struct RasterizationTaskData //-V730
  {
    mat44f viewproj;
    vec3f bmin;
    vec3f bmax;
    // Two modes, picked at job time:
    //   1) Legacy verts/faces: caller supplies a vertex array + uint16 index buffer for the slice
    //      (callers that already have a raw triangle soup).
    //   2) BLAS: caller supplies a pointer to vert21-packed BLAS data and a tree slice (treeStart,
    //      treeEnd). The job calls RenderBLAS, which walks the subtree, frustum-culls per BVH node,
    //      and emits triangle indices into a job-local cache RenderVert21TriList consumes. Used by
    //      the CollisionResource BLAS feeder. blasData must outlive rasterizeMeshes -- safe because
    //      CollisionResource grids are persistent.
    //   verts != nullptr selects mode 1; verts == nullptr && blasData != nullptr selects mode 2.
    // tri_count is the exact triangle count for the slice in both modes; in mode 2 it is RenderBLAS's
    // triLimit cap. triSkip is the BLAS-order triangle offset for mode 2, non-zero only for the 2nd+
    // slice of a BLAS partitioned by triangles_partition. doJob also uses tri_count to decide between
    // the per-job static index cache and a framemem-backed buffer.
    const float *verts = nullptr;
    const uint16_t *faces = nullptr;
    uint32_t tri_count = 0;
    const uint8_t *blasData = nullptr;
    uint32_t vertOffset = 0; // mode 2: byte offset of the vert21 stream within blasData (gather base = blasData + vertOffset)
    uint32_t treeStart = 0;
    uint32_t treeEnd = 0;
    uint32_t triSkip = 0;
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
    dag::AtomicInteger<uint32_t> processedTasks;
    dag::AtomicInteger<uint32_t> remainingJobs;
    eastl::vector<RasterizationTaskData> tasks;
    MaskedOcclusionCulling::BackfaceWinding backfaceWinding;
    eastl::fixed_function<sizeof(void *) * 3, void()> allDoneCb;
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
      // Note: would be wake-up either by wake_up_all in `animchar_before_render_es` or `bvh_start_before_render_jobs_es`
      threadpool::add(this, prio, qpos, threadpool::AddFlags::None);
    }
    const char *getJobName(bool &) const override { return "rasterizer_all_tasks"; }
    // Per-job triangle index cache, reused across BLAS-mode tasks. The feeder partitions tasks to
    // triangles_partition slices so tri_count normally fits this static cap; a single unpartitioned
    // BLAS slice could exceed it and falls back to a framemem-backed spill.
    static constexpr int BLAS_TRI_CACHE_MAX = 4096;
    alignas(16) uint32_t blasTriCache[BLAS_TRI_CACHE_MAX * 3];

    virtual void doJob() override
    {
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
          if (task.verts)
          {
            // Legacy verts/faces path (raw triangle soup).
            occlusion->RenderTriangles(task.verts, task.faces, task.tri_count, (float *)&task.viewproj, context->backfaceWinding,
              clipPlanes);
            trianglesCount += task.tri_count;
          }
          else if (task.blasData)
          {
            // BLAS-walk path (combined-per-behavior CollisionResource BLAS). RenderBLAS frustum-culls
            // and emits triangles directly to RenderVert21TriList -- no caller-side vert/face arrays,
            // and blasData is persistent so it satisfies the async task lifetime a decoded scratch
            // could not. CACHE_INSUFFICIENT = walk-and-emit on this call; triSkip/tri_count select
            // this slice of the partitioned BLAS triangle range.
            uint32_t triCount = 0;
            uint32_t *indexCache = blasTriCache;
            uint32_t indexCacheCap = (uint32_t)BLAS_TRI_CACHE_MAX;
            dag::Vector<uint32_t, framemem_allocator, false> spillCache;
            if (task.tri_count > indexCacheCap)
            {
              spillCache.resize(task.tri_count * 3);
              indexCache = spillCache.data();
              indexCacheCap = task.tri_count;
            }
            occlusion->RenderBLAS(task.blasData, (int)task.vertOffset, (int)task.treeStart, (int)task.treeEnd, (float *)&task.viewproj,
              indexCache, indexCacheCap, triCount, MaskedOcclusionCulling::CACHE_INSUFFICIENT, context->backfaceWinding, clipPlanes,
              task.triSkip, task.tri_count);
            trianglesCount += triCount;
          }
        }
        taskId = context->processedTasks.fetch_add(1);
      }
      if (context->allDoneCb && context->remainingJobs.sub_fetch(1) == 0)
        context->allDoneCb();
    }
  };

  struct ClearingJob final : public cpujobs::IJob
  {
    // Low prio to give priority to `fx_start_update` job which is added with normal prio
    static constexpr auto prio = threadpool::PRIO_LOW;
    MaskedOcclusionCulling *occlusion = nullptr;
    void start(uint32_t &qpos) { threadpool::add(this, prio, qpos, threadpool::AddFlags::None); }
    void doJob() override { occlusion->ClearBuffer(); }
    const char *getJobName(bool &) const override { return "clear_rasterizer_occlusion"; };
  };

  struct MergingJob final : public cpujobs::IJob
  {
    Occlusion *targetOcclusion = nullptr;
    MaskedOcclusionCulling **occlusions = nullptr;
    uint32_t occlusionsCount = 0;
    uint32_t initialTile = 0;
    uint32_t endTile = 0;
    uint32_t quant = 256;
    dag::AtomicInteger<uint32_t> &mergedTiles;

    MergingJob(Occlusion &target, MaskedOcclusionCulling **occl, uint32_t occl_count, uint32_t end_tile, uint32_t quant_,
      dag::AtomicInteger<uint32_t> &t) :
      mergedTiles(t)
    {
      targetOcclusion = &target;
      occlusions = occl;
      occlusionsCount = occl_count;
      endTile = end_tile;
      quant = quant_;
      initialTile = mergedTiles.fetch_add(quant, dag::memory_order_relaxed);
    }
    const char *getJobName(bool &) const override { return "merge_occlusion"; }
    virtual void doJob() override
    {
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
  dag::AtomicInteger<uint32_t> mergedTiles;

public:
  ParallelOcclusionRasterizer(uint32_t width, uint32_t height);
  // Send meshes on rasterization
  void rasterizeMeshes(eastl::vector<RasterizationTaskData> &&tasks, float z_near,
    MaskedOcclusionCulling::BackfaceWinding backface_winding = MaskedOcclusionCulling::BACKFACE_CW,
    eastl::fixed_function<sizeof(void *) * 3, void()> &&done_cb = {});
  // Merge rasterized geometry
  uint32_t mergeRasterizationResults(Occlusion &occlusion, bool active_wait = true);

  void waitRasterizeJobs()
  {
    threadpool::barrier_active_wait_for_job(&rasterJobs[max((int)usedOcclusions - 1, 0)], rasterJobs[0].prio, rasterJobsTPQPos);
    for (uint32_t i = 0; i < usedOcclusions; ++i)
      threadpool::wait(&rasterJobs[i]);
  }

  void waitClearJobs()
  {
    threadpool::barrier_active_wait_for_job(&clearJobs[max((int)usedOcclusions - 1, 0)], clearJobs[0].prio, clearJobsTPQPos);
    for (uint32_t i = 0; i < usedOcclusions; ++i)
      threadpool::wait(&clearJobs[i]);
  }

  void waitAllJobs()
  {
    waitRasterizeJobs();
    waitClearJobs();
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
