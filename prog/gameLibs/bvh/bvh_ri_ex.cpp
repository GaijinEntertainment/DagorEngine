// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <math/dag_frustum.h>
#include <rendInst/rendInstExtraAccess.h>
#include <generic/dag_enumerate.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_extra_range_enable;
extern float bvh_ri_extra_range;
#endif

namespace bvh::ri
{

static OSSpinlock tree_lock;
static OSSpinlock flag_lock;
static OSSpinlock deformed_lock;

static void handle_flag(ContextId context_id, ShaderMesh::RElem &elem, uint64_t mesh_id, mat44f_cref tm,
  rendinst::riex_handle_t handle, eastl::optional<TMatrix4> &inv_world_tm, FlagInfo &flagInfo, MeshMetaAllocator::AllocId &metaAllocId)
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

  flag_lock.lock();
  auto &data = context_id->uniqueRiExtraFlagBuffers[handle][mesh_id];
  flag_lock.unlock();

  if (data.metaAllocId < 0)
    data.metaAllocId = context_id->allocateMetaRegion();

  metaAllocId = data.metaAllocId;

  flagInfo.invWorldTm = inv_world_tm.value();
  flagInfo.transformedBuffer = &data.buffer;
  flagInfo.transformedBlas = &data.blas;

  static int wind_typeVarId = VariableMap::getVariableId("wind_type");
  static int frequency_amplitudeVarId = VariableMap::getVariableId("frequency_amplitude");
  static int wind_directionVarId = VariableMap::getVariableId("wind_direction");
  static int wind_strengthVarId = VariableMap::getVariableId("wind_strength");
  static int wave_lengthVarId = VariableMap::getVariableId("wave_length");
  static int flagpole_pos_0VarId = VariableMap::getVariableId("flagpole_pos_0");
  static int flagpole_pos_1VarId = VariableMap::getVariableId("flagpole_pos_1");
  static int stiffnessVarId = VariableMap::getVariableId("stiffness");
  static int flag_movement_scaleVarId = VariableMap::getVariableId("flag_movement_scale");
  static int bendVarId = VariableMap::getVariableId("bend");
  static int deviationVarId = VariableMap::getVariableId("deviation");
  static int stretchVarId = VariableMap::getVariableId("stretch");
  static int flag_lengthVarId = VariableMap::getVariableId("flag_length");
  static int sway_speedVarId = VariableMap::getVariableId("sway_speed");
  static int width_typeVarId = VariableMap::getVariableId("width_type");

  if (!elem.mat->getIntVariable(wind_typeVarId, flagInfo.data.windType))
    flagInfo.data.windType = 0;
  if (flagInfo.data.windType == 0)
  {
    if (!elem.mat->getColor4Variable(frequency_amplitudeVarId, *(Color4 *)&flagInfo.data.fixedWind.frequencyAmplitude))
      flagInfo.data.fixedWind.frequencyAmplitude = Point4::ONE;
    if (!elem.mat->getColor4Variable(wind_directionVarId, *(Color4 *)&flagInfo.data.fixedWind.windDirection))
      flagInfo.data.fixedWind.windDirection = Point4(0, 0, 1, 0);
    if (!elem.mat->getRealVariable(wind_strengthVarId, flagInfo.data.fixedWind.windStrength))
      flagInfo.data.fixedWind.windStrength = 1.1;
    if (!elem.mat->getRealVariable(wave_lengthVarId, flagInfo.data.fixedWind.waveLength))
      flagInfo.data.fixedWind.waveLength = 0.5;
  }
  else
  {
    if (!elem.mat->getColor4Variable(flagpole_pos_0VarId, *(Color4 *)&flagInfo.data.globalWind.flagpolePos0))
      flagInfo.data.globalWind.flagpolePos0 = Point4(0, 0, -1, 0);
    if (!elem.mat->getColor4Variable(flagpole_pos_1VarId, *(Color4 *)&flagInfo.data.globalWind.flagpolePos1))
      flagInfo.data.globalWind.flagpolePos1 = Point4(0, 0, 1, 0);
    if (!elem.mat->getRealVariable(stiffnessVarId, flagInfo.data.globalWind.stiffness))
      flagInfo.data.globalWind.stiffness = 0.1;
    if (!elem.mat->getRealVariable(flag_movement_scaleVarId, flagInfo.data.globalWind.flagMovementScale))
      flagInfo.data.globalWind.flagMovementScale = 1;
    if (!elem.mat->getRealVariable(bendVarId, flagInfo.data.globalWind.bend))
      flagInfo.data.globalWind.bend = 2;
    if (!elem.mat->getRealVariable(deviationVarId, flagInfo.data.globalWind.deviation))
      flagInfo.data.globalWind.deviation = 4;
    if (!elem.mat->getRealVariable(stretchVarId, flagInfo.data.globalWind.stretch))
      flagInfo.data.globalWind.stretch = 0.1;
    if (!elem.mat->getRealVariable(flag_lengthVarId, flagInfo.data.globalWind.flagLength))
      flagInfo.data.globalWind.flagLength = 15;
    if (!elem.mat->getRealVariable(sway_speedVarId, flagInfo.data.globalWind.swaySpeed))
      flagInfo.data.globalWind.swaySpeed = 1;
    if (!elem.mat->getIntVariable(width_typeVarId, flagInfo.data.globalWind.widthType))
      flagInfo.data.globalWind.widthType = 0;
  }
}

