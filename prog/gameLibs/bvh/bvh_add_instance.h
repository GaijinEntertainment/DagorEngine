// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "util/dag_compilerDefs.h"
#include <bvh/bvh.h>
#include <math/dag_dxmath.h>

#define BVH_INLINE __forceinline

namespace bvh
{

namespace parallel_instance_processing
{
void start_frame(ContextId context_id);
void before_job_start(ContextId context_id);
void after_job_end(ContextId context_id);
} // namespace parallel_instance_processing

BVH_INLINE bool VECTORCALL need_winding_flip(mat43f transform)
{
  /* So this need explanation. From the DXR specification:
   * Since these winding direction rules are defined in object space, they are unaffected by instance
   * transforms. For example, an instance transform matrix with negative determinant (e.g. mirroring
   * some geometry) does not change the facing of the triangles within the instance. Per-geometry
   * transforms, by contrast, (defined in D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC), get combined with
   * the associated vertex data in object space, so a negative determinant matrix there does flip triangle
   * winding.
   *
   * BUT. Some of our models are modelled in a way that the actual triangles are flipped, and expected to be
   * flipped during rasterization by the model matrix, which has 1 or 3 axes flipped. So as the instance
   * transform is not flipping the winding, we need to do it here to match the rasterized output.
   */

  XMMATRIX xmtransform = XMLoadFloat3x4(reinterpret_cast<const XMFLOAT3X4 *>(&transform));
  float determinant = XMVectorGetX(XMMatrixDeterminant(xmtransform));
  return determinant < 0;
}

BVH_INLINE void VECTORCALL add_instance(ContextId context_id, Context::InstanceMap &instanceMap, uint64_t object_id, mat43f transform,
  const PerInstanceData *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode,
  MeshSkinningInfo *skinning_info, MeshHeliRotorInfo *heli_rotor_info, TreeInfo *tree_info, LeavesInfo *leaves_info,
  FlagInfo *flag_info, DeformedInfo *deformed_info, SplineGenInfo *splinegen_info, MeshMetaAllocator::AllocId meta_alloc_id)
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
  instance.animIndex = -1;
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
    instance.uniqueIsStationary = tree_info->stationary;
    instance.animIndex = tree_info->animIndex;

