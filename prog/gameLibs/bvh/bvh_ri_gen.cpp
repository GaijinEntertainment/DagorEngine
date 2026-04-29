// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <rendInst/rendInstAccess.h>
#include <generic/dag_enumerate.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_bitwise_cast.h>
#include <util/dag_finally.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_gen_range_enable;
extern float bvh_ri_gen_range;
#endif

#if 0
CONSOLE_BOOL_VAL("render", stationary_rt_trees, false);
#else
#define stationary_rt_trees true
#endif

#define ENABLE_HAS_COLLISION_DEBUG 0

#if ENABLE_HAS_COLLISION_DEBUG
#include <mutex>

struct TreeMappingData
{
  Point4 pos;
  uint64_t meshId;
  rendinst::riex_handle_t handle;
  uint64_t bvhId;
  TMatrix tm;
};

static std::mutex treeMappingDebugMutex;
static eastl::unordered_map<uint64_t, TreeMappingData> treeMappingDebug[bvh::Context::maxUniqueLods];

uint64_t tree_mapping_debug_key = 0;

void start_new_tree_mapping_debug_frame()
{
  std::lock_guard<std::mutex> lock(treeMappingDebugMutex);
  for (auto &bucket : treeMappingDebug)
    bucket.clear();
}
void check_tree_mapping_debug(uint64_t key, uint64_t object_id, const Point4 &pos, rendinst::riex_handle_t handle, int lod_ix,
  uint64_t bvh_id)
{
  if (key == tree_mapping_debug_key)
    logdbg("");

  std::lock_guard<std::mutex> lock(treeMappingDebugMutex);

  auto &bucket = treeMappingDebug[lod_ix];
  auto iter = bucket.find(key);
  if (iter != bucket.end())
    logerr("[BVH] hash collision!");
  else
    bucket[key] = {pos, object_id, handle, bvh_id};
}
#else
void start_new_tree_mapping_debug_frame() {}
void check_tree_mapping_debug(uint64_t, uint64_t, const Point4 &, rendinst::riex_handle_t, int, uint64_t) {}
#endif

