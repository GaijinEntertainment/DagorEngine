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
#include <memory/dag_framemem.h>
#include <memory/dag_linearHeapAllocator.h>

#include "bvh_color_from_pos.h"
#include "bvh_ri_common.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("raytracing", invalidate_ri_ex_instances, false);

CONSOLE_BOOL_VAL("raytracing", bvh_ri_extra_split_enabled, true);
CONSOLE_INT_VAL("raytracing", bvh_ri_extra_split_chunk_budget, 6400, 1000, 100000);

#if DAGOR_DBGLEVEL > 0
extern bool bvh_ri_extra_range_enable;
extern float bvh_ri_extra_range;
#else
static constexpr bool bvh_ri_extra_range_enable = false;
static constexpr float bvh_ri_extra_range = 0;
#endif

namespace bvh::ri
{
static OSReadWriteLock flag_lock;

OSSpinlock typeDirtListLock;
static eastl::unordered_set<int> typeDirtList DAG_TS_GUARDED_BY(typeDirtListLock) = {-1};
static eastl::unordered_set<int> typeDirtListStaging DAG_TS_GUARDED_BY(typeDirtListLock) = {-1};

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
    data.metaAllocId = context_id->allocateMetaRegion(1, "riExtraFlag");

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

struct RiextraCacheHeapManager
{
  struct Heap
  {
    Tab<vec4f> cacheDataBsphereArr;
    Tab<float> cacheDataPoolRadSqArr;
    Tab<rendinst::riex_handle_t> cacheDataHandleArr;
  };

  template <typename T>
  static void copyVecImpl(Tab<T> &to, size_t to_offset, const Tab<T> &from, size_t from_offset, size_t len)
  {
    if (to.data() == from.data())
    {
      memmove(to.begin() + to_offset, from.begin() + from_offset, len * sizeof(T));
    }
    else
    {
      memcpy(to.begin() + to_offset, from.begin() + from_offset, len * sizeof(T));
    }
  }

  void copy(Heap &to, size_t to_offset, const Heap &from, size_t from_offset, size_t len)
  {
    G_ASSERT_RETURN(from.cacheDataBsphereArr.size() == from.cacheDataPoolRadSqArr.size() &&
                      from.cacheDataBsphereArr.size() == from.cacheDataHandleArr.size(), );
    G_ASSERT_RETURN(from_offset + len <= from.cacheDataBsphereArr.size(), );
    G_ASSERT_RETURN(to_offset + len <= to.cacheDataBsphereArr.size(), );

    copyVecImpl(to.cacheDataBsphereArr, to_offset, from.cacheDataBsphereArr, from_offset, len);
    copyVecImpl(to.cacheDataPoolRadSqArr, to_offset, from.cacheDataPoolRadSqArr, from_offset, len);
    copyVecImpl(to.cacheDataHandleArr, to_offset, from.cacheDataHandleArr, from_offset, len);
  }
  bool canCopyInSameHeap() const { return true; }
  bool canCopyOverlapped() const { return true; }
  bool create(Heap &h, size_t sz, ResourceTagType)
  {
    h.cacheDataBsphereArr.resize(sz);
    h.cacheDataPoolRadSqArr.resize(sz);
    h.cacheDataHandleArr.resize(sz);
    return true;
  }
  void orphan(Heap &h)
  {
    h.cacheDataBsphereArr.clear();
    h.cacheDataPoolRadSqArr.clear();
    h.cacheDataHandleArr.clear();
  }
};

using RiextraHandleHeap = LinearHeapAllocator<RiextraCacheHeapManager>;

struct RiExtraGroup
{
  RiextraHandleHeap::RegionId regionPtr;
  RiextraHandleHeap::Region regionCache;
  int regionSplitOffset;
  float maxLodDistSq;
  int weight;
  int riType;
};

alignas(hardware_destructive_interference_size) static RiextraHandleHeap handleHeap;


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
  vec4f lightDirection;
  unsigned handleCount;
  unsigned instanceCount;
  unsigned elemCount;