    G_ASSERT(!instance.perInstanceData.has_value());
    instance.perInstanceData = PerInstanceData{instance.tree.color.u, 0, 0, 0};
  }
  else if (leaves_info)
  {
    instance.invWorldTm = leaves_info->invWorldTm;
    instance.uniqueTransformedBuffer = leaves_info->transformedBuffer;
    instance.uniqueBlas = leaves_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
    instance.uniqueIsRecycled = leaves_info->recycled;
    instance.uniqueIsStationary = leaves_info->stationary;
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
  else if (splinegen_info)
  {
    instance.getSplineDataFn = splinegen_info->getSplineDataFn;
    instance.uniqueTransformedBuffer = splinegen_info->transformedBuffer;
    instance.uniqueBlas = splinegen_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else
    instance.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
}

BVH_INLINE void VECTORCALL add_instance(Context::InstanceMap &instanceMap, uint64_t object_id, mat43f transform,
  const PerInstanceData *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode)
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
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    MeshMetaAllocator::INVALID_ALLOC_ID);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t object_id, mat43f transform, TreeInfo *tree_info,
  bool is_stationary, MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], object_id, transform, nullptr, false,
    is_stationary ? Context::Instance::AnimationUpdateMode::FORCE_OFF : Context::Instance::AnimationUpdateMode::FORCE_ON, nullptr,
    nullptr, tree_info, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t object_id, mat43f transform, LeavesInfo *leaves_info,
  bool is_stationary, MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], object_id, transform, nullptr, false,
    is_stationary ? Context::Instance::AnimationUpdateMode::FORCE_OFF : Context::Instance::AnimationUpdateMode::FORCE_ON, nullptr,
    nullptr, nullptr, leaves_info, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(mat43f transform, vec4f pX, vec4f pY, vec4f pZ, ContextId context_id,
  uint64_t object_id, int thread_ix, uint32_t hashVal, bool backFaceCulling)
{
  auto objectIter = context_id->objects.find(object_id);
  if (objectIter == context_id->objects.end())
    return;

  auto &object = objectIter->second;

  if (DAGOR_UNLIKELY(!object.blas || object.isAnimated))
    return;

  auto &perInstanceData = context_id->riExtraInstanceData[thread_ix];

  HWInstance desc;
  desc.transform.row0 = v_sub(transform.row0, pX);
  desc.transform.row1 = v_sub(transform.row1, pY);
  desc.transform.row2 = v_sub(transform.row2, pZ);
  desc.instanceID = MeshMetaAllocator::decode(object.metaAllocId);
  desc.instanceMask = bvhGroupRi;
  desc.instanceContributionToHitGroupIndex = 0;
  desc.flags = !backFaceCulling ? RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE : 0;
  desc.blasGpuAddress = object.blas.getGPUAddress();

  context_id->riExtraInstances[thread_ix].push_back(convert_instance(desc));

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

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t object_id, mat43f transform, TreeInfo *tree_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraTreeInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, tree_info, nullptr, nullptr, nullptr, nullptr,
    meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t object_id, mat43f transform, FlagInfo *flag_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraFlagInstances[thread_ix], object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, flag_info, nullptr, nullptr,
    meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(Context::InstanceMap &instances, uint64_t object_id, mat43f transform,
  const PerInstanceData *per_instance_data, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode)
{
  add_instance(instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const PerInstanceData *per_instance_data, bool no_shadow,
  Context::Instance::AnimationUpdateMode animation_update_mode, MeshSkinningInfo &skinning_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, &skinning_info,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const PerInstanceData *per_instance_data, bool no_shadow,
  Context::Instance::AnimationUpdateMode animation_update_mode, MeshHeliRotorInfo &heli_rotor_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, nullptr,
    &heli_rotor_info, nullptr, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, const PerInstanceData *per_instance_data, bool no_shadow,
  Context::Instance::AnimationUpdateMode animation_update_mode, DeformedInfo &deformed_info, MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, per_instance_data, no_shadow, animation_update_mode, nullptr, nullptr,
    nullptr, nullptr, nullptr, &deformed_info, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_splinegen_instance(ContextId context_id, Context::InstanceMap &instances, uint64_t object_id,
  mat43f transform, bool no_shadow, Context::Instance::AnimationUpdateMode animation_update_mode, SplineGenInfo &splinegen_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, instances, object_id, transform, nullptr, no_shadow, animation_update_mode, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, &splinegen_info, meta_alloc_id);
}

BVH_INLINE bool VECTORCALL add_impostor_instance(ContextId context_id, uint64_t object_id, mat43f transform, uint32_t color,
  int thread_ix, int &meta_alloc_id_accel, uint64_t &blas_accel, uint64_t &mesh_id_accel)
{
  if (object_id != mesh_id_accel)
  {
    mesh_id_accel = object_id;

    auto iter = context_id->impostors.find(object_id);
    if (iter == context_id->impostors.end())
    {
      meta_alloc_id_accel = -1;
      blas_accel = 0;
      return false;
    }

    meta_alloc_id_accel = MeshMetaAllocator::decode(iter->second.metaAllocId);
    blas_accel = iter->second.blas.getGPUAddress();
  }

  if (!blas_accel)
    return false;

  auto &data = context_id->impostorInstanceData[thread_ix];

  HWInstance instance;
  instance.transform = transform;
  instance.instanceID = meta_alloc_id_accel;
  instance.instanceMask = bvhGroupImpostor;
  instance.instanceContributionToHitGroupIndex = 0;
  instance.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
  instance.blasGpuAddress = blas_accel;
  context_id->impostorInstances[thread_ix].push_back(convert_instance(instance));

  *(PerInstanceData *)data.push_back_uninitialized() = PerInstanceData{color};

  return true;
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