namespace bvh::ri
{

struct DECLSPEC_ALIGN(16) IPoint4_vec4 : public IPoint4
{
} ATTRIBUTE_ALIGN(16);

template <map_tree_fn mapper>
static bool handle_leaves(ContextId context_id, uint64_t object_id, int lod_ix, mat44f_cref tm, rendinst::riex_handle_t handle,
  eastl::optional<TMatrix4> &inv_world_tm, LeavesInfo &leavesInfo, MeshMetaAllocator::AllocId &metaAllocId, void *user_data)
{
  if (!inv_world_tm.has_value())
  {
    inv_world_tm = TMatrix4::IDENT;

    mat44f inv44;
    v_mat44_inverse(inv44, tm);
    v_mat44_transpose(inv44, inv44);
    v_stu(&inv_world_tm.value().row[0], inv44.col0);
    v_stu(&inv_world_tm.value().row[1], inv44.col1);
    v_stu(&inv_world_tm.value().row[2], inv44.col2);
    v_stu(&inv_world_tm.value().row[3], inv44.col3);
  }

  int dummy;
  auto data = mapper(context_id, object_id, tm.col3, handle, lod_ix, user_data, leavesInfo.recycled, dummy);

  if (!data)
  {
    auto buffers = context_id->stationaryTreeBuffers.find(object_id);
    if (buffers == context_id->stationaryTreeBuffers.end())
      return false;

    leavesInfo.recycled = false;
    leavesInfo.stationary = true;
    data = &buffers->second;
  }
  else
    leavesInfo.stationary = false;

  if (data->metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
    data->metaAllocId = context_id->allocateMetaRegion(1);

  metaAllocId = data->metaAllocId;

  leavesInfo.invWorldTm = inv_world_tm.value();
  leavesInfo.transformedBuffer = &data->buffer;
  leavesInfo.transformedBlas = &data->blas;

  return true;
}

static struct RiGenBVHJob : public cpujobs::IJob
{
  struct AccelStruct
  {
    dag::Vector<uint32_t> accel1;
    dag::Vector<uint64_t> accel2;

    volatile int accel1Cursor;
    volatile int accel2Cursor;
  };
  inline static AccelStruct accels[2];

  vec4f viewPosition;
  Frustum viewFrustum;

  vec4f viewPositionX;
  vec4f viewPositionY;
  vec4f viewPositionZ;

  vec4f lightDirection;

  RiGenVisibility *riGenVisibility;
  ContextId contextId;
  int threadIx;
  static constexpr int OUT_OF_FRUSTUM_VISIVILITY_INDEX = 1;
  int visibilityIndex;

  int meta_alloc_id_accel;
  uint64_t blas_accel;
  uint64_t mesh_id_accel;

  int riGenCount;
  int impostorCount;

  unsigned counter;

  bbox3f treeArea;

  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> newUniqueTreeBuffers[Context::maxUniqueLods];

  static ReferencedTransformData *mapTreeStationary(ContextId, uint64_t, vec4f, rendinst::riex_handle_t, int, void *, bool &,
    int &anim_index)
  {
    anim_index = -1;
    return nullptr;
  }

  static ReferencedTransformData *mapTree(ContextId context_id, uint64_t object_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, void *user_data, bool &recycled, int &anim_index)
  {
    G_UNUSED(handle);

    recycled = false;

    Point4 p4;
    v_stu(&p4, pos);
    auto kx = (uint64_t)bitwise_cast<uint32_t, float>(p4.x + p4.y);
    auto kz = (uint64_t)bitwise_cast<uint32_t, float>(p4.z - p4.y);
    auto id = (kx << 32 | kz) ^ object_id;

    // No need to have a lock here to access the containers. While this function is called from multiple threads,
    // the uniqueTreeBuffers is only read from all threads. All new items are added to the newUniqueTreeBuffers,
    // which is thread local. Then later it will be merged into the uniqueTreeBuffers on the main thread.

    G_ASSERT(lod_ix < Context::maxUniqueLods);
    auto &uniqueContainer = context_id->uniqueTreeBuffers[lod_ix];

    ReferencedTransformDataWithAge *tree = nullptr;
    if (auto instanceIter = uniqueContainer.find(id); instanceIter != uniqueContainer.end())
    {
      tree = &instanceIter->second;
    }
    else
    {
      tree = &static_cast<RiGenBVHJob *>(user_data)->newUniqueTreeBuffers[lod_ix][id];

      // Assign an anim index, which has the least instances
      int leastIndices = 0;
      for (int i = 1; i < Context::MaxTreeAnimIndices; ++i)
        if (context_id->treeAnimIndexCount[i] < context_id->treeAnimIndexCount[leastIndices])
          leastIndices = i;

      tree->animIndex = leastIndices;
      ++context_id->treeAnimIndexCount[leastIndices];
    }

    anim_index = tree->animIndex;

    auto &data = tree->elems[object_id];

    // Hash collision. It is possible that the same position is used for different trees of the same type.
    if (data.used)
      return nullptr;

    data.used = true;

    tree->age = -1;

    if (!data.buffer)
    {
      if (auto iter = context_id->freeUniqueTreeBLASes.find(object_id); iter != context_id->freeUniqueTreeBLASes.end())
      {
        if (int index = iter->second.cursor.sub_fetch(1); index >= 0)
        {
          auto &blas = iter->second.blases[index];
          data.blas.swap(blas);
          recycled = true;
        }
      }
    }

    return &data;
  }

  template <bool use_min_lod, bool filter_out_view_frustum>
  void doJobImpl()
  {
    counter = 0;

    auto callback = [](int layer_ix, int pool_ix, int lod_ix, int last_lod_ix, bool impostor, mat44f_cref tm, const E3DCOLOR *colors,
                      uint32_t bvh_id, void *user_data, uint32_t palette_id) {
      auto thiz = (RiGenBVHJob *)user_data;

      if ((++thiz->counter & ~32) == 0)
        bvh_yield();

#if DAGOR_DBGLEVEL > 0
      if (DAGOR_UNLIKELY(bvh_ri_gen_range_enable))
      {
        auto distSq = v_extract_x(v_length3_sq(v_sub(tm.col3, thiz->viewPosition)));
        if (sqr(bvh_ri_gen_range) < distSq)
          return;
      }
#endif

      ContextId context_id = thiz->contextId;

      mat43f tm43;
      v_mat44_transpose_to_mat43(tm43, tm);

      RenderableInstanceLodsResource *riRes = rendinst::getRIGenRes(layer_ix, pool_ix);
      if (!riRes || riRes->getBvhId() == 0)
        return;

      auto isInFrustum = [&]() -> bool {
        bbox3f originalBox = v_ldu_bbox3(riRes->bbox);
        bbox3f transformedBbox;
        v_bbox3_init(transformedBbox, tm, originalBox);
        return thiz->viewFrustum.testBoxB(transformedBbox.bmin, transformedBbox.bmax);
      };

      if constexpr (filter_out_view_frustum)
        if (isInFrustum())
          return;

      if (impostor)
      {
        if (!thiz->contextId->has(Features::RIFull))
          return;

        tm43.row0 = v_sub(tm43.row0, thiz->viewPositionX);
        tm43.row1 = v_sub(tm43.row1, thiz->viewPositionY);
        tm43.row2 = v_sub(tm43.row2, thiz->viewPositionZ);

        auto color = random_color_from_pos(tm.col3, palette_id, colors[0], colors[1]);

        if (add_impostor_instance(context_id, make_relem_mesh_id(bvh_id, lod_ix, 0), tm43, color.u, thiz->threadIx,
              thiz->meta_alloc_id_accel, thiz->blas_accel, thiz->mesh_id_accel))
          v_bbox3_add_pt(thiz->treeArea, v_mat43_extract_pos(tm43));

        thiz->impostorCount++;
        return;
      }

      if (thiz->contextId->has(Features::RIBaked))
        lod_ix = last_lod_ix;

      if constexpr (use_min_lod)
        lod_ix = max<int>(lod_ix, riRes->getBvhMinLod());

      auto &lod = riRes->lods[lod_ix]; //-V522
      auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      G_ASSERT(lod_ix < 16);

      bool hasTreeOrLeaves = eastl::any_of(elems.begin(), elems.end(),
        [&](auto &e) { return is_tree(thiz->contextId, e) || is_leaves(thiz->contextId, e); });

      if (!hasTreeOrLeaves)
      {
        auto objectId = make_relem_mesh_id(bvh_id, lod_ix, 0);
        add_riGen_instance(context_id, objectId, tm43, thiz->threadIx);
        thiz->riGenCount++;
      }
      else
      {
        eastl::optional<TMatrix4> invWorldTm;

        bool isStationary = false;
        if constexpr (!filter_out_view_frustum)
          if (stationary_rt_trees)
          {
            vec4f localBounds = v_make_vec4f(riRes->bsphCenter.x, riRes->bsphCenter.y, riRes->bsphCenter.z, riRes->bsphRad);
            vec4f worldBounds = v_mat44_mul_bsph(tm, localBounds);
            bbox3f worldBoundsBox;
            worldBoundsBox.bmin = v_sub(worldBounds, v_splat_w(worldBounds));
            worldBoundsBox.bmax = v_add(worldBounds, v_splat_w(worldBounds));
            vec3f farPoint = v_mul(v_add(v_add(worldBoundsBox.bmax, worldBoundsBox.bmin), thiz->lightDirection), V_C_HALF);
            worldBoundsBox.bmin = v_min(worldBoundsBox.bmin, farPoint);
            worldBoundsBox.bmax = v_max(worldBoundsBox.bmax, farPoint);
            isStationary = !thiz->viewFrustum.testBoxB(worldBoundsBox.bmin, worldBoundsBox.bmax);
          }

        for (auto [elemIx, elem] : enumerate(elems))
        {
          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          auto meshId = make_relem_mesh_id(bvh_id, lod_ix, elemIx);

          if (is_tree(thiz->contextId, elem))
          {
            bool isPosInst = rendinst::isRIGenOnlyPosInst(layer_ix, pool_ix);

            TreeInfo treeInfo;
            MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
            bool iok = isStationary ? handle_tree<mapTreeStationary>(thiz->contextId, elem, meshId, lod_ix, isPosInst, tm, tm.col3,
                                        colors, 0, invWorldTm, treeInfo, metaAllocId, thiz, true, false, palette_id)
                                    : handle_tree<mapTree>(thiz->contextId, elem, meshId, lod_ix, isPosInst, tm, tm.col3, colors, 0,
                                        invWorldTm, treeInfo, metaAllocId, thiz, false, false, palette_id);
            if (!iok)
              continue;
            add_riGen_instance(context_id, meshId, tm43, &treeInfo, isStationary, metaAllocId, thiz->threadIx);
            thiz->riGenCount++;
          }
          else if (is_leaves(thiz->contextId, elem))
          {
            LeavesInfo leavesInfo;
            MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
            bool iok =
              isStationary
                ? handle_leaves<mapTreeStationary>(thiz->contextId, meshId, lod_ix, tm, 0, invWorldTm, leavesInfo, metaAllocId, thiz)
                : handle_leaves<mapTree>(thiz->contextId, meshId, lod_ix, tm, 0, invWorldTm, leavesInfo, metaAllocId, thiz);
            if (!iok)
              continue;
            add_riGen_instance(context_id, meshId, tm43, &leavesInfo, isStationary, metaAllocId, thiz->threadIx);
            thiz->riGenCount++;
          }
          else
          {
            add_riGen_instance(context_id, meshId, tm43, thiz->threadIx);
            thiz->riGenCount++;
          }
        }
      }
    };

    riGenCount = 0;
    impostorCount = 0;

    mesh_id_accel = -1;
    meta_alloc_id_accel = -1;
    blas_accel = 0;

    v_bbox3_init_empty(treeArea);

    rendinst::foreachRiGenInstance(riGenVisibility, callback, this, accels[visibilityIndex].accel1, accels[visibilityIndex].accel2,
      accels[visibilityIndex].accel1Cursor, accels[visibilityIndex].accel2Cursor, true);

    DA_PROFILE_TAG_DESC(getJobNameProfDesc(), "riGen count: %d, impostor count: %d", riGenCount, impostorCount);
  }

  void doJob()
  {
    struct CallHelper
    {
      using jobCallType = void (*)(RiGenBVHJob &job);
      jobCallType callHelper[4] = {
        [](RiGenBVHJob &job) { job.doJobImpl<false, false>(); },
        [](RiGenBVHJob &job) { job.doJobImpl<false, true>(); },
        [](RiGenBVHJob &job) { job.doJobImpl<true, false>(); },
        [](RiGenBVHJob &job) { job.doJobImpl<true, true>(); },
      };

      void operator()(RiGenBVHJob &job, bool filterOutViewFrustum, bool useMinLod)
      {
        callHelper[(useMinLod ? 2 : 0) + (filterOutViewFrustum ? 1 : 0)](job);
      }
    } doJobWrapper;

    const bool filterOutViewFrustum = visibilityIndex == OUT_OF_FRUSTUM_VISIVILITY_INDEX;
    const bool useMinLod = bvh::ri::get_ri_lod_dist_bias() > 0;
    doJobWrapper(*this, filterOutViewFrustum, useMinLod);

    parallel_instance_processing::after_job_end(contextId);
  }

  const char *getJobName(bool &) const override { return "RiGenBVHJob"; }

} ri_gen_bvh_job[ri_gen_thread_count];

void wait_ri_gen_instances_update([[maybe_unused]] ContextId context_id)
{
  bbox3f treeArea;
  v_bbox3_init_empty(treeArea);

  for (int threadIx = 0; threadIx < ri_gen_thread_count; ++threadIx)
  {
    TIME_PROFILE(bvh::update_ri_extra_instances);
    DA_PROFILE_TAG(bvh::update_ri_extra_instances, "wait thread %d", threadIx);
    threadpool::wait(&ri_gen_bvh_job[threadIx]);
    v_bbox3_add_box(treeArea, ri_gen_bvh_job[threadIx].treeArea);
  }

  static int rtsm_tree_areaVarId = get_shader_variable_id("rtsm_tree_area");
  ShaderGlobal::set_float4(rtsm_tree_areaVarId, v_extract_x(treeArea.bmin), v_extract_z(treeArea.bmin), v_extract_x(treeArea.bmax),
    v_extract_z(treeArea.bmax));
}

void update_ri_gen_instances(ContextId context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  const Point3 &view_position, const Point3 &light_direction, const Frustum &view_frustum, threadpool::JobPriority prio)
{
  if (ri_gen_visibilities.empty() || !context_id->has(Features::AnyRI))
    return;
  G_ASSERT(ri_gen_visibilities.size() <= 2);

  wait_ri_gen_instances_update(context_id);
  start_new_tree_mapping_debug_frame();

  const float maxLightDistForBvhShadow = 20.0f;

  for (auto [i, ri_gen_visibility] : enumerate(ri_gen_visibilities))
  {
    rendinst::build_ri_gen_thread_accel(ri_gen_visibility, RiGenBVHJob::accels[i].accel1, RiGenBVHJob::accels[i].accel2);
    RiGenBVHJob::accels[i].accel1Cursor = 0;
    RiGenBVHJob::accels[i].accel2Cursor = 0;

    int threadStart = get_ri_gen_worker_count() * i;
    int threadEnd = get_ri_gen_worker_count() * (i + 1);
    for (int threadIx = threadStart; threadIx < threadEnd; ++threadIx)
    {
      auto &job = ri_gen_bvh_job[threadIx];
      job.riGenVisibility = ri_gen_visibility;
      job.contextId = context_id;
      job.threadIx = threadIx;
      job.visibilityIndex = i;
      job.viewPosition = v_ldu_p3_safe(&view_position.x);
      job.viewFrustum = view_frustum;
      job.viewPositionX = v_make_vec4f(0, 0, 0, view_position.x);
      job.viewPositionY = v_make_vec4f(0, 0, 0, view_position.y);
      job.viewPositionZ = v_make_vec4f(0, 0, 0, view_position.z);
      job.lightDirection = v_mul(v_ldu_p3_safe(&light_direction.x), v_splats(maxLightDistForBvhShadow * 2));

      parallel_instance_processing::before_job_start(context_id);
      threadpool::add(&job, prio);
    }
  }
}

static struct TidyUpTreesJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "TidyUpTreesJob"; }