  dag::AtomicInteger<int> *nextGroupIx = nullptr;
  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> newUniqueRiExtraTreeBuffers[Context::maxUniqueLods];
  RiExtraBVHJob *otherFlip = nullptr;
  threadpool::JobPriority prio = threadpool::PRIO_DEFAULT;
  dag::AtomicInteger<unsigned> chunkBudgetItems = 0;
  bool hitChunkBudget = false;
  bool globalObjectTessellationEnabled = false;

  int fetchNextGroupIx() { return nextGroupIx->fetch_add(1); }

  static ReferencedTransformData *mapTreeRiEx(ContextId context_id, uint64_t object_id, vec4f pos, rendinst::riex_handle_t handle,
    int lod_ix, void *user_data, bool &recycled, int &anim_index)
  {
    struct RiExtraTreeHash
    {
      inline static uint64_t hash(uint64_t object_id, vec4f pos, rendinst::riex_handle_t handle)
      {
        G_UNUSED(object_id);
        G_UNUSED(pos);
        uint64_t id = handle;
        return id;
      }
    };
    MapTreePointers pointers;
    pointers.uniqueTreeBuffers = make_span(context_id->uniqueRiExtraTreeBuffers, Context::maxUniqueLods);
    pointers.newUniqueTreeBuffers =
      make_span(static_cast<RiExtraBVHJob *>(user_data)->newUniqueRiExtraTreeBuffers, Context::maxUniqueLods);
    pointers.contextId = context_id;
    pointers.freeUniqueTreeBLASes = &context_id->freeUniqueRiExtraTreeBLASes;
    return map_tree_base<RiExtraTreeHash, false>(object_id, pos, handle, lod_ix, recycled, anim_index, pointers);
  }

  const char *getJobName(bool &) const override { return "RiExtraBVHJob"; }

