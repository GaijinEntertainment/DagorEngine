// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_eventQueryHolder.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>
#include "bvh_context.h"
#include <bvh/bvh_connection.h>
#include <EASTL/unordered_set.h>

namespace bvh
{

ComputeShaderElement &get_hw_instance_copy_shader();

struct BVHConnection : public ::BVHConnection
{
  eastl::unordered_set<ContextId> contexts;

  UniqueBuf counter;
  UniqueBuf counterReadback;
  UniqueBuf instances;
  EventQueryHolder counterQuery;

  UniqueBuf metainfoMappings;

  bool queryInProgress = false;

  int maxCount = 2;

  String name;

  BVHConnection(const char *name) : name(name) {}

  bool isReady() const override { return !contexts.empty(); }
  bool prepare() override
  {
    // Read back the number of instances. The instance buffer is resized to fit the maximum number of instances.
    // This will only help the next frame, but in a few frames the buffer will be resized to fit the actual number of instances.

    if (queryInProgress && d3d::get_event_query_status(counterQuery.get(), false))
    {
      int instanceCount = 0;
      if (auto bufferData = lock_sbuffer<const uint32_t>(counterReadback.getBuf(), 0, 0, VBLOCK_READONLY))
        instanceCount = bufferData[0];

      maxCount = max(maxCount, instanceCount);

      if (instances && maxCount > instances->getNumElements())
        instances.close();

      queryInProgress = false;
    }

    if (!instances)
    {
      instances = dag::buffers::create_ua_sr_structured(sizeof(HWInstance), maxCount, String(0, "bvh_%s_instances", name.data()));
      HANDLE_LOST_DEVICE_STATE(instances, false);
    }

    if (!counter)
    {
      counter = dag::buffers::create_ua_sr_byte_address(1, String(0, "bvh_%s_instance_counter", name.data()));
      HANDLE_LOST_DEVICE_STATE(counter, false);
      counterReadback = dag::buffers::create_staging(4, String(0, "bvh_%s_instance_counter_readback", name.data()));
      HANDLE_LOST_DEVICE_STATE(counterReadback, false);
      counterQuery.reset(d3d::create_event_query());
      HANDLE_LOST_DEVICE_STATE(counterQuery, false);
    }

    // Clear both buffers to zero. For the instance, this makes sure that for the range where there are
    // no instances, the BVH will get zero matrices and BLAS pointers for the TLAS. That is easy to
    // optimize out during the TLAS build.

    uint32_t zero[4] = {0, 0, 0, 0};
    d3d::clear_rwbufi(counter.getBuf(), zero);
    d3d::clear_rwbufi(instances.getBuf(), zero);

    return true;
  }
  void done() override
  {
    if (!queryInProgress && counter && counterReadback && counterQuery)
    {
      counter->copyTo(counterReadback.getBuf());

      if (counterReadback->lock(0, 0, (void **)nullptr, VBLOCK_READONLY))
        counterReadback->unlock();

      d3d::issue_event_query(counterQuery.get());

      queryInProgress = true;
    }
  }
  const UniqueBuf &getInstanceCounter() override { return counter; }
  const UniqueBuf &getInstancesBuffer() override { return instances; }
  const UniqueBuf &getMappingsBuffer() override { return metainfoMappings; }

  void init() {}

  virtual void teardown()
  {
    counter.close();
    counterReadback.close();
    counterQuery.reset();
    instances.close();
    queryInProgress = false;
    maxCount = 1;
  }
};

} // namespace bvh