  void doJob() override
  {
    // Remove unique tree buffers that was not used for a long time

    WinAutoLock lock(contextId->tidyUpTreesLock);

    for (auto &job : ri_gen_bvh_job)
    {
      for (auto [index, lod] : enumerate(job.newUniqueTreeBuffers))
      {
        for (auto &tree : lod)
          contextId->uniqueTreeBuffers[index].insert(std::move(tree));
        lod.clear();
      }
    }

    // We make use of the fact that resize does not shrink the vector itself, only the elem count is changed
    for (auto &freeTrees : contextId->freeUniqueTreeBLASes)
    {
      if (freeTrees.second.cursor.load() < 0)
        freeTrees.second.cursor.store(0);
      freeTrees.second.blases.resize(freeTrees.second.cursor.load());
    }

    int dropCount = 0;

    for (auto [index, lod] : enumerate(contextId->uniqueTreeBuffers))
      for (auto iter = lod.begin(); iter != lod.end();)
      {
        // Reset the used flags, so in the next frame, all start fresh
        for (auto &elem : iter->second.elems)
          elem.second.used = false;

        iter->second.age++;
        if (iter->second.age < 1)
        {
          ++iter;
          continue;
        }

        --contextId->treeAnimIndexCount[iter->second.animIndex];

        for (auto &elem : iter->second.elems)
        {
          auto &storage = contextId->freeUniqueTreeBLASes[elem.first].blases.push_back();
          storage.swap(elem.second.blas);

          contextId->freeMetaRegion(elem.second.metaAllocId);

          if (elem.second.buffer)
          {
            WinAutoLock lock(contextId->processBufferAllocatorLock);
            contextId->processBufferAllocator[elem.second.buffer.allocator].first.free(elem.second.buffer.allocId);
          }
        }

        dropCount++;
        iter = lod.erase(iter);
      }

    for (auto &freeTrees : contextId->freeUniqueTreeBLASes)
      freeTrees.second.cursor.store(freeTrees.second.blases.size());
  }
} tidy_up_trees_job;

void tidy_up_trees(ContextId context_id)
{
  tidy_up_trees_job.contextId = context_id;
  threadpool::add(&tidy_up_trees_job, threadpool::PRIO_HIGH);
}

void wait_tidy_up_trees() { threadpool::wait(&tidy_up_trees_job); }

void teardown_ri_gen()
{
  for (int i = 0; i < countof(RiGenBVHJob::accels); i++)
  {
    RiGenBVHJob::accels[i].accel1.clear();
    RiGenBVHJob::accels[i].accel1.shrink_to_fit();
    RiGenBVHJob::accels[i].accel2.clear();
    RiGenBVHJob::accels[i].accel2.shrink_to_fit();
  }
}

} // namespace bvh::ri