  template <bool useMinLod, bool cullDistIncreased, bool riBaked, bool rangeCheck, bool treeOrFlagsCheck, bool instanceHasNode>
  void doJobHotPath(int riType, IPoint2 interval, float maxLodDistSq, float distSqMul, float distSqMulOOC, float rangeCheckSq,
    vec4f viewPositionVec, vec4f viewPositionX, vec4f viewPositionY, vec4f viewPositionZ)
    DAG_TS_REQUIRES_SHARED(contextId->objectsLock)
  {
    auto riRes = rendinst::getRIGenExtraRes(riType);
    auto bestLod = riRes->getQlBestLod();

    const rendinst::riex_handle_t *__restrict cacheDataHandleArr = handleHeap.getHeap().cacheDataHandleArr.data();
    const vec4f *__restrict cacheDataBsphereArr = handleHeap.getHeap().cacheDataBsphereArr.data();
    const float *__restrict cacheDataPoolRadSqArr = handleHeap.getHeap().cacheDataPoolRadSqArr.data();

    const E3DCOLOR *colors = nullptr;
    bool isTessellated = false;
    if constexpr (treeOrFlagsCheck)
      colors = rendinst::getRIGenExtraColors(riType);
    else if (riRes->isTessellated())
      isTessellated = rendinst::isRIGenExtraRendinstClipmap(riType) || globalObjectTessellationEnabled;

    if constexpr (useMinLod)
      bestLod = max<int>(bestLod, riRes->getBvhMinLod());

    for (int i = interval.x; i < interval.y; ++i)
    {
      ++handleCount;

#if _TARGET_SCARLETT
      if ((handleCount & ~32) == 0)
        bvh_yield();
#endif

      float distCheckSq;
      vec4f worldBsphereForAnimation;
      if constexpr (!instanceHasNode)
      {
        // The instance has no node. This can happen for some trees which are fallen
        // as far as I have seen it. Lets just use its position and calculate the LOD
        // the old way.

        vec3f pos;
        {
          // pos cannot be cached, as falling trees can make a difference, and cause self-shadowing from inconsistency with raster
          uint32_t hashVal;
          const mat44f *transform_ptr;
          rendinst::riex_handle_t handle = cacheDataHandleArr[i];
          if (!rendinst::isNodeVisible(handle, riType, transform_ptr, hashVal))
            continue;
          G_UNUSED(hashVal);
          pos = transform_ptr->col3;
        }
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

        distCheckSq = distSq * rendinst::getCullDistSqMul();
        auto approxRadius = v_mul(v_length3(v_bbox3_size(lbb)), V_C_HALF);
        worldBsphereForAnimation = v_perm_xyzd(center, approxRadius);
      }
      else
      {
        auto effectiveDistSqMul = distSqMul;
        vec4f bsphere = cacheDataBsphereArr[i];
        if constexpr (cullDistIncreased)
          if (!viewFrustum.testSphereB(bsphere, v_splat_w(bsphere)))
            effectiveDistSqMul = distSqMulOOC;

        float poolRadSq = cacheDataPoolRadSqArr[i];
        auto distSqScaled = v_extract_x(v_length3_sq_x(v_sub(viewPositionVec, bsphere))) * effectiveDistSqMul;
        auto rad = v_extract_w(bsphere);
        auto rad2 = rad * rad;
        auto sdist = distSqScaled / rad2;
        auto distSqScaledNormalized = sdist * poolRadSq;

        if constexpr (rangeCheck)
          if (rangeCheckSq < distSqScaledNormalized)
            continue;

        distCheckSq = distSqScaledNormalized;
        worldBsphereForAnimation = bsphere;
      }

      if (maxLodDistSq <= distCheckSq)
        continue;

      int lastLod;
      bool overMaxLod;
      int preferredLodIx = rendinst::getRIGenExtraPreferableLodRawDistanceUnsafe(riType, distCheckSq, overMaxLod, lastLod);
      G_UNUSED(overMaxLod); // already checked with maxLodDistSq

      uint32_t hashVal;
      const mat44f *transform_ptr;
      rendinst::riex_handle_t handle = cacheDataHandleArr[i];
      // it's pretty much alive at this point, but get a few more safety checks + retrieve transform and hash
      if (DAGOR_UNLIKELY(!rendinst::isNodeVisible(handle, riType, transform_ptr, hashVal)))
        continue;

      if constexpr (riBaked)
        preferredLodIx = lastLod;
      auto lodIx = min<int>(max(preferredLodIx, bestLod), riRes->lods.size() - 1);

      ++instanceCount;


      mat43f transform;
      v_mat44_transpose_to_mat43(transform, *transform_ptr);

      if constexpr (!treeOrFlagsCheck)
      {
        ++elemCount;

        auto objectId = make_relem_mesh_id(riRes->getBvhId(), lodIx, 0);
        hashVal &= 0xFFFF; // Bottom 16 bits are more than enough (bottom 7 should be fine, but we have 23)

        const bool backFaceCulling = isTessellated; // Tessellation (PN triangulation) can cause self-intersection
                                                    // artifacts; backface culling hides them.

        add_riExtra_instance(transform, viewPositionX, viewPositionY, viewPositionZ, contextId, objectId, threadIx, hashVal,
          backFaceCulling);
      }
      else
      {
        // Tree burns fully immediately
        uint32_t additionalDataFlags = rendinst::getRiExtraPerInstanceRenderAdditionalDataFlags(handle);
        bool isBurning = additionalDataFlags & eastl::to_underlying(rendinst::RiExtraPerInstanceDataType::TREE_BURNING);

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

          if (DAGOR_UNLIKELY(is_tree(contextId, elem))) // TODO: still unlikely??
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
              bool isStationary = !instance_needs_animation_broad_phase_with_distance_rate(worldBsphereForAnimation, viewFrustum,
                viewPositionVec, lightDirection);
              if (!isStationary && v_test_vec_x_gt_0(ri_tree_anim_max_distance_sq_v))
                isStationary = // TODO: move all these out of the loop, but check if the batch has trees, also put this under a
                               // template filter
                  v_test_vec_x_gt(v_length3_sq_x(v_sub(worldBsphereForAnimation, viewPositionVec)), ri_tree_anim_max_distance_sq_v);
              bool isOk = isStationary ? handle_tree<map_tree_stationary>(contextId, elem, meshId, lodIx, false, tm44, originalPos,
                                           colors, handle, invWorldTm, treeInfo, metaAllocId, this, true, isBurning, hashVal)
                                       : handle_tree<mapTreeRiEx>(contextId, elem, meshId, lodIx, false, tm44, originalPos, colors,
                                           handle, invWorldTm, treeInfo, metaAllocId, this, false, isBurning, hashVal);
              if (!isOk)
                continue;
              add_riExtra_instance(contextId, meshId, transform, &treeInfo, isStationary, metaAllocId, threadIx);
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
            add_riExtra_instance(transform, viewPositionX, viewPositionY, viewPositionZ, contextId, meshId, threadIx, hashVal, false);
          }
        }
      }
    }
  }

  template <bool useMinLod, bool cullDistIncreased, bool riBaked, bool rangeCheck>
  void doJobImpl(float distSqMul, float distSqMulOOC)
  {
    Context::BvhObjectReadLock objectsGuard(contextId->objectsLock);
    vec4f viewPositionVec = v_make_vec4f(viewPosition.x, viewPosition.y, viewPosition.z, 0);
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
      if (group.regionCache.size == 0)
        continue;

      auto riRes = rendinst::getRIGenExtraRes(group.riType);
      if (DAGOR_UNLIKELY(!riRes))
        continue;

      G_ASSERT_CONTINUE(group.regionSplitOffset <= group.regionCache.size);
      G_ASSERT_CONTINUE(handleHeap.getHeapSize() >= group.regionCache.offset + group.regionCache.size);

      const IPoint2 region0 = IPoint2(group.regionCache.offset, group.regionCache.offset + group.regionSplitOffset);
      const IPoint2 region1 =
        IPoint2(group.regionCache.offset + group.regionSplitOffset, group.regionCache.offset + group.regionCache.size);

      if (riRes->hasTreeOrFlag())
      {
        doJobHotPath<useMinLod, cullDistIncreased, riBaked, rangeCheck, true, true>(group.riType, region0, group.maxLodDistSq,
          distSqMul, distSqMulOOC, rangeCheckSq, viewPositionVec, viewPositionX, viewPositionY, viewPositionZ);
        doJobHotPath<useMinLod, cullDistIncreased, riBaked, rangeCheck, true, false>(group.riType, region1, group.maxLodDistSq,
          distSqMul, distSqMulOOC, rangeCheckSq, viewPositionVec, viewPositionX, viewPositionY, viewPositionZ);
      }
      else
      {
        doJobHotPath<useMinLod, cullDistIncreased, riBaked, rangeCheck, false, true>(group.riType, region0, group.maxLodDistSq,
          distSqMul, distSqMulOOC, rangeCheckSq, viewPositionVec, viewPositionX, viewPositionY, viewPositionZ);
        doJobHotPath<useMinLod, cullDistIncreased, riBaked, rangeCheck, false, false>(group.riType, region1, group.maxLodDistSq,
          distSqMul, distSqMulOOC, rangeCheckSq, viewPositionVec, viewPositionX, viewPositionY, viewPositionZ);
      }

      const int heuristic = handleCount + elemCount * 15;
      if (DAGOR_UNLIKELY(heuristic >= chunkBudgetItems.load(dag::memory_order_relaxed)))
      {
        hitChunkBudget = true;
        return;
      }
    }
  }

  void doJob()
  {
    struct CallHelper
    {
      using jobCallType = void (*)(RiExtraBVHJob &job, float distSqMul, float distSqMulOOC);
      jobCallType callHelper[16] = {
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, false, false, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, false, false, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, false, true, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, false, true, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, true, false, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, true, false, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, true, true, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<false, true, true, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, false, false, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, false, false, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, false, true, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, false, true, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, true, false, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, true, false, true>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, true, true, false>(distSqMul, dSqMulOOC); },
        [](RiExtraBVHJob &job, float distSqMul, float dSqMulOOC) { job.doJobImpl<true, true, true, true>(distSqMul, dSqMulOOC); },
      };

      void operator()(RiExtraBVHJob &job, float distSqMul, float distSqMulOOC, bool cullDistIncreased, bool riBaked, bool rangeCheck,
        bool useMinLod)
      {
        callHelper[(useMinLod ? 8 : 0) + (cullDistIncreased ? 4 : 0) + (riBaked ? 2 : 0) + (rangeCheck ? 1 : 0)](job, distSqMul,
          distSqMulOOC);
      }
    } doJobWrapper;


    float distSqMul = safediv(rendinst::getCullDistSqMul(), dist_mul * dist_mul);
    float distSqMulOOC = override_dist_sq_mul_ooc ? saved_ooc_cull_dist_sq_mul : rendinst::getCullDistSqMul();
    bool cullDistIncreased = distSqMul != distSqMulOOC;
    bool riBaked = contextId->has(Features::RIBaked);
    const bool useMinLod = bvh::ri::get_ri_lod_dist_bias() > 0;

    hitChunkBudget = false;

    doJobWrapper(*this, distSqMul, distSqMulOOC, cullDistIncreased, riBaked, bvh_ri_extra_range_enable, useMinLod);

    DA_PROFILE_TAG_DESC(getJobNameProfDesc(), "handles: %d, instances: %d, elems: %d", handleCount, instanceCount, elemCount);

    // flip-flop scheduling: start the other pair right before this one finishes, so the parallel processing finish is consistent
    if (hitChunkBudget && nextGroupIx->load() < (int)orderedRiExtraHandles.size())
    {
      G_ASSERT(otherFlip);
      threadpool::wait(otherFlip); // just in case it is still running
      parallel_instance_processing::before_job_start(contextId);
      threadpool::add(otherFlip, prio);
    }

    parallel_instance_processing::after_job_end(contextId);
  }
};

