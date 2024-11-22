// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <rendInst/rendInstAccess.h>
#include <generic/dag_enumerate.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_gen_range_enable;
extern float bvh_ri_gen_range;
#endif

namespace bvh::ri
{

struct DECLSPEC_ALIGN(16) IPoint4_vec4 : public IPoint4
{
} ATTRIBUTE_ALIGN(16);

static constexpr float tree_mapper_scale = 100;

template <TreeMapper mapper>
static void handle_leaves(ContextId context_id, uint64_t mesh_id, int lod_ix, mat44f_cref tm, rendinst::riex_handle_t handle,
  eastl::optional<TMatrix4> &inv_world_tm, LeavesInfo &leavesInfo, MeshMetaAllocator::AllocId &metaAllocId, uint64_t bvh_id,
  void *user_data)
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

  auto &data = mapper(context_id, mesh_id, tm.col3, handle, lod_ix, bvh_id, user_data, leavesInfo.recycled);

  if (data.metaAllocId < 0)
    data.metaAllocId = context_id->allocateMetaRegion();

  metaAllocId = data.metaAllocId;

  leavesInfo.invWorldTm = inv_world_tm.value();
  leavesInfo.transformedBuffer = &data.buffer;
  leavesInfo.transformedBlas = &data.blas;
}

static struct RiGenBVHJob : public cpujobs::IJob
{
  inline static dag::Vector<uint32_t> accel1;
  inline static dag::Vector<uint64_t> accel2;

  inline static volatile int accel1Cursor;
  inline static volatile int accel2Cursor;

  vec4f viewPosition;

  vec4f viewPositionX;
  vec4f viewPositionY;
  vec4f viewPositionZ;

  RiGenVisibility *riGenVisibility;
  ContextId contextId;
  int threadIx;

  int meta_alloc_id_accel;
  uint64_t blas_accel;
  uint64_t mesh_id_accel;

  int riGenCount;
  int impostorCount;

  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> newUniqueTreeBuffers;

  static ReferencedTransformData &mapTree(ContextId context_id, uint64_t mesh_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, uint64_t bvh_id, void *user_data, bool &recycled)
  {
    G_UNUSED(handle);

    recycled = false;

    auto keyPos = v_cvti_vec4i(v_round(v_mul(pos, v_splats(tree_mapper_scale))));
    IPoint4_vec4 keyPosScalar;
    v_sti(&keyPosScalar, keyPos);
    auto id = (uint64_t(keyPosScalar.x) << 34 | (uint64_t(keyPosScalar.z) & 0x3FFFFFFF) << 4 | lod_ix) ^ bvh_id;

    ReferencedTransformDataWithAge *tree = nullptr;
    if (auto instanceIter = context_id->uniqueTreeBuffers.find(id); instanceIter != context_id->uniqueTreeBuffers.end())
    {
      tree = &instanceIter->second;
    }
    else
    {
      tree = &static_cast<RiGenBVHJob *>(user_data)->newUniqueTreeBuffers[id];
    }

    auto &data = tree->elems[mesh_id];
    tree->age = -1;

    if (!data.buffer)
    {
      if (auto iter = context_id->freeUniqueTreeBLASes.find(mesh_id); iter != context_id->freeUniqueTreeBLASes.end())
      {
        if (int index = --iter->second.cursor; index >= 0)
        {
          auto &blas = iter->second.blases[index];
          data.blas.swap(blas);
          recycled = true;
        }
      }
    }


    return data;
  }

