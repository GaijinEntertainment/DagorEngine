// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <shaders/dag_shaderMesh.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_eventQueryHolder.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"


namespace bvh::splinegen
{

static eastl::vector<eastl::tuple<uint64_t, SplineGenInfo, MeshMetaAllocator::AllocId>> splinegenBVHInstances;

static void add_splinegen_obj(ContextId context_id, uint64_t mesh_id, MeshInfo &meshInfo)
{
  ObjectInfo obj;
  meshInfo.posMul = Point4::ONE;
  meshInfo.posAdd = Point4::ZERO;
  meshInfo.vertexProcessor = &ProcessorInstances::getSplineGenVertexProcessor();
  obj.meshes.push_back(meshInfo);
  obj.isAnimated = true;
  obj.tag = "splinegen";
  add_object(context_id, mesh_id, obj);
  context_id->splineGenObjectIds.push_back(mesh_id);
}

static void add_splinegen_instance_to_list(ContextId context_id, uint64_t bvh_object_id, Sbuffer *vertex_buffer,
  uint32_t splinegen_instance_id, uint32_t instance_vertex_count)
{
  auto &data = context_id->uniqueSplinegenBuffers[bvh_object_id];

  SplineGenInfo splinegenInfo;
  splinegenInfo.transformedBuffer = &data.buffer;
  splinegenInfo.transformedBlas = &data.blas;
  splinegenInfo.getSplineDataFn = [vertex_buffer, splinegen_instance_id, instance_vertex_count](uint32_t &start_vertex) {
    start_vertex = splinegen_instance_id * instance_vertex_count;
    return vertex_buffer;
  };

  if (data.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
    data.metaAllocId = context_id->allocateMetaRegion(1);

  splinegenBVHInstances.push_back(eastl::make_tuple(bvh_object_id, splinegenInfo, data.metaAllocId));
}

void add_meshes(ContextId context_id, Sbuffer *vertex_buffer, eastl::vector<eastl::pair<uint32_t, MeshInfo>> &meshes,
  uint32_t instance_vertex_count, uint32_t &bvh_id)
{
  if (!context_id->has(Features::Splinegen))
    return;

  if (bvh_id == 0)
    bvh_id = bvh_id_gen.add_fetch(1);

  for (auto [instanceId, meshInfo] : meshes)
  {
    const auto elemId = make_dyn_elem_id(instance_vertex_count, instanceId);
    const uint64_t objectId = make_relem_mesh_id(bvh_id, instanceId % 16, elemId);

    splinegen::add_splinegen_instance_to_list(context_id, objectId, vertex_buffer, instanceId, instance_vertex_count);

    // only add object if it does not exist yet
    if (context_id->objects.find(objectId) != context_id->objects.end())
      continue;

    splinegen::add_splinegen_obj(context_id, objectId, meshInfo);
  }
}

void update_instances(ContextId context_id, const Point3 &view_pos)
{
  if (!context_id->has(Features::Splinegen))
    return;

  context_id->splineGenInstances.clear();

  // Splinegen instances are in world-space, raytracing expects them to
  // be in view-space so their "instance" TM position is -view_pos
  mat43f instanceTransform;
  instanceTransform.row0 = v_make_vec4f(1, 0, 0, -view_pos.x);
  instanceTransform.row1 = v_make_vec4f(0, 1, 0, -view_pos.y);
  instanceTransform.row2 = v_make_vec4f(0, 0, 1, -view_pos.z);

  for (auto &[objectId, info, alloc] : splinegenBVHInstances)
    add_splinegen_instance(context_id, context_id->splineGenInstances, objectId, instanceTransform, false,
      Context::Instance::AnimationUpdateMode::FORCE_ON, info, alloc);

  splinegenBVHInstances.clear();
}

void teardown(ContextId context_id)
{
  if (!context_id->has(Features::Splinegen))
    return;

  for (auto [objectId, info, alloc] : splinegenBVHInstances)
  {
    context_id->freeMetaRegion(alloc);
    remove_object(context_id, objectId);
  }

  splinegenBVHInstances.clear();

  for (auto &objectId : context_id->splineGenObjectIds)
    remove_object(context_id, objectId);
  context_id->splineGenObjectIds = {};

  for (auto &buffer : context_id->uniqueSplinegenBuffers)
    context_id->freeMetaRegion(buffer.second.metaAllocId);
  context_id->uniqueSplinegenBuffers.clear();
}

void on_unload_scene(ContextId context_id) { bvh::splinegen::teardown(context_id); }

} // namespace bvh::splinegen