struct RIExtraBVHJobGroup
{
  dag::AtomicInteger<int> nextGroupIx;
  RiExtraBVHJob jobs[ri_extra_thread_count][2]; // flip-flop pair
};

static eastl::unordered_map<ContextId, RIExtraBVHJobGroup> riExtraJobGroups;

void wait_ri_extra_instances_update(ContextId context_id)
{
  TIME_PROFILE(bvh::update_ri_extra_instances);
  auto &jobGroup = riExtraJobGroups[context_id];
  for (int threadIx = 0; threadIx < ri_extra_thread_count; ++threadIx)
  {
    // skip budget check and process the rest
    jobGroup.jobs[threadIx][0].chunkBudgetItems.store(~0u, dag::memory_order_relaxed);
    jobGroup.jobs[threadIx][1].chunkBudgetItems.store(~0u, dag::memory_order_relaxed);
  }
  for (int threadIx = 0; threadIx < ri_extra_thread_count; ++threadIx)
  {
    DA_PROFILE_TAG(bvh::update_ri_extra_instances, "wait thread %d", threadIx);
    threadpool::wait(&jobGroup.jobs[threadIx][0]);
    threadpool::wait(&jobGroup.jobs[threadIx][1]);
    threadpool::wait(&jobGroup.jobs[threadIx][0]); // just in case for the edge case when job[0] starts job[1] in-between
  }
}

