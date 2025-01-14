// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include "render/genRender.h"
#include "riGen/rendInstTiledScene.h"
#include "visibility/extraVisibility.h"


struct RiGenExtraVisibility;
class Occlusion;

namespace rendinst
{

const float texStreamingLargeRiRad = 50;

struct CullJobSharedState
{
  mat44f globtm = {};
  vec4f vpos_distscale = {};
  Occlusion *use_occlusion = nullptr;
  RiGenExtraVisibility *v = nullptr;
  dag::ConstSpan<RendinstTiledScene> scenes;
  scene::TiledSceneCullContext *sceneContexts = nullptr;
  const eastl::vector<TiledScenePoolInfo> *poolInfo = nullptr;
  std::atomic<uint32_t> *riexDataCnt = nullptr;
  std::atomic<uint32_t> *riexLargeCnt = nullptr;
  SmallTab<Point2, framemem_allocator> *perPoolMinDist = nullptr;
  SmallTab<RiGenExtraVisibility::UVec2, framemem_allocator> *perPoolBestId = nullptr;
};

class CullJob final : public cpujobs::IJob
{
  const CullJobSharedState *parent = 0;
  int jobIdx = 0;

public:
  void set(int job_idx, const CullJobSharedState *parent_)
  {
    jobIdx = job_idx;
    parent = parent_;
  }

  static inline void perform_job(int job_idx, const CullJobSharedState *info)
  {
    if (job_idx == 0) // First job wakes the rest
      threadpool::wake_up_all();

    TIME_PROFILE(parallel_ri_cull_job);

    mat44f globtm = info->globtm;
    vec4f vpos_distscale = info->vpos_distscale;
    auto use_occlusion = info->use_occlusion;

    auto &v = *info->v;
    G_ASSERTF(v.forcedExtraLod < 0, "CullJob is expected to not have forcedExtraLod enabled.");
    auto &poolInfo = *info->poolInfo;
    const bool sortLarge = use_occlusion && check_occluders;
    std::atomic<uint32_t> *riexDataCnt = info->riexDataCnt;
    std::atomic<uint32_t> *riexLargeCnt = info->riexLargeCnt;

    [[maybe_unused]] int tiled_scene_idx = 0;
    const scene::TiledSceneCullContext *ctx = info->sceneContexts;
    for (const RendinstTiledScene &tiled_scene : info->scenes)
    {
      while (!ctx->tilesPassDone.load() || ctx->nextIdxToProcess.load(std::memory_order_relaxed) < ctx->tilesCount)
      {
        const int next_idx = ctx->nextIdxToProcess.fetch_add(1);
        spin_wait([&] { return next_idx >= ctx->tilesCount && !ctx->tilesPassDone.load(); });
        if (next_idx >= ctx->tilesCount)
          break;

        int tile_idx = ctx->tilesPtr[next_idx];
        // debug("  cull %d:[%d]=%d", tiled_scene_idx, next_idx, tile_idx);
        auto cb = [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
          static constexpr int LARGE_LOD_CNT = RiGenExtraVisibility::LARGE_LOD_CNT;
          G_UNUSED(ni);
          const scene::pool_index poolId = scene::get_node_pool(m);
          const auto &riPool = poolInfo[poolId];

          float poolRad = tiled_scene.getPoolSphereRad(poolId);
          float poolRad2 = poolRad * poolRad;
          float rad = scene::get_node_bsphere_rad(m);
          float rad2 = rad * rad;
          float sdist = v_extract_x(distSqScaled) / rad2;
          float distSqScaledNormalized = sdist * poolRad2; // = distSqScaled * (poolRad / rad)^2. Estimation of distSq where a
                                                           // non-scaled object of the pool has the same screensize as the node.

          const unsigned llm = riPool.lodLimits >> ((ri_game_render_mode + 1) * 8);
          const unsigned min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
          if (riPool.distSqLOD[max_lod] <= distSqScaledNormalized * rendinst::render::riExtraLodsShiftDistMulForCulling)
            return;

          unsigned lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, distSqScaledNormalized);
          lod = clamp(lod, min_lod, max_lod);
          int cnt_idx = poolId * rendinst::RiExtraPool::MAX_LODS + lod;
          int new_sz = riexDataCnt[cnt_idx].fetch_add(RIEXTRA_VECS_COUNT) + RIEXTRA_VECS_COUNT;
          if (new_sz > v.riexData[lod].data()[poolId].size())
          {
            if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
              riexLargeCnt[poolId * LARGE_LOD_CNT + lod]++;
            return;
          }

          if (rad > texStreamingLargeRiRad && distSqScaledNormalized > 0)
          {
            float texLevelDist = max(0.0f, sqrt(distSqScaledNormalized) - poolRad); // = (sqrt(distSqScaled) - rad) * (poolRad / rad)
            distSqScaledNormalized = texLevelDist * texLevelDist;
          }
          const uint32_t curDistSq = interlocked_relaxed_load(*(const uint32_t *)(v.minSqDistances[lod].data() + poolId));
          const uint32_t distI = bitwise_cast<uint32_t>(distSqScaledNormalized); // since squared distance is positive, it is fine to
                                                                                 // compare in ints (bitwise representation of floats
                                                                                 // will still be in same order)
          if (distI < curDistSq)
          {
            // while we could make CAS min (in loop), or parallel sum (different arrays per thread), it is not important distance, and
            // used only for texture lod selection so, even a bit randomly it should work with relaxed load/store
            interlocked_relaxed_store(*(uint32_t *)(v.minSqDistances[lod].data() + poolId), distI);
          }
          vec4f *addData = v.riexData[lod].data()[poolId].data() + (new_sz - RIEXTRA_VECS_COUNT);
          rendinst::render::write_ri_extra_per_instance_data(addData, tiled_scene, poolId, ni, m, riPool.isDynamic);

          const uint32_t id = new_sz / RIEXTRA_VECS_COUNT - 1;
          if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
          {
            cnt_idx = poolId * LARGE_LOD_CNT + lod;
            new_sz = riexLargeCnt[cnt_idx].fetch_add(1) + 1;
            if (new_sz > v.riexLarge[lod].data()[poolId].size())
              return;
            // this is almost as fast as using dist2 and is technically more correct.
            // however, since large occluders are usually not scaled, their radius is constant, and
            //  v_dot3_x(sphere, sort_dir) isn't that much different from projected dist
            // vec4f sphere = scene::get_node_bsphere(m);
            // float sdist = v_extract_x(v_sub(v_dot3_x(sphere, sort_dir), v_splat_w(sphere)));
            v.riexLarge[lod].data()[poolId][new_sz - 1] = {bitwise_cast<int, float>(sdist), id};
          }

          auto &minDist = info->perPoolMinDist->data()[poolId + poolInfo.size() * job_idx];
          auto &bestId = info->perPoolBestId->data()[poolId + poolInfo.size() * job_idx];
          if (sdist < minDist.x)
          {
            minDist.y = minDist.x;
            bestId.y = bestId.x;
            minDist.x = sdist;
            bestId.x = (id) | (lod << 28);
          }
          else if (sdist < minDist.y)
          {
            minDist.y = sdist;
            bestId.y = (id) | (lod << 28);
          }
        };

        if (ctx->test_flags)
          tiled_scene.frustumCullOneTile<true, true, true>(*ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
        else
          tiled_scene.frustumCullOneTile<false, true, true>(*ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
      }
      ctx++;

      G_FAST_ASSERT(ctx == &info->sceneContexts[++tiled_scene_idx]);
    }
  }
  void doJob() override { perform_job(jobIdx, parent); }
};

} // namespace rendinst
