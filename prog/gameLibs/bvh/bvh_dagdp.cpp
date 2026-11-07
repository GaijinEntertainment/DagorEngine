// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <rendInst/rendInstExtra.h>
#include <EASTL/unordered_map.h>
#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"

namespace bvh::dagdp
{
struct BVHInstanceMapper : ::BVHInstanceMapper
{
  ContextId contextId;
  Sbuffer *instanceCounter;
  Sbuffer *instances;
  BVHInstanceMapper() = default;
  BVHInstanceMapper(ContextId context_id) : contextId(context_id), instanceCounter(nullptr), instances(nullptr) {};
  void mapRendinst(const RenderableInstanceLodsResource *ri, int lod, int elem, IPoint2 &out_blas, uint32_t &out_meta) const override
  {
    Context::BvhObjectReadLock objectsGuard(contextId->objectsLock);
    get_blas_mapping(contextId, make_relem_mesh_id(ri->getBvhId(), lod, elem), out_blas, out_meta);
  }

  void submitBuffers(Sbuffer *instance_counter, Sbuffer *instances_) override
  {
    this->instanceCounter = instance_counter;
    this->instances = instances_;
  }
  void requestStaticBLAS(const RenderableInstanceLodsResource *ri) override
  {
    if (ri->isUsedAsDagdp())
      return;
    ri->markForUseInDagdp();

    if (!ri->hasTreeOrFlag()) // These are processed properly anyway
      return;

    contextId->pendingStaticBLASRequestActions.insert(ri);
  }

  bool isInStaticBLASQueue(const RenderableInstanceLodsResource *ri) override
  {
    return contextId->pendingStaticBLASRequestActions.count(ri) > 0;
  }
};

eastl::unordered_map<ContextId, BVHInstanceMapper> mappers;

::BVHInstanceMapper *get_mapper(ContextId context_id)
{
  if (!context_id->hasAny(Features::Dagdp))
    return nullptr;
  G_ASSERT(mappers.count(context_id) == 1);
  return &mappers.at(context_id);
}

void init(ContextId context_id)
{
  if (!context_id->hasAny(Features::Dagdp))
    return;
  G_ASSERT(mappers.count(context_id) == 0);
  mappers[context_id] = BVHInstanceMapper(context_id);
}

void teardown(ContextId context_id)
{
  if (!context_id->hasAny(Features::Dagdp))
    return;
  G_ASSERT(mappers.count(context_id) == 1);
  mappers.erase(context_id);
}

::BVHInstanceMapper::InstanceBuffers get_buffers(ContextId context_id)
{
  auto mapper = static_cast<BVHInstanceMapper *>(get_mapper(context_id));
  if (!mapper)
    return {};
  ::BVHInstanceMapper::InstanceBuffers result = {mapper->instanceCounter, mapper->instances};
  mapper->instanceCounter = mapper->instances = nullptr; // These are only valid for one frame, so clear them!
  return result;
}

} // namespace bvh::dagdp