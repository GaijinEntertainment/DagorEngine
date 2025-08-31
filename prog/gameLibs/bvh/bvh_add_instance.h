// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "util/dag_compilerDefs.h"
#include <bvh/bvh.h>
#include <math/dag_dxmath.h>

#define BVH_INLINE __forceinline

namespace bvh
{

BVH_INLINE void VECTORCALL add_instance(ContextId context_id, Context::InstanceMap &instanceMap, uint64_t object_id, mat43f transform,
  const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode,
  MeshSkinningInfo *skinning_info, MeshHeliRotorInfo *heli_rotor_info, TreeInfo *tree_info, LeavesInfo *leaves_info,
  FlagInfo *flag_info, DeformedInfo *deformed_info, MeshMetaAllocator::AllocId meta_alloc_id)
{
  auto objectIter = context_id->objects.find(object_id);

  if (objectIter == context_id->objects.end())
    return;

  // The VN RT Validation layer emits a warning if the scale is zero.
  // It is enough to check one axis only. Having an instance with 0 scale on X but not on
  // YZ is unlikely.
  if (v_check_xyz_all_true(v_cmp_eq(transform.row0, v_zero())))
    return;

  auto &instance = *static_cast<Context::Instance *>(instanceMap.push_back_uninitialized());
  memset(&instance, 0, sizeof(instance));
  instance.transform = transform;
  instance.objectId = object_id;
  instance.uniqueIsRecycled = false;
  instance.noShadow = no_shadow;
  instance.animationUpdateMode = animation_update_mode;
  if (per_instance_data)
    instance.perInstanceData = *per_instance_data;
  if (skinning_info)
  {
    instance.invWorldTm = skinning_info->invWorldTm;
    instance.setTransformsFn = skinning_info->setTransformsFn;
    instance.uniqueTransformedBuffer = skinning_info->skinningBuffer;
    instance.uniqueBlas = skinning_info->skinningBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (heli_rotor_info)
  {
    instance.invWorldTm = heli_rotor_info->invWorldTm;
    instance.getHeliParamsFn = heli_rotor_info->getParamsFn;
    instance.uniqueTransformedBuffer = heli_rotor_info->transformedBuffer;
    instance.uniqueBlas = heli_rotor_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (tree_info)
  {
    instance.invWorldTm = tree_info->invWorldTm;
    instance.uniqueTransformedBuffer = tree_info->transformedBuffer;
    instance.uniqueBlas = tree_info->transformedBlas;
    instance.tree = tree_info->data;
    instance.metaAllocId = meta_alloc_id;
    instance.hasInstanceColor = true;
    instance.uniqueIsRecycled = tree_info->recycled;

    G_ASSERT(!instance.perInstanceData.has_value());
    instance.perInstanceData = UPoint2{instance.tree.color.u, 0};
  }
  else if (leaves_info)
  {
    instance.invWorldTm = leaves_info->invWorldTm;
    instance.uniqueTransformedBuffer = leaves_info->transformedBuffer;
    instance.uniqueBlas = leaves_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
    instance.uniqueIsRecycled = leaves_info->recycled;
  }
  else if (flag_info)
  {
    instance.invWorldTm = flag_info->invWorldTm;
    instance.uniqueTransformedBuffer = flag_info->transformedBuffer;
    instance.uniqueBlas = flag_info->transformedBlas;
    instance.flag = flag_info->data;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (deformed_info)
  {
    instance.invWorldTm = deformed_info->invWorldTm;
    instance.getDeformParamsFn = deformed_info->getParamsFn;
    instance.uniqueTransformedBuffer = deformed_info->transformedBuffer;
    instance.uniqueBlas = deformed_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else
    instance.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
}

BVH_INLINE void VECTORCALL add_instance(Context::InstanceMap &instanceMap, uint64_t object_id, mat43f transform,
  const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode)
{
  auto &instance = *static_cast<Context::Instance *>(instanceMap.push_back_uninitialized());
  memset(&instance, 0, sizeof(instance));
  instance.transform = transform;
  instance.objectId = object_id;
  instance.noShadow = no_shadow;
  instance.animationUpdateMode = animation_update_mode;
  instance.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  if (per_instance_data)
    instance.perInstanceData = *per_instance_data;
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t object_id, mat43f transform, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    MeshMetaAllocator::INVALID_ALLOC_ID);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t object_id, mat43f transform, TreeInfo *tree_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, tree_info, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t object_id, mat43f transform, LeavesInfo *leaves_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, leaves_info, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(mat43f transform, vec4f pX, vec4f pY, vec4f pZ, ContextId context_id,
  uint64_t object_id, int thread_ix, uint32_t hashVal)
{
  auto objectIter = context_id->objects.find(object_id);
  if (objectIter == context_id->objects.end())
    return;

  auto &object = objectIter->second;

  if (DAGOR_UNLIKELY(!object.blas || object.isAnimated))
    return;

  auto &perInstanceData = context_id->riExtraInstanceData[thread_ix];

  auto &desc = *(HWInstance *)context_id->riExtraInstances[thread_ix].push_back_uninitialized();
  desc.transform.row0 = v_sub(transform.row0, pX);
  desc.transform.row1 = v_sub(transform.row1, pY);
  desc.transform.row2 = v_sub(transform.row2, pZ);
  desc.instanceID = MeshMetaAllocator::decode(object.metaAllocId);
  desc.instanceMask = bvhGroupRiExtra;
  desc.instanceContributionToHitGroupIndex = [&]() -> uint32_t {
    if (hashVal)
    {
      if constexpr (use_icthgi_for_per_instance_data)
      {
        uint32_t packedHashVal = (hashVal & 0x001FFFFF) << 3;
        return packedHashVal | ICTHGI_USE_AS_HASH_VALUE;
      }
      else
        return ICTHGI_LOAD_PER_INSTANCE_DATA_WITH_INSTANCE_INDEX;
    }
    else
      return ICTHGI_NO_PER_INSTANCE_DATA;
  }();
  desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
  desc.blasGpuAddress = object.blas.getGPUAddress();

  if constexpr (!use_icthgi_for_per_instance_data)
  {
    if (hashVal)
    {
      auto packedHash = perInstanceData.push_back_uninitialized();
      memcpy(packedHash, &hashVal, sizeof(hashVal));
    }
    else
    {
      auto packedHash = perInstanceData.push_back_uninitialized();
      memset(packedHash, 0, sizeof(hashVal));
    }
  }
}

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t object_id, mat43f transform, TreeInfo *tree_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraTreeInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, tree_info, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t object_id, mat43f transform, FlagInfo *flag_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraTreeInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, flag_info, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(Context::InstanceMap &instances, uint64_t object_id, mat43f transform,
  const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode)
{
  add_instance(instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode,
  MeshSkinningInfo &skinning_info, MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, &skinning_info,
    nullptr, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode,
  MeshHeliRotorInfo &heli_rotor_info, MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, nullptr,
    &heli_rotor_info, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const UPoint2 *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode,
  DeformedInfo &deformed_info, MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, nullptr, nullptr,
    nullptr, nullptr, nullptr, &deformed_info, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_impostor_instance(ContextId context_id, uint64_t object_id, mat43f transform, uint32_t color,
  int thread_ix, int &meta_alloc_id_accel, uint64_t blas_accel, uint64_t mesh_id_accel)
{
  if (object_id != mesh_id_accel)
  {
    mesh_id_accel = object_id;

    auto iter = context_id->impostors.find(object_id);
    if (iter == context_id->impostors.end())
    {
      meta_alloc_id_accel = -1;
      blas_accel = 0;
      return;
    }

    meta_alloc_id_accel = MeshMetaAllocator::decode(iter->second.metaAllocId);
    blas_accel = iter->second.blas.getGPUAddress();
  }

  if (!blas_accel)
    return;

  auto &data = context_id->impostorInstanceData[thread_ix];

  uint32_t color777 = pack_color8_to_color777(color);

  auto &instance = *(HWInstance *)context_id->impostorInstances[thread_ix].push_back_uninitialized();
  instance.transform = transform;
  instance.instanceID = meta_alloc_id_accel;
  instance.instanceMask = bvhGroupImpostor;
  instance.instanceContributionToHitGroupIndex = color777 | ICTHGI_USE_AS_777_ISTANCE_COLOR;
  instance.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
  instance.blasGpuAddress = blas_accel;

  if (!use_icthgi_for_per_instance_data)
    *(UPoint2 *)data.push_back_uninitialized() = UPoint2{color};
}

BVH_INLINE bool is_mesh_added(ContextId context_id, uint64_t object_id)
{
  return context_id->objects.find(object_id) != context_id->objects.end() ||
         context_id->impostors.find(object_id) != context_id->impostors.end();
}

BVH_INLINE Object *get_object(ContextId context_id, uint64_t object_id)
{
  auto iter = context_id->objects.find(object_id);
  return iter == context_id->objects.end() ? nullptr : &iter->second;
}
} // namespace bvh