void prepare_ri_extra_instances()
{
  bool needSort = false;

  // Here we try to make sure that all threads has roughly the same amount of work
  {
    TIME_PROFILE(collect);

    struct TmpCacheData
    {
      rendinst::riex_handle_t handle;
      float poolRadSq;
      vec4f bsphere;
    };
    Tab<TmpCacheData> tmpCacheVec0;
    Tab<rendinst::riex_handle_t> tmpCacheVec1;

    eastl::unordered_set<int> newDirtList;

    Tab<rendinst::riex_handle_t> tmpHandles;
    auto processType = [&newDirtList, &tmpHandles, &tmpCacheVec0, &tmpCacheVec1](int riType, RiExtraGroup &group) {
      group.riType = riType;
      group.weight = 0;
      handleHeap.free(group.regionPtr);
      group.regionPtr.reset();
      group.regionCache = {};
      group.regionSplitOffset = 0;

      int elemCount = 0;

      auto riRes = rendinst::getRIGenExtraRes(riType);
      if (!riRes || riRes->getBvhId() == 0)
      {
        // Not yet loaded. Put it into the dirt list so it will be checked later.
        newDirtList.insert(riType);
        return;
      }


      tmpHandles.clear();
      if (rendinst::getRiGenExtraInstances(tmpHandles, riType) > 0)
      {
        if (!riRes->hasTreeOrFlag())
          elemCount = 1;
        else
        {
          int bestLod = min<int>(max<int>(riRes->getQlBestLod(), riRes->getBvhMinLod()), riRes->lods.size() - 1);
          auto &lod = riRes->lods[bestLod];
          auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

          bool hasIndices;
          for (auto [elemIx, elem] : enumerate(elems))
            elemCount += elem.vertexData->isRenderable(hasIndices) ? 1 : 0;
        }

        if (elemCount == 0)
          return;

        group.maxLodDistSq = rendinst::getMaxLodDist(riType);

        tmpCacheVec0.clear();
        tmpCacheVec1.clear();
        tmpCacheVec0.reserve(tmpHandles.size());
        // at most, only a few percent of instances will have invalid bsphere, so we don't need to reserve for tmpCacheVec1
        for (const auto handle : tmpHandles)
        {
          vec4f bsphere;
          float poolRad;
          if (rendinst::isNodeAlive(handle, bsphere, poolRad))
          {
            TmpCacheData cache;
            cache.handle = handle;
            cache.bsphere = bsphere;
            cache.poolRadSq = poolRad * poolRad;
            if (DAGOR_UNLIKELY(v_check_xyzw_all_true(v_cmp_eq(bsphere, v_zero()))))
              tmpCacheVec1.push_back(handle);
            else
              tmpCacheVec0.push_back(cache);
          }
        }
        const int allocSize = tmpCacheVec0.size() + tmpCacheVec1.size();
        if (allocSize == 0)
          return;

        if (!handleHeap.canAllocate(allocSize))
        {
          int sizeIncrement = handleHeap.getHeapSize() / 2; // +50% capcacity
          int newCapacity = handleHeap.getHeapSize() + max<int>(allocSize, sizeIncrement);
          handleHeap.resize(newCapacity);
        }
        G_ASSERT(handleHeap.canAllocate(allocSize));
        const RiextraHandleHeap::RegionId handlePtr = handleHeap.allocateInHeap(allocSize);
        G_ASSERT(handlePtr);
        const RiextraHandleHeap::Region region = handleHeap.get(handlePtr);
        auto &cacheDataHandleArr = handleHeap.getHeap().cacheDataHandleArr;
        auto &cacheDataBsphereArr = handleHeap.getHeap().cacheDataBsphereArr;
        auto &cacheDataPoolRadSqArr = handleHeap.getHeap().cacheDataPoolRadSqArr;
        for (int i = 0; i < tmpCacheVec0.size(); ++i)
        {
          cacheDataHandleArr[region.offset + i] = tmpCacheVec0[i].handle;
          cacheDataBsphereArr[region.offset + i] = tmpCacheVec0[i].bsphere;
          cacheDataPoolRadSqArr[region.offset + i] = tmpCacheVec0[i].poolRadSq;
        }
        group.regionSplitOffset = tmpCacheVec0.size();
        const int splitOffset = region.offset + group.regionSplitOffset;
        for (int i = 0; i < tmpCacheVec1.size(); ++i)
        {
          cacheDataHandleArr[splitOffset + i] = tmpCacheVec1[i];
          cacheDataBsphereArr[splitOffset + i] = v_zero(); // unused
          cacheDataPoolRadSqArr[splitOffset + i] = 0;      // unused
        }
        group.regionPtr = handlePtr;
        group.weight = region.size * elemCount;
      }
    };

    decltype(typeDirtListStaging) typeDirtListCopy;
    {
      OSSpinlockScopedLock lock(typeDirtListLock);
      typeDirtListCopy.swap(typeDirtListStaging);
    }

    if (need_type_reset(typeDirtListCopy) || invalidate_ri_ex_instances)
    {
      invalidate_ri_ex_instances = false;
      rendinst::ScopedRIExtraReadLock rd;
      rendinst::AllRendinstExtraScenesLock sceneLock;

      handleHeap.clear();

      int totalRiTypes = rendinst::getRiGenExtraResCount();
      allRiExtraHandles.clear();
      for (int riType = 0; riType < totalRiTypes; ++riType)
        processType(riType, allRiExtraHandles[riType]);

      needSort = true;
    }
    else
    {
      if (!typeDirtListCopy.empty())
      {
        rendinst::ScopedRIExtraReadLock rd;
        rendinst::AllRendinstExtraScenesLock sceneLock;

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

  {
    TIME_PROFILE(refresh_cache);
    for (auto &group : allRiExtraHandles)
      group.second.regionCache = handleHeap.get(group.second.regionPtr);
  }
}

void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &bvh_frustum,
  const Frustum &view_frustum, const Point3 &light_direction, threadpool::JobPriority prio)
{
  {
    OSSpinlockScopedLock lock(typeDirtListLock);
    for (auto e : typeDirtList)
      typeDirtListStaging.insert(e);
    typeDirtList.clear();
  }

  auto &jobGroup = riExtraJobGroups[context_id];

  jobGroup.nextGroupIx.store(0);

  const vec4f lightDirection = v_ldu_p3_safe(&light_direction.x);
  const unsigned chunkBudgetItems = bvh_ri_extra_split_enabled ? (unsigned)bvh_ri_extra_split_chunk_budget.get() : ~0u;
  const bool globalObjectTessellationEnabled = is_global_object_tessellation_enabled();

  // Sets up and extra job for the main thread, just in case these don't finish in time
  for (int threadIx = 0; threadIx < get_ri_extra_worker_count(); ++threadIx)
  {
    for (int flip = 0; flip < 2; ++flip)
    {
      auto &job = jobGroup.jobs[threadIx][flip];
      job.contextId = context_id;
      job.threadIx = threadIx;
      job.viewPosition = view_position;
      job.nextGroupIx = &jobGroup.nextGroupIx;
      job.bvhFrustum = bvh_frustum;
      job.viewFrustum = view_frustum;
      job.lightDirection = lightDirection;
      job.prio = prio;
      job.otherFlip = &jobGroup.jobs[threadIx][flip ^ 1];
      job.chunkBudgetItems.store(chunkBudgetItems, dag::memory_order_relaxed);
      job.globalObjectTessellationEnabled = globalObjectTessellationEnabled;
    }
    parallel_instance_processing::before_job_start(context_id);
    threadpool::add(&jobGroup.jobs[threadIx][0], prio);
  }
}

void on_scene_loaded_ri_ex(ContextId) { do_type_reset(); }

void on_unload_scene_ri_ex(ContextId context_id)
{
  allRiExtraHandles.clear();
  handleHeap.clear();
  riExtraJobGroups.erase(context_id);

  do_type_reset();
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

void tidy_up_riex_trees(ContextId context_id)
{
  TIME_PROFILE(tidy_up_riex_trees)

  for (auto &jobFlips : riExtraJobGroups[context_id].jobs)
  {
    for (int flip = 0; flip < 2; ++flip)
    {
      auto &job = jobFlips[flip];
      for (auto [lodIndex, lodRes] : enumerate(job.newUniqueRiExtraTreeBuffers))
      {
        for (auto &tree : lodRes)
        {
          auto [it, inserted] = context_id->uniqueRiExtraTreeBuffers[lodIndex].insert(std::move(tree));
          if (!inserted)
            for (auto &elem : tree.second.elems)
              context_id->freeMetaRegion(elem.second.metaAllocId);
        }
        lodRes.clear();
      }
    }
  }

  TidyUpTreePointers pointers;
  pointers.uniqueTreeBuffers = make_span(context_id->uniqueRiExtraTreeBuffers, Context::maxUniqueLods);
  pointers.contextId = context_id;
  pointers.freeUniqueTreeBLASes = &context_id->freeUniqueRiExtraTreeBLASes;
  tidy_up_trees_base<false>(context_id, pointers);
}

} // namespace bvh::ri