  void doJob()
  {
    auto callback = [](int layer_ix, int pool_ix, int lod_ix, int last_lod_ix, bool impostor, mat44f_cref tm, const E3DCOLOR *colors,
                      uint32_t bvh_id, void *user_data) {
      auto thiz = (RiGenBVHJob *)user_data;

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

      if (impostor)
      {
        if (!thiz->contextId->has(Features::RIFull))
          return;

        tm43.row0 = v_sub(tm43.row0, thiz->viewPositionX);
        tm43.row1 = v_sub(tm43.row1, thiz->viewPositionY);
        tm43.row2 = v_sub(tm43.row2, thiz->viewPositionZ);

        auto color = random_color_from_pos(tm.col3, colors[0], colors[1]);

        add_impostor_instance(context_id, make_relem_mesh_id(bvh_id, lod_ix, 0), tm43, color.u, thiz->threadIx,
          thiz->meta_alloc_id_accel, thiz->blas_accel, thiz->mesh_id_accel);
        thiz->impostorCount++;
        return;
      }

      if (thiz->contextId->has(Features::RIBaked))
        lod_ix = last_lod_ix;

      auto riRes = rendinst::getRIGenRes(layer_ix, pool_ix);
      if (!riRes || riRes->getBvhId() == 0)
        return;

      auto &lod = riRes->lods[lod_ix];
      auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      G_ASSERT(lod_ix < 16);

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
          MeshMetaAllocator::AllocId metaAllocId = -1;
          handle_tree<mapTree>(thiz->contextId, elem, meshId, lod_ix, isPosInst, tm, colors, 0, invWorldTm, treeInfo, metaAllocId,
            bvh_id, thiz);
          add_riGen_instance(context_id, meshId, tm43, &treeInfo, metaAllocId, thiz->threadIx);
          thiz->riGenCount++;
        }
        else if (is_leaves(thiz->contextId, elem))
        {
          LeavesInfo leavesInfo;
          MeshMetaAllocator::AllocId metaAllocId = -1;
          handle_leaves<mapTree>(thiz->contextId, meshId, lod_ix, tm, 0, invWorldTm, leavesInfo, metaAllocId, bvh_id, thiz);
          add_riGen_instance(context_id, meshId, tm43, &leavesInfo, metaAllocId, thiz->threadIx);
          thiz->riGenCount++;
        }
        else
        {
          add_riGen_instance(context_id, meshId, tm43, thiz->threadIx);
          thiz->riGenCount++;
        }
      }
    };

    riGenCount = 0;
    impostorCount = 0;

    mesh_id_accel = -1;
    meta_alloc_id_accel = -1;
    blas_accel = 0;

    TIME_PROFILE(bvh::update_ri_gen_instances);
    rendinst::foreachRiGenInstance(riGenVisibility, callback, this, accel1, accel2, accel1Cursor, accel2Cursor);

    DA_PROFILE_TAG(bvh::update_ri_gen_instances, "riGen count: %d, impostor count: %d", riGenCount, impostorCount);
  }
} ri_gen_bvh_job[ri_gen_thread_count];

void wait_ri_gen_instances_update(ContextId context_id)
{
  for (int threadIx = 0; threadIx < get_ri_gen_worker_count(); ++threadIx)
  {
    TIME_PROFILE_NAME(bvh::update_ri_extra_instances,
      String(128, "wait ri_gen_bvh_job for %s: %d", context_id->name.data(), threadIx));
    threadpool::wait(&ri_gen_bvh_job[threadIx]);
  }
}

void update_ri_gen_instances(ContextId context_id, RiGenVisibility *ri_gen_visibility, const Point3 &view_position)
{
  if (!ri_gen_visibility || !context_id->has(Features::RIFull | Features::RIBaked))
    return;

  wait_ri_gen_instances_update(context_id);

  rendinst::build_ri_gen_thread_accel(ri_gen_visibility, RiGenBVHJob::accel1, RiGenBVHJob::accel2);
  RiGenBVHJob::accel1Cursor = 0;
  RiGenBVHJob::accel2Cursor = 0;

  for (int threadIx = 0; threadIx < get_ri_gen_worker_count(); ++threadIx)
  {
    auto &job = ri_gen_bvh_job[threadIx];
    job.riGenVisibility = ri_gen_visibility;
    job.contextId = context_id;
    job.threadIx = threadIx;
    job.viewPosition = v_ldu_p3_safe(&view_position.x);
    job.viewPositionX = v_make_vec4f(0, 0, 0, view_position.x);
    job.viewPositionY = v_make_vec4f(0, 0, 0, view_position.y);
    job.viewPositionZ = v_make_vec4f(0, 0, 0, view_position.z);

    threadpool::add(&job, threadpool::PRIO_HIGH);
  }
}

static struct CutDownTreesJob : public cpujobs::IJob
{
  ContextId contextId;

  void doJob() override
  {
    // Remove unique tree buffers that was not used for a long time

    TIME_PROFILE(bvh__cut_down_trees);

    WinAutoLock lock(contextId->cutdownTreeLock);

    for (auto &job : ri_gen_bvh_job)
    {
      for (auto &tree : job.newUniqueTreeBuffers)
        contextId->uniqueTreeBuffers.insert(std::move(tree));
      job.newUniqueTreeBuffers.clear();
    }

    // We make use of the fact that resize does not shrink the vector itself, only the elem count is changed
    for (auto &freeTrees : contextId->freeUniqueTreeBLASes)
    {
      if (freeTrees.second.cursor < 0)
        freeTrees.second.cursor = 0;
      freeTrees.second.blases.resize(freeTrees.second.cursor);
    }

    int dropCount = 0;

    for (auto iter = contextId->uniqueTreeBuffers.begin(); iter != contextId->uniqueTreeBuffers.end();)
    {
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
      iter = contextId->uniqueTreeBuffers.erase(iter);
    }

    for (auto &freeTrees : contextId->freeUniqueTreeBLASes)
      freeTrees.second.cursor = freeTrees.second.blases.size();
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
  RiGenBVHJob::accel1.clear();
  RiGenBVHJob::accel1.shrink_to_fit();
  RiGenBVHJob::accel2.clear();
  RiGenBVHJob::accel2.shrink_to_fit();
}

} // namespace bvh::ri