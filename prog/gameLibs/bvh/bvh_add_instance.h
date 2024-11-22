// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <math/dag_dxmath.h>

#define BVH_INLINE __forceinline

namespace bvh
{
BVH_INLINE bool VECTORCALL need_winding_flip(const Mesh &mesh, mat43f transform)
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

  bool flipWinding = false;
  if (mesh.enableCulling)
  {
    XMMATRIX xmtransform = XMLoadFloat3x4(reinterpret_cast<const XMFLOAT3X4 *>(&transform));
    float determinant = XMVectorGetX(XMMatrixDeterminant(xmtransform));
    flipWinding = determinant < 0;
  }

  return flipWinding;
}

BVH_INLINE void VECTORCALL add_instance(ContextId context_id, Context::InstanceMap &instanceMap, uint64_t mesh_id, mat43f transform,
  const Point4 *per_instance_data, bool no_shadow, MeshSkinningInfo *skinning_info, MeshHeliRotorInfo *heli_rotor_info,
  TreeInfo *tree_info, LeavesInfo *leaves_info, FlagInfo *flag_info, DeformedInfo *deformed_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  auto meshIter = context_id->meshes.find(mesh_id);

  if (meshIter == context_id->meshes.end())
    return;

  bool isAnimated = meshIter->second.vertexProcessor && !meshIter->second.vertexProcessor->isOneTimeOnly();
  G_ASSERTF_RETURN(!isAnimated || tree_info || skinning_info || heli_rotor_info || leaves_info || flag_info || deformed_info, ,
    "Animated models should have tree, leaves, skinning, heli rotor, flag or deformed info");

  auto &instance = instanceMap.emplace_back();
  memset(&instance, 0, sizeof(instance));
  instance.transform = transform;
  instance.meshId = mesh_id;
  instance.uniqueIsRecycled = false;
  instance.noShadow = no_shadow;
  if (per_instance_data)
    instance.perInstanceData = *per_instance_data;
  if (skinning_info)
  {
    instance.invWorldTm = skinning_info->invWorldTm;
    instance.setTransformsFn = skinning_info->setTransformsFn;
    instance.uniqueTransformBuffer = skinning_info->skinningBuffer;
    instance.uniqueBlas = skinning_info->skinningBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (heli_rotor_info)
  {
    instance.invWorldTm = heli_rotor_info->invWorldTm;
    instance.getHeliParamsFn = heli_rotor_info->getParamsFn;
    instance.uniqueTransformBuffer = heli_rotor_info->transformedBuffer;
    instance.uniqueBlas = heli_rotor_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (tree_info)
  {
    instance.invWorldTm = tree_info->invWorldTm;
    instance.uniqueTransformBuffer = tree_info->transformedBuffer;
    instance.uniqueBlas = tree_info->transformedBlas;
    instance.tree = tree_info->data;
    instance.metaAllocId = meta_alloc_id;
    instance.hasInstanceColor = true;
    instance.uniqueIsRecycled = tree_info->recycled;

    G_ASSERT(!instance.perInstanceData.has_value());
    instance.perInstanceData = Point4::rgba(Color4(instance.tree.color));
  }
  else if (leaves_info)
  {
    instance.invWorldTm = leaves_info->invWorldTm;
    instance.uniqueTransformBuffer = leaves_info->transformedBuffer;
    instance.uniqueBlas = leaves_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
    instance.uniqueIsRecycled = leaves_info->recycled;
  }
  else if (flag_info)
  {
    instance.invWorldTm = flag_info->invWorldTm;
    instance.uniqueTransformBuffer = flag_info->transformedBuffer;
    instance.uniqueBlas = flag_info->transformedBlas;
    instance.flag = flag_info->data;
    instance.metaAllocId = meta_alloc_id;
  }
  else if (deformed_info)
  {
    instance.invWorldTm = deformed_info->invWorldTm;
    instance.getDeformParamsFn = deformed_info->getParamsFn;
    instance.uniqueTransformBuffer = deformed_info->transformedBuffer;
    instance.uniqueBlas = deformed_info->transformedBlas;
    instance.metaAllocId = meta_alloc_id;
  }
  else
    instance.metaAllocId = -1;
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], mesh_id, transform, nullptr, false, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, -1);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, TreeInfo *tree_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], mesh_id, transform, nullptr, false, nullptr, nullptr, tree_info,
    nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riGen_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, LeavesInfo *leaves_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riGenInstances[thread_ix], mesh_id, transform, nullptr, false, nullptr, nullptr, nullptr,
    leaves_info, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(mat43f transform, vec4f pX, vec4f pY, vec4f pZ, ContextId context_id, uint64_t mesh_id,
  int thread_ix, uint32_t hashVal)
{
  auto meshIter = context_id->meshes.find(mesh_id);

  if (meshIter == context_id->meshes.end())
    return;

  auto &mesh = meshIter->second;

  if (!mesh.blas)
    return;

  if (mesh.vertexProcessor && !mesh.vertexProcessor->isOneTimeOnly())
    return;

  bool flipWinding = need_winding_flip(mesh, transform);

  auto &perInstanceData = context_id->riExtraInstanceData[thread_ix];

  auto &desc = *(HWInstance *)context_id->riExtraInstances[thread_ix].push_back_uninitialized();
  desc.transform.row0 = v_sub(transform.row0, pX);
  desc.transform.row1 = v_sub(transform.row1, pY);
  desc.transform.row2 = v_sub(transform.row2, pZ);
  desc.instanceID = mesh.metaAllocId;
  desc.instanceMask = bvhGroupRiExtra;
  desc.instanceContributionToHitGroupIndex = hashVal ? perInstanceData.size() : 0xFFFFFFU;
  desc.flags = mesh.enableCulling ? (flipWinding ? RaytraceGeometryInstanceDescription::TRIANGLE_CULL_FLIP_WINDING
                                                 : RaytraceGeometryInstanceDescription::NONE)
                                  : RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
  desc.blasGpuAddress = mesh.blas.getGPUAddress();

  if (hashVal)
  {
    auto packedHash = perInstanceData.push_back_uninitialized();
    memcpy(packedHash, &hashVal, sizeof(hashVal));
  }
}

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, TreeInfo *tree_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraTreeInstances[thread_ix], mesh_id, transform, nullptr, false, nullptr, nullptr,
    tree_info, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_riExtra_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, FlagInfo *flag_info,
  MeshMetaAllocator::AllocId meta_alloc_id, int thread_ix)
{
  add_instance(context_id, context_id->riExtraTreeInstances[thread_ix], mesh_id, transform, nullptr, false, nullptr, nullptr, nullptr,
    nullptr, flag_info, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, dynrend::ContextId dynrend_context_id, uint64_t mesh_id,
  mat43f transform, const Point4 *per_instance_data, bool no_shadow)
{
  add_instance(context_id, context_id->dynrendInstances[dynrend_context_id], mesh_id, transform, per_instance_data, no_shadow, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, -1);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, dynrend::ContextId dynrend_context_id, uint64_t mesh_id,
  mat43f transform, const Point4 *per_instance_data, bool no_shadow, MeshSkinningInfo &skinning_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, context_id->dynrendInstances[dynrend_context_id], mesh_id, transform, per_instance_data, no_shadow,
    &skinning_info, nullptr, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, dynrend::ContextId dynrend_context_id, uint64_t mesh_id,
  mat43f transform, const Point4 *per_instance_data, bool no_shadow, MeshHeliRotorInfo &heli_rotor_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, context_id->dynrendInstances[dynrend_context_id], mesh_id, transform, per_instance_data, no_shadow, nullptr,
    &heli_rotor_info, nullptr, nullptr, nullptr, nullptr, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_dynrend_instance(ContextId context_id, dynrend::ContextId dynrend_context_id, uint64_t mesh_id,
  mat43f transform, const Point4 *per_instance_data, bool no_shadow, DeformedInfo &deformed_info,
  MeshMetaAllocator::AllocId meta_alloc_id)
{
  add_instance(context_id, context_id->dynrendInstances[dynrend_context_id], mesh_id, transform, per_instance_data, no_shadow, nullptr,
    nullptr, nullptr, nullptr, nullptr, &deformed_info, meta_alloc_id);
}

BVH_INLINE void VECTORCALL add_impostor_instance(ContextId context_id, uint64_t mesh_id, mat43f transform, uint32_t color,
  int thread_ix, int &meta_alloc_id_accel, uint64_t blas_accel, uint64_t mesh_id_accel)
{
  if (mesh_id != mesh_id_accel)
  {
    mesh_id_accel = mesh_id;

    auto iter = context_id->impostors.find(mesh_id);
    if (iter == context_id->impostors.end())
    {
      meta_alloc_id_accel = -1;
      blas_accel = 0;
      return;
    }

    meta_alloc_id_accel = iter->second.metaAllocId;
    blas_accel = iter->second.blas.getGPUAddress();
  }

  if (!blas_accel)
    return;

  auto &data = context_id->impostorInstanceData[thread_ix];

  auto &instance = *(HWInstance *)context_id->impostorInstances[thread_ix].push_back_uninitialized();
  instance.transform = transform;
  instance.instanceID = meta_alloc_id_accel;
  instance.instanceMask = bvhGroupImpostor;
  instance.instanceContributionToHitGroupIndex = data.size();
  instance.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
  instance.blasGpuAddress = blas_accel;

  *(Point4 *)data.push_back_uninitialized() = Point4::rgba(Color4(color));
}

BVH_INLINE bool is_mesh_added(ContextId context_id, uint64_t mesh_id)
{
  return context_id->meshes.find(mesh_id) != context_id->meshes.end() ||
         context_id->impostors.find(mesh_id) != context_id->impostors.end();
}

BVH_INLINE Mesh *get_mesh(ContextId context_id, uint64_t mesh_id)
{
  auto iter = context_id->meshes.find(mesh_id);
  return iter == context_id->meshes.end() ? nullptr : &iter->second;
}
} // namespace bvh