struct RiExtraAccel
{
  int elemCount = -1;
  Tab<rendinst::riex_handle_t> handles;
};

static eastl::unordered_map<uint32_t, RiExtraAccel> ri_extra_accel;

struct RiExtraGroup
{
  int riType;
  int weight;
  int work;
  Tab<rendinst::riex_handle_t> *handles;
};
using RiExtraGroupList = Tab<RiExtraGroup>;

static RiExtraGroupList allRiExtraHandles;

struct RiExtraBVHJob : public cpujobs::IJob
{
  ContextId contextId;
  int threadIx;
  Point3 viewPosition;
  Frustum frustum;

  std::atomic_int *nextGroupIx = nullptr;

  int fetchNextGroupIx() { return nextGroupIx->fetch_add(1); }

  static ReferencedTransformData &mapTree(ContextId context_id, uint64_t mesh_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, uint64_t bvh_id, void *user_data, bool &recycled)
  {
    G_UNUSED(pos);
    G_UNUSED(lod_ix);
    G_UNUSED(bvh_id);
    G_UNUSED(user_data);

    tree_lock.lock();
    auto &data = context_id->uniqueRiExtraTreeBuffers[handle][mesh_id];
    tree_lock.unlock();

    recycled = false;

    return data;
  }

  void doJob()
  {
    TIME_PROFILE(bvh::update_ri_extra_instances);

    vec4f viewPositionVec = v_make_vec4f(viewPosition.x, viewPosition.y, viewPosition.z, 0);

    bool overMaxLod;

    vec4f viewPositionX = v_make_vec4f(0, 0, 0, viewPosition.x);
    vec4f viewPositionY = v_make_vec4f(0, 0, 0, viewPosition.y);
    vec4f viewPositionZ = v_make_vec4f(0, 0, 0, viewPosition.z);
    rendinst::ScopedRIExtraReadLock rd;

    for (int groupIx = fetchNextGroupIx(); groupIx < allRiExtraHandles.size(); groupIx = fetchNextGroupIx())
    {
      auto &group = allRiExtraHandles[groupIx];
      auto riType = group.riType;
      auto riRes = rendinst::getRIGenExtraRes(riType);
      auto colors = rendinst::getRIGenExtraColors(riType);
      auto &handles = *group.handles;
      auto bestLod = riRes->getQlBestLod();

      for (auto handle : handles)
      {
        uint32_t hashVal;
        const mat43f &transform = rendinst::getRIGenExtra43(handle, hashVal);

        vec4f bsph = rendinst::getRIGenExtraBSphere(handle);
        vec4f center = v_make_vec4f(v_extract_x(bsph), v_extract_y(bsph), v_extract_z(bsph), 0);
        float radius = abs(v_extract_w(bsph));
        vec4f difference = v_sub(center, viewPositionVec);

        if (contextId->riExtraRange > 0)
        {
          vec4f absDiff = v_abs(difference);
          if (v_extract_x(absDiff) > contextId->riExtraRange || v_extract_z(absDiff) > contextId->riExtraRange)
            continue;
        }

        float distSq = v_extract_x(v_dot3(difference, difference));
        distSq += radius * radius - 2 * radius * sqrtf(distSq);
        if (distSq < 0)
          distSq = 0;

#if DAGOR_DBGLEVEL > 0
        if (bvh_ri_extra_range_enable && sqr(bvh_ri_extra_range) < distSq)
          continue;
#endif

        int lastLod;
        auto preferredLodIx = rendinst::getRIGenExtraPreferrableLod(riType, distSq, overMaxLod, lastLod);
        if (preferredLodIx < 0 || overMaxLod)
          continue;
        if (contextId->has(Features::RIBaked))
          preferredLodIx = lastLod;
        auto lodIx = max(preferredLodIx, bestLod);
        auto &lod = riRes->lods[lodIx];
        auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

        eastl::optional<TMatrix4> invWorldTm;

        for (auto [elemIx, elem] : enumerate(elems))
        {
          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          auto meshId = make_relem_mesh_id(riRes->getBvhId(), lodIx, elemIx);

          if (DAGOR_UNLIKELY(is_tree(contextId, elem)))
          {
            mat44f tm44;
            v_mat43_transpose_to_mat44(tm44, transform);
            tm44.col3 = v_add(tm44.col3, v_make_vec4f(0, 0, 0, 1));

            bbox3f aabb = {v_ldu(&riRes->bbox.boxMin().x), v_ldu_p3_safe(&riRes->bbox.boxMax().x)};
            bbox3f aabbWorld;
            v_bbox3_init(aabbWorld, tm44, aabb);

            if (frustum.testBox(aabbWorld.bmin, aabbWorld.bmax))
            {
              // This code is to dump riExtra tree names
              // String name;
              // if (resolve_game_resource_name(name, riRes))
              //  debug("riExtra tree: %s", name.data());
              TreeInfo treeInfo;
              MeshMetaAllocator::AllocId metaAllocId = -1;
              handle_tree<mapTree>(contextId, elem, meshId, lodIx, false, tm44, colors, handle, invWorldTm, treeInfo, metaAllocId,
                riRes->getBvhId(), this);
              add_riExtra_instance(contextId, meshId, transform, &treeInfo, metaAllocId, threadIx);
            }
          }
          else if (DAGOR_UNLIKELY(is_flag(contextId, elem)))
          {
            mat44f tm44;
            v_mat43_transpose_to_mat44(tm44, transform);
            tm44.col3 = v_add(tm44.col3, v_make_vec4f(0, 0, 0, 1));

            bbox3f aabb = {v_ldu(&riRes->bbox.boxMin().x), v_ldu_p3_safe(&riRes->bbox.boxMax().x)};
            bbox3f aabbWorld;
            v_bbox3_init(aabbWorld, tm44, aabb);

            if (frustum.testBox(aabbWorld.bmin, aabbWorld.bmax))
            {
              FlagInfo flagInfo;
              flagInfo.data.hashVal = hashVal;
              MeshMetaAllocator::AllocId metaAllocId = -1;
              handle_flag(contextId, elem, meshId, tm44, handle, invWorldTm, flagInfo, metaAllocId);
              add_riExtra_instance(contextId, meshId, transform, &flagInfo, metaAllocId, threadIx);
            }
          }
          else
          {
            hashVal %= max(contextId->paintTexSize, 1U);
            add_riExtra_instance(transform, viewPositionX, viewPositionY, viewPositionZ, contextId, meshId, threadIx, hashVal);
          }
        }
      }
    }
  }
};

