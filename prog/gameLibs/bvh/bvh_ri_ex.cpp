// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <math/dag_frustum.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/riexHandle.h>
#include <generic/dag_enumerate.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_rwLock.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("raytracing", ri_ex_work_on_main, true);

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_extra_range_enable;
extern float bvh_ri_extra_range;
#else
static constexpr bool bvh_ri_extra_range_enable = false;
static constexpr float bvh_ri_extra_range = 0;
#endif

namespace bvh::ri
{

static OSReadWriteLock tree_lock;
static OSReadWriteLock flag_lock;

OSSpinlock typeDirtListLock;
static eastl::unordered_set<int> typeDirtList DAG_TS_GUARDED_BY(typeDirtListLock) = {-1};
static bool need_type_reset(const auto &t) { return t.size() == 1 && *t.begin() == -1; }
static void do_type_reset()
{
  OSSpinlockScopedLock lock(typeDirtListLock);
  typeDirtList = {-1};
}

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

  flag_lock.lockRead();
  ReferencedTransformData *dataPtr = nullptr;
  if (auto handleIter = context_id->uniqueRiExtraFlagBuffers.find(handle); handleIter != context_id->uniqueRiExtraFlagBuffers.cend())
  {
    if (auto meshIdIter = handleIter->second.find(mesh_id); meshIdIter != handleIter->second.cend())
    {
      dataPtr = &meshIdIter->second;
    }
  }
  flag_lock.unlockRead();
  if (dataPtr == nullptr)
  {
    flag_lock.lockWrite();
    dataPtr = &context_id->uniqueRiExtraFlagBuffers[handle][mesh_id];
    flag_lock.unlockWrite();
  }
  auto &data = *dataPtr;

