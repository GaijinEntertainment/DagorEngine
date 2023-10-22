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
  CullJobSharedState *parent = 0;
  int jobIdx = 0;

public:
  void set(int job_idx, CullJobSharedState *parent_)
  {
    jobIdx = job_idx;
    parent = parent_;
  }

  static inline void perform_job(int job_idx, CullJobSharedState *info)
  {
    TIME_PROFILE(parallel_ri_cull_job);

    dag::ConstSpan<RendinstTiledScene> scenes = info->scenes;
    mat44f globtm = info->globtm;
    vec4f vpos_distscale = info->vpos_distscale;
    auto use_occlusion = info->use_occlusion;

    auto &v = *info->v;
    G_ASSERTF(v.forcedExtraLod < 0, "CullJob is expected to not have forcedExtraLod enabled.");
    auto &poolInfo = *info->poolInfo;
    const bool sortLarge = use_occlusion && check_occluders;
    std::atomic<uint32_t> *riexDataCnt = info->riexDataCnt;
    std::atomic<uint32_t> *riexLargeCnt = info->riexLargeCnt;

    for (int tiled_scene_idx = 0; tiled_scene_idx < scenes.size(); tiled_scene_idx++)
    {
      const auto &tiled_scene = scenes[tiled_scene_idx];
      scene::TiledSceneCullContext &ctx = info->sceneContexts[tiled_scene_idx];
      while (!ctx.tilesPassDone.load() || ctx.nextIdxToProcess.load(std::memory_order_relaxed) < ctx.tilesCount)
      {
        const int next_idx = ctx.nextIdxToProcess.fetch_add(1);
        spin_wait([&] { return next_idx >= ctx.tilesCount && !ctx.tilesPassDone.load(); });
        if (next_idx >= ctx.tilesCount)
          break;

        int tile_idx = ctx.tilesPtr[next_idx];
        // debug("  cull %d:[%d]=%d", tiled_scene_idx, next_idx, tile_idx);
        auto cb = [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
          static constexpr int LARGE_LOD_CNT = RiGenExtraVisibility::LARGE_LOD_CNT;
          G_UNUSED(ni);
          const scene::pool_index poolId = scene::get_node_pool(m);
          const auto &riPool = poolInfo[poolId];
          float dist = v_extract_x(distSqScaled);
          unsigned lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, dist);
          const unsigned llm = riPool.lodLimits >> ((ri_game_render_mode + 1) * 8);
          const unsigned min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
          if (lod > max_lod)
            return;
          lod = clamp(lod, min_lod, max_lod);
          int cnt_idx = poolId * rendinst::RiExtraPool::MAX_LODS + lod;
          int new_sz = riexDataCnt[cnt_idx].fetch_add(RIEXTRA_VECS_COUNT) + RIEXTRA_VECS_COUNT;
          if (new_sz > v.riexData[lod].data()[poolId].size())
          {
            if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
              riexLargeCnt[poolId * LARGE_LOD_CNT + lod]++;
            return;
          }

          vec4f *addData = v.riexData[lod].data()[poolId].data() + (new_sz - RIEXTRA_VECS_COUNT);
          const int32_t *userData = tiled_scene.getUserData(ni);
          const uint32_t curDistSq = interlocked_relaxed_load(*(const uint32_t *)(v.minSqDistances[lod].data() + poolId));
          const uint32_t distI = bitwise_cast<uint32_t>(dist); // since squared distance is positive, it is fine to compare in ints
                                                               // (bitwise representation of floats will still be in same order)
          if (distI < curDistSq)
          {
            // while we could make CAS min (in loop), or parallel sum (different arrays per thread), it is not important distance, and
            // used only for texture lod selection so, even a bit randomly it should work with relaxed load/store
            interlocked_relaxed_store(*(uint32_t *)(v.minSqDistances[lod].data() + poolId), distI);
          }
          if (userData)
            eastl::copy_n(userData, tiled_scene.getUserDataWordCount(), (uint32_t *)(addData + rendinst::render::ADDITIONAL_DATA_IDX));
          v_mat44_transpose_to_mat43(*(mat43f *)addData, m);
          uint32_t perDataBufferOffset = poolId * (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(vec4f)) + 1;
          vec4i extraData = v_make_vec4i(0, perDataBufferOffset, (poolId << 1) | uint32_t(riPool.isDynamic), 0);
          addData[rendinst::render::ADDITIONAL_DATA_IDX] =
            v_perm_ayzw(v_cast_vec4f(extraData), addData[rendinst::render::ADDITIONAL_DATA_IDX]);

          const uint32_t id = new_sz / RIEXTRA_VECS_COUNT - 1;
          vec4f rad = scene::get_node_bsphere_vrad(m);
          rad = v_div_x(distSqScaled, v_mul_x(rad, rad));
          float sdist = v_extract_x(rad);
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

        if (ctx.test_flags)
          tiled_scene.frustumCullOneTile<true, true, true>(ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
        else
          tiled_scene.frustumCullOneTile<false, true, true>(ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
      }
    }
  }
  virtual void doJob() override
  {
    if (!parent)
      return;
    perform_job(jobIdx, parent);
    parent = nullptr;
  }
};

} // namespace rendinst