struct RIExtraBVHJobGroup
{
  std::atomic_int nextGroupIx;
  RiExtraBVHJob jobs[ri_extra_thread_count];
};

static eastl::unordered_map<ContextId, RIExtraBVHJobGroup> riExtraJobGroups;

void wait_ri_extra_instances_update(ContextId context_id)
{
  for (int threadIx = 0; threadIx < get_ri_extra_worker_count(); ++threadIx)
  {
    TIME_PROFILE_NAME(bvh::update_ri_extra_instances,
      String(128, "wait ri_extra_bvh_job for %s: %d", context_id->name.data(), threadIx));
    threadpool::wait(&riExtraJobGroups[context_id].jobs[threadIx]);
  }
}

void prepare_ri_extra_instances()
{
  TIME_PROFILE(prepare_bvh_ri_extra_instances);

  int totalRiTypes = rendinst::getRiGenExtraResCount();

  // Here we try to make sure that all threads has roughly the same amount of work

  allRiExtraHandles.clear();

  {
    TIME_PROFILE(collect);

    for (int riType = 0; riType < totalRiTypes; ++riType)
    {
      auto riRes = rendinst::getRIGenExtraRes(riType);
      auto bvhId = riRes->getBvhId();
      if (bvhId == 0)
        continue;

      auto bestLod = riRes->getQlBestLod();
      auto &accel = ri_extra_accel[bvhId << 8 | bestLod];

      if (rendinst::getRiGenExtraInstances(accel.handles, riType) > 0)
      {
        if (accel.elemCount < 0)
        {
          auto &lod = riRes->lods[bestLod];
          auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

          accel.elemCount = 0;
          bool hasIndices;
          for (auto [elemIx, elem] : enumerate(elems))
            accel.elemCount += elem.vertexData->isRenderable(hasIndices) ? 1 : 0;
        }

        if (accel.elemCount == 0)
          continue;

        allRiExtraHandles.push_back_uninitialized();
        allRiExtraHandles.back().riType = riType;
        allRiExtraHandles.back().weight = accel.handles.size() * accel.elemCount;
        allRiExtraHandles.back().handles = &accel.handles;
      }
    }
  }

  {
    TIME_PROFILE(sort);
    eastl::sort(allRiExtraHandles.begin(), allRiExtraHandles.end(), [](const auto &a, const auto &b) { return a.weight > b.weight; });
  }
}

void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &frustum)
{
  auto &jobGroup = riExtraJobGroups[context_id];

  jobGroup.nextGroupIx = 0;

  for (int threadIx = 0; threadIx < get_ri_extra_worker_count(); ++threadIx)
  {
    auto &job = jobGroup.jobs[threadIx];
    job.contextId = context_id;
    job.threadIx = threadIx;
    job.viewPosition = view_position;
    job.nextGroupIx = &jobGroup.nextGroupIx;
    job.frustum = frustum;
    threadpool::add(&job, threadpool::PRIO_HIGH);
  }
}

void on_unload_scene_ri_ex(ContextId context_id)
{
  ri_extra_accel.clear();
  riExtraJobGroups.erase(context_id);
}

} // namespace bvh::ri