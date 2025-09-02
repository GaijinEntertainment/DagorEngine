// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <rendInst/rendInstAccess.h>
#include <generic/dag_enumerate.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_bitwise_cast.h>
#include <util/dag_finally.h>
#include <math/dag_frustum.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_gen_range_enable;
extern float bvh_ri_gen_range;
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

  auto data = mapper(context_id, object_id, tm.col3, handle, lod_ix, user_data, leavesInfo.recycled);

  if (!data)
    return false;

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

  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> newUniqueTreeBuffers[Context::maxUniqueLods];

  static ReferencedTransformData *mapTree(ContextId context_id, uint64_t object_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, void *user_data, bool &recycled)
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
    }

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

  template <bool filter_out_view_frustum>
  void doJobImpl()
  {
    counter = 0;

    auto callback = [](int layer_ix, int pool_ix, int lod_ix, int last_lod_ix, bool impostor, mat44f_cref tm, const E3DCOLOR *colors,
                      uint32_t bvh_id, void *user_data) {
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

      RenderableInstanceLodsResource *riRes = nullptr;

      if constexpr (filter_out_view_frustum)
      {
        riRes = rendinst::getRIGenRes(layer_ix, pool_ix);
        if (!riRes || riRes->getBvhId() == 0)
          return;

        // These are already present in the other visibility
        bbox3f originalBox = v_ldu_bbox3(riRes->bbox);
        bbox3f transformedBbox;
        v_bbox3_init(transformedBbox, tm, originalBox);
        if (thiz->viewFrustum.testBoxB(transformedBbox.bmin, transformedBbox.bmax))
          return;
      }

      if (impostor)
      {
        if (!thiz->contextId->has(Features::RIFull))
          return;

        tm43.row0 = v_sub(tm43.row0, thiz->viewPositionX);
        tm43.row1 = v_sub(tm43.row1, thiz->viewPositionY);
        tm43.row2 = v_sub(tm43.row2, thiz->viewPositionZ);

        auto color = random_color_from_pos(tm.col3, 0, colors[0], colors[1]);

        add_impostor_instance(context_id, make_relem_mesh_id(bvh_id, lod_ix, 0), tm43, color.u, thiz->threadIx,
          thiz->meta_alloc_id_accel, thiz->blas_accel, thiz->mesh_id_accel);
        thiz->impostorCount++;
        return;
      }

      if (thiz->contextId->has(Features::RIBaked))
        lod_ix = last_lod_ix;

      if constexpr (!filter_out_view_frustum)
      {
        riRes = rendinst::getRIGenRes(layer_ix, pool_ix);
        if (!riRes || riRes->getBvhId() == 0)
          return;
      }

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
            if (!handle_tree<mapTree>(thiz->contextId, elem, meshId, lod_ix, isPosInst, tm, tm.col3, colors, 0, invWorldTm, treeInfo,
                  metaAllocId, thiz))
              continue;
            add_riGen_instance(context_id, meshId, tm43, &treeInfo, metaAllocId, thiz->threadIx);
            thiz->riGenCount++;
          }
          else if (is_leaves(thiz->contextId, elem))
          {
            LeavesInfo leavesInfo;
            MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
            if (!handle_leaves<mapTree>(thiz->contextId, meshId, lod_ix, tm, 0, invWorldTm, leavesInfo, metaAllocId, thiz))
              continue;
            add_riGen_instance(context_id, meshId, tm43, &leavesInfo, metaAllocId, thiz->threadIx);
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

    rendinst::foreachRiGenInstance(riGenVisibility, callback, this, accels[visibilityIndex].accel1, accels[visibilityIndex].accel2,
      accels[visibilityIndex].accel1Cursor, accels[visibilityIndex].accel2Cursor, true);

    DA_PROFILE_TAG_DESC(getJobNameProfDesc(), "riGen count: %d, impostor count: %d", riGenCount, impostorCount);
  }

  void doJob()
  {
    if (visibilityIndex == OUT_OF_FRUSTUM_VISIVILITY_INDEX)
      doJobImpl<true>();
    else
      doJobImpl<false>();
  }

  const char *getJobName(bool &) const override { return "RiGenBVHJob"; }

} ri_gen_bvh_job[ri_gen_thread_count];

void wait_ri_gen_instances_update(ContextId context_id)
{
  for (int threadIx = 0; threadIx < ri_gen_thread_count; ++threadIx)
  {
    TIME_PROFILE_NAME(bvh::update_ri_extra_instances,
      String(128, "wait ri_gen_bvh_job for %s: %d", context_id->name.data(), threadIx));
    threadpool::wait(&ri_gen_bvh_job[threadIx]);
  }
}

void update_ri_gen_instances(ContextId context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  const Point3 &view_position, const Frustum &view_frustum, threadpool::JobPriority prio)
{
  if (ri_gen_visibilities.empty() || !context_id->has(Features::RIFull | Features::RIBaked))
    return;
  G_ASSERT(ri_gen_visibilities.size() <= 2);

  wait_ri_gen_instances_update(context_id);
  start_new_tree_mapping_debug_frame();

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

      threadpool::add(&job, prio);
    }
  }
}

static struct CutDownTreesJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "CutDownTreesJob"; }

  void doJob() override
  {
    // Remove unique tree buffers that was not used for a long time

    WinAutoLock lock(contextId->cutdownTreeLock);

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

        for (auto &elem : iter->second.elems)
        {
          auto &storage = contextId->freeUniqueTreeBLASes[elem.first].blases.push_back();
          storage.swap(elem.second.blas);

          contextId->freeMetaRegion(elem.second.metaAllocId);
          contextId->releaseBuffer(elem.second.buffer.get());

          if (auto iter = contextId->processBufferAllocators.find(elem.second.buffer.allocator);
              iter != contextId->processBufferAllocators.end())
            iter->second.free(elem.second.buffer);
        }

        dropCount++;
        iter = lod.erase(iter);
      }

    for (auto &freeTrees : contextId->freeUniqueTreeBLASes)
      freeTrees.second.cursor.store(freeTrees.second.blases.size());
  }
} cut_down_trees_job;

void cut_down_trees(ContextId context_id)
{
  cut_down_trees_job.contextId = context_id;
  threadpool::add(&cut_down_trees_job, threadpool::PRIO_HIGH);
}

void wait_cut_down_trees() { threadpool::wait(&cut_down_trees_job); }

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