// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <gpuObjects/gpuObjects.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_generic_connection.h"
#include "bvh_add_instance.h"

namespace bvh::gobj
{

struct BVHConnection : public bvh::BVHConnection
{
  struct GpuObjectsBvhMapping
  {
    IPoint2 blas;
    uint32_t metaIndex;
    uint32_t padding;
  };

  // From gpuObjects.h
  static constexpr int maxLodCount = 4;

  BVHConnection(const char *name) : bvh::BVHConnection(name) {}

  bool prepare() override
  {
    if (!bvh::BVHConnection::prepare())
      return false;

    for (auto &mapping : metainfoMappingsById)
      mapping.second.dirty = true;

    return true;
  }

  bool translateObjectId(int id, D3DRESID &buffer_id) override
  {
    buffer_id = BAD_D3DRESID;

    auto riRes = rendinst::getRIGenExtraRes(id);
    if (riRes->getBvhId() == 0)
      return false;

    auto &mapping = metainfoMappingsById[id];
    if (!mapping.mapping)
    {
      String bname(64, "bvh_gpuobject_mapping_%d", id);
      mapping.mapping = dag::buffers::create_persistent_sr_structured(sizeof(GpuObjectsBvhMapping), maxLodCount, bname);

      HANDLE_LOST_DEVICE_STATE(mapping.mapping, false);
    }

    if (mapping.dirty)
    {
      GpuObjectsBvhMapping mappings[maxLodCount];
      for (auto [lodIx, mapping] : enumerate(mappings))
      {
        auto meshId = make_relem_mesh_id(riRes->getBvhId(), lodIx, 0);
        Mesh *mesh = get_mesh(*contexts.begin(), meshId);

        if (mesh && mesh->blas && mesh->metaAllocId >= 0)
        {
          mapping.blas.x = mesh->blas.getGPUAddress() & 0xFFFFFFFFLLU;
          mapping.blas.y = mesh->blas.getGPUAddress() >> 32;
          mapping.metaIndex = mesh->metaAllocId;
        }
        else
        {
          mapping.blas.x = 0;
          mapping.blas.y = 0;
          mapping.metaIndex = 0;
        }
      }

      mapping.mapping->updateData(0, sizeof(mappings), mappings, 0);
      mapping.dirty = false;
    }

    buffer_id = mapping.mapping.getBufId();
    return true;
  }

  void teardown() override
  {
    metainfoMappingsById.clear();
    bvh::BVHConnection::teardown();
  }

  struct Mapping
  {
    UniqueBuf mapping;
    bool dirty = true;
  };
  eastl::unordered_map<int, Mapping> metainfoMappingsById;

} bvhConnection("gpuobject");

void init() { gpu_objects::GpuObjects::setBVHConnection(&bvhConnection); }

void teardown()
{
  gpu_objects::GpuObjects::setBVHConnection(nullptr);
  bvhConnection.teardown();
  bvhConnection.metainfoMappings.close();
}

void init(ContextId context_id)
{
  if (!context_id->has(Features::GpuObjects))
    return;

  bvhConnection.contexts.insert(context_id);
}

void teardown(ContextId context_id)
{
  if (!context_id->has(Features::GpuObjects))
    return;

  bvhConnection.contexts.erase(context_id);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::GpuObjects))
    return;

  bvhConnection.teardown();
}

void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count)
{
  if (context_id->has(Features::GpuObjects))
  {
    instances = bvhConnection.instances.getBuf();
    instance_count = bvhConnection.counter.getBuf();
  }
  else
  {
    instances = nullptr;
    instance_count = nullptr;
  }
}

void get_memory_statistics(int &meta, int &queries)
{
  meta = queries = 0;
  if (bvhConnection.instances)
    queries = bvhConnection.instances->getElementSize() * bvhConnection.instances->getNumElements();
  if (bvhConnection.metainfoMappings)
    meta = bvhConnection.metainfoMappings->getElementSize() * bvhConnection.metainfoMappings->getNumElements();
}

} // namespace bvh::gobj