  if (data.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
    data.metaAllocId = context_id->allocateMetaRegion(1);

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

struct RiExtraGroup
{
  int bvhId;
  int weight;
  int elemCount;
  int riType;
  int bestLod;
  Tab<rendinst::riex_handle_t> handles;
};

static eastl::unordered_map<int, RiExtraGroup> allRiExtraHandles;
static dag::Vector<eastl::pair<int, int>> orderedRiExtraHandles;

static float dist_mul = 1;
bool override_dist_sq_mul_ooc = false;
static float saved_ooc_cull_dist_sq_mul = 0;

struct RiExtraBVHJob : public cpujobs::IJob
{
  ContextId contextId;
  int threadIx;
  Point3 viewPosition;
  Frustum bvhFrustum;
  Frustum viewFrustum;
  unsigned handleCount;
  unsigned instanceCount;
  unsigned elemCount;

  dag::AtomicInteger<int> *nextGroupIx = nullptr;

  int fetchNextGroupIx() { return nextGroupIx->fetch_add(1); }

  static ReferencedTransformData *mapTree(ContextId context_id, uint64_t mesh_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, void *user_data, bool &recycled)
  {
    G_UNUSED(pos);
    G_UNUSED(lod_ix);
    G_UNUSED(user_data);

    tree_lock.lockRead();
    ReferencedTransformData *dataPtr = nullptr;
    if (auto handleIter = context_id->uniqueRiExtraTreeBuffers.find(handle); handleIter != context_id->uniqueRiExtraTreeBuffers.cend())
    {
      if (auto meshIdIter = handleIter->second.find(mesh_id); meshIdIter != handleIter->second.cend())
      {
        dataPtr = &meshIdIter->second;
      }
    }
    tree_lock.unlockRead();
    if (dataPtr == nullptr)
    {
      tree_lock.lockWrite();
      dataPtr = &context_id->uniqueRiExtraTreeBuffers[handle][mesh_id];
      tree_lock.unlockWrite();
    }

    recycled = false;

    return dataPtr;
  }

  const char *getJobName(bool &) const override { return "RiExtraBVHJob"; }

  template <bool cullDistIncreased, bool riBaked, bool rangeCheck>
  void doJobImpl(float distSqMul, float distSqMulOOC)
  {
    vec4f viewPositionVec = v_make_vec4f(viewPosition.x, viewPosition.y, viewPosition.z, 0);

    bool overMaxLod;
    int lastLod;

    vec4f viewPositionX = v_make_vec4f(0, 0, 0, viewPosition.x);
    vec4f viewPositionY = v_make_vec4f(0, 0, 0, viewPosition.y);
    vec4f viewPositionZ = v_make_vec4f(0, 0, 0, viewPosition.z);

    handleCount = instanceCount = elemCount = 0;

    auto rangeCheckSq = sqr(bvh_ri_extra_range);

    rendinst::ScopedRIExtraReadLock rd;
    rendinst::AllRendinstExtraScenesLock sceneLock;

    for (int groupIx = fetchNextGroupIx(); groupIx < orderedRiExtraHandles.size(); groupIx = fetchNextGroupIx())
    {
      auto iter = allRiExtraHandles.find(orderedRiExtraHandles[groupIx].first);
      if (iter == allRiExtraHandles.end())
        continue;
      auto &group = iter->second;
      if (group.handles.empty())
        continue;

      auto riType = group.riType;
      auto riRes = rendinst::getRIGenExtraRes(riType);
      auto colors = rendinst::getRIGenExtraColors(riType);
      auto bestLod = riRes->getQlBestLod();
      bool hasTreeOrFlags = riRes->hasTreeOrFlag();


      for (auto iter = group.handles.begin(); iter != group.handles.end(); ++iter)
      {
        auto nextIter = iter + 1;
        if (nextIter != group.handles.end())
          rendinst::prefetchNode(*nextIter);

        auto handle = *iter;

        ++handleCount;

#if _TARGET_SCARLETT
        if ((handleCount & ~32) == 0)
          bvh_yield();
#endif

        uint32_t hashVal;
        const mat43f *transform_ptr;
        vec3f pos;

        // Approximation to avoid needing to calculate the bsphere, since that's slow.
        // The accurate bbox calculation depends on poolRad2/rad2, so need to take the minimum of that to not throw away objects
        constexpr float ESTIMATION_FACTOR = 0.05;
        if (!rendinst::isVisibleByLodEst(handle, viewPositionVec, distSqMul * ESTIMATION_FACTOR, transform_ptr, pos, hashVal))
          continue;

        const mat43f &transform = *transform_ptr;

        int preferredLodIx;

        float poolRad;
        auto bsphere = rendinst::getNodeBsphere(handle, poolRad);
        if (DAGOR_UNLIKELY(v_check_xyzw_all_true(v_cmp_eq(bsphere, v_zero()))))
        {
          // The instance has no node. This can happen for some trees which are fallen
          // as far as I have seen it. Lets just use its position and calculate the LOD
          // the old way.

          bbox3f lbb = rendinst::riex_get_lbb(riType);
          vec4f center = v_bbox3_center(lbb);
          auto riPosition = v_madd(center, v_make_vec4f(0.0f, 1.0f, 0.0f, 0.0f), pos);
          vec4f difference = v_sub(riPosition, viewPositionVec);
          float distSq = v_extract_x(v_dot3(difference, difference));
          if (distSq < 0)
            distSq = 0;

          if constexpr (rangeCheck)
            if (rangeCheckSq < distSq)
              continue;

          preferredLodIx = rendinst::getRIGenExtraPreferrableLod(riType, distSq, overMaxLod, lastLod);
        }
        else
        {
          auto effectiveDistSqMul = distSqMul;
          if constexpr (cullDistIncreased)
            if (!viewFrustum.testSphereB(bsphere, v_splat_w(bsphere)))
              effectiveDistSqMul = distSqMulOOC;

          auto poolRad2 = poolRad * poolRad;
          auto distSqScaled = v_extract_x(v_length3_sq_x(v_sub(viewPositionVec, bsphere))) * effectiveDistSqMul;
          auto rad = v_extract_w(bsphere);
          auto rad2 = rad * rad;
          auto sdist = distSqScaled / rad2;
          auto distSqScaledNormalized = sdist * poolRad2;

          if constexpr (rangeCheck)
            if (rangeCheckSq < distSqScaledNormalized)
              continue;

          preferredLodIx = rendinst::getRIGenExtraPreferrableLodRawDistance(riType, distSqScaledNormalized, overMaxLod, lastLod);
        }

        if (preferredLodIx < 0 || overMaxLod)
          continue;
        if constexpr (riBaked)
          preferredLodIx = lastLod;
        auto lodIx = max(preferredLodIx, bestLod);

        ++instanceCount;

        if (!hasTreeOrFlags)
        {
          ++elemCount;

          auto objectId = make_relem_mesh_id(riRes->getBvhId(), lodIx, 0);
          hashVal &= 0xFFFF; // Bottom 16 bits are more than enough (bottom 7 should be fine, but we have 23)
          add_riExtra_instance(transform, viewPositionX, viewPositionY, viewPositionZ, contextId, objectId, threadIx, hashVal);
        }
        else
        {
          auto &lod = riRes->lods[lodIx];
          auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

          eastl::optional<TMatrix4> invWorldTm;

          for (auto [elemIx, elem] : enumerate(elems))
          {
            bool hasIndices;
            if (!elem.vertexData->isRenderable(hasIndices))
              continue;

            ++elemCount;

            auto meshId = make_relem_mesh_id(riRes->getBvhId(), lodIx, elemIx);

            if (DAGOR_UNLIKELY(is_tree(contextId, elem)))
            {
              mat44f tm44;
              v_mat43_transpose_to_mat44(tm44, transform);
              tm44.col3 = v_add(tm44.col3, v_make_vec4f(0, 0, 0, 1));

              bbox3f aabb = {v_ldu(&riRes->bbox.boxMin().x), v_ldu_p3_safe(&riRes->bbox.boxMax().x)};
              bbox3f aabbWorld;
              v_bbox3_init(aabbWorld, tm44, aabb);

              if (bvhFrustum.testBox(aabbWorld.bmin, aabbWorld.bmax))
              {
                dag::ConstSpan<rendinst::RiExtraPerInstanceGpuItem> originalPosData =
                  rendinst::getRiExtraPerInstanceRenderAdditionalData(handle, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS);
                vec4f originalPos;
                if (originalPosData.size())
                  originalPos = v_ldu(reinterpret_cast<const float *>(&originalPosData[0]));
                else
                  originalPos = tm44.col3;

                // This code is to dump riExtra tree names
                // String name;
                // if (resolve_game_resource_name(name, riRes))
                //  debug("riExtra tree: %s", name.data());
                TreeInfo treeInfo;
                MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
                if (!handle_tree<mapTree>(contextId, elem, meshId, lodIx, false, tm44, originalPos, colors, handle, invWorldTm,
                      treeInfo, metaAllocId, this))
                  continue;
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

              if (bvhFrustum.testBox(aabbWorld.bmin, aabbWorld.bmax))
              {
                FlagInfo flagInfo;
                flagInfo.data.hashVal = hashVal;
                MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
                handle_flag(contextId, elem, meshId, tm44, handle, invWorldTm, flagInfo, metaAllocId);
                add_riExtra_instance(contextId, meshId, transform, &flagInfo, metaAllocId, threadIx);
              }
            }
            else
            {
              hashVal &= 0xFFFF; // Bottom 16 bits are more than enough (bottom 7 should be fine, but we have 23)
              add_riExtra_instance(transform, viewPositionX, viewPositionY, viewPositionZ, contextId, meshId, threadIx, hashVal);
            }
          }
        }
      }
    }
  }

  void doJob()
  {
    struct CallHelper
    {
      using jobCallType = void (*)(RiExtraBVHJob &job, float distSqMul, float distSqMulOOC);
      jobCallType callHelper[8] = {
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<false, false, false>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<false, false, true>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<false, true, false>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<false, true, true>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<true, false, false>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<true, false, true>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<true, true, false>(distSqMul, distSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float distSqMulOOC) { job.doJobImpl<true, true, true>(distSqMul, distSqMulOOC); },
      };

      void operator()(RiExtraBVHJob &job, float distSqMul, float distSqMulOOC, bool cullDistIncreased, bool riBaked, bool rangeCheck)
      {
        callHelper[(cullDistIncreased ? 4 : 0) + (riBaked ? 2 : 0) + (rangeCheck ? 1 : 0)](job, distSqMul, distSqMulOOC);
      }
    } doJobWrapper;


    float distSqMul = safediv(rendinst::getCullDistSqMul(), dist_mul * dist_mul);
    float distSqMulOOC = override_dist_sq_mul_ooc ? saved_ooc_cull_dist_sq_mul : rendinst::getCullDistSqMul();
    bool cullDistIncreased = distSqMul != distSqMulOOC;
    bool riBaked = contextId->has(Features::RIBaked);
    doJobWrapper(*this, distSqMul, distSqMulOOC, cullDistIncreased, riBaked, bvh_ri_extra_range_enable);

    DA_PROFILE_TAG_DESC(getJobNameProfDesc(), "handles: %d, instances: %d, elems: %d", handleCount, instanceCount, elemCount);
  }
};

struct RIExtraBVHJobGroup
{
  dag::AtomicInteger<int> nextGroupIx;
  RiExtraBVHJob jobs[ri_extra_thread_count];
};

static eastl::unordered_map<ContextId, RIExtraBVHJobGroup> riExtraJobGroups;

void wait_ri_extra_instances_update(ContextId context_id, bool do_work)
{
  if (do_work && ri_ex_work_on_main && get_ri_extra_worker_count() < ri_extra_thread_count)
  {
    // better do some real work instead of idle waiting
    // doJob will just early return if it's finished
    TIME_PROFILE(do_work_dont_wait)
    riExtraJobGroups[context_id].jobs[get_ri_extra_worker_count()].doJob();
  }
  for (int threadIx = 0; threadIx < ri_extra_thread_count; ++threadIx)
  {
    TIME_PROFILE_NAME(bvh::update_ri_extra_instances,
      String(128, "wait ri_extra_bvh_job for %s: %d", context_id->name.data(), threadIx));
    threadpool::wait(&riExtraJobGroups[context_id].jobs[threadIx]);
  }
}

void prepare_ri_extra_instances()
{
  bool needSort = false;

  // Here we try to make sure that all threads has roughly the same amount of work
  {
    TIME_PROFILE(collect);

    eastl::unordered_set<int> newDirtList;

    auto processType = [&newDirtList](int riType, RiExtraGroup &group) {
      group.riType = riType;
      group.weight = 0;
      group.handles.clear();
      group.elemCount = 0;

      auto riRes = rendinst::getRIGenExtraRes(riType);
      group.bvhId = riRes->getBvhId();
      if (group.bvhId == 0)
      {
        // Not yet loaded. Put it into the dirt list so it will be checked later.
        newDirtList.insert(riType);
        return;
      }

      group.bestLod = riRes->getQlBestLod();

      if (rendinst::getRiGenExtraInstances(group.handles, riType) > 0)
      {
        if (!riRes->hasTreeOrFlag())
          group.elemCount = 1;
        else
        {
          auto &lod = riRes->lods[group.bestLod];
          auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

          bool hasIndices;
          for (auto [elemIx, elem] : enumerate(elems))
            group.elemCount += elem.vertexData->isRenderable(hasIndices) ? 1 : 0;
        }

        if (group.elemCount == 0)
          return;

        group.weight = group.handles.size() * group.elemCount;
      }
    };

    decltype(typeDirtList) typeDirtListCopy;
    {
      OSSpinlockScopedLock lock(typeDirtListLock);
      typeDirtListCopy.swap(typeDirtList);
    }

    if (need_type_reset(typeDirtListCopy))
    {
      rendinst::ScopedRIExtraReadLock rd;

      for (auto &group : allRiExtraHandles)
        group.second.handles.clear();

      int totalRiTypes = rendinst::getRiGenExtraResCount();

      for (int riType = 0; riType < totalRiTypes; ++riType)
        processType(riType, allRiExtraHandles[riType]);

      needSort = true;
    }
    else
    {
      if (!typeDirtListCopy.empty())
      {
        rendinst::ScopedRIExtraReadLock rd;

        for (auto riType : typeDirtListCopy)
        {
          auto &group = allRiExtraHandles[riType];
          processType(riType, group);
        }

        needSort = true;
      }
    }

    OSSpinlockScopedLock lock(typeDirtListLock);
    if (!need_type_reset(typeDirtList))
      for (auto type : newDirtList)
        typeDirtList.insert(type);
  }

  if (needSort)
  {
    TIME_PROFILE(sort);
    orderedRiExtraHandles.clear();
    for (auto &group : allRiExtraHandles)
      if (group.second.weight)
        orderedRiExtraHandles.push_back({group.first, group.second.weight});

    eastl::sort(orderedRiExtraHandles.begin(), orderedRiExtraHandles.end(),
      [](const auto &a, const auto &b) { return a.second > b.second; });
  }
}

void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &bvh_frustum,
  const Frustum &view_frustum, threadpool::JobPriority prio)
{
  auto &jobGroup = riExtraJobGroups[context_id];

  jobGroup.nextGroupIx.store(0);

  // Sets up and extra job for the main thread, just in case these don't finish in time
  for (int threadIx = 0; threadIx < min(ri_extra_thread_count, get_ri_extra_worker_count() + 1); ++threadIx)
  {
    auto &job = jobGroup.jobs[threadIx];
    job.contextId = context_id;
    job.threadIx = threadIx;
    job.viewPosition = view_position;
    job.nextGroupIx = &jobGroup.nextGroupIx;
    job.bvhFrustum = bvh_frustum;
    job.viewFrustum = view_frustum;
    if (threadIx < get_ri_extra_worker_count())
      threadpool::add(&job, prio);
  }
}

void on_scene_loaded_ri_ex(ContextId) { do_type_reset(); }

void on_unload_scene_ri_ex(ContextId context_id)
{
  allRiExtraHandles.clear();
  riExtraJobGroups.erase(context_id);

  do_type_reset();
}

static void ri_ex_callback(int riType)
{
  OSSpinlockScopedLock lock(typeDirtListLock);
  if (!need_type_reset(typeDirtList))
    typeDirtList.insert(riType);
}

static void ri_ex_callback(rendinst::riex_handle_t handle)
{
  OSSpinlockScopedLock lock(typeDirtListLock);
  if (!need_type_reset(typeDirtList))
    typeDirtList.insert(rendinst::handle_to_ri_type(handle));
}

static void ri_ex_add_callback(rendinst::riex_handle_t handle, bool) { ri_ex_callback(handle); }

void init_ex()
{
  rendinst::registerRIGenExtraAddedCb(ri_ex_add_callback);
  rendinst::registerRIGenExtraInvalidateHandleCb(ri_ex_callback);
}

void teardown_ex()
{
  rendinst::unregisterRIGenExtraAddedCb(ri_ex_add_callback);
  rendinst::unregisterRIGenExtraInvalidateHandleCb(ri_ex_callback);
}

void set_dist_mul(float mul) { dist_mul = mul; }

void override_out_of_camera_ri_dist_mul(float dist_sq_mul_ooc)
{
  override_dist_sq_mul_ooc = true;
  saved_ooc_cull_dist_sq_mul = dist_sq_mul_ooc;
}

} // namespace bvh::ri