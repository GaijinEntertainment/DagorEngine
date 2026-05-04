// Copyright (C) Gaijin Games KFT.  All rights reserved.

// event query implementation
#include <drv/3d/dag_query.h>
#include <util/dag_compilerDefs.h>
#include "globals.h"
#include "device_context.h"
#include "external_resource_pools.h"

using namespace drv3d_vulkan;

d3d::EventQuery *d3d::create_event_query() { return (d3d::EventQuery *)Globals::Res::queries.allocate(); }

void d3d::release_event_query(d3d::EventQuery *fence)
{
  if (DAGOR_UNLIKELY(!fence))
    return;
  Globals::Res::queries.free((AsyncCompletionState *)fence);
}

bool d3d::issue_event_query(d3d::EventQuery *fence)
{
  if (DAGOR_UNLIKELY(!fence))
    return true;
  auto tf = (AsyncCompletionState *)fence;
  // external API may issue query without reading its contents
  // so always reset it
  tf->reset();
  // we use event query to detect readback completion, yet we don't supply queue that should be used
  // so report status with extra latency when async readbacks is active
  size_t extraLatency = 0;
  if (!Frontend::readbacks.getForWrite().empty())
    extraLatency = Frontend::readbacks.getLatency();
  tf->request(Globals::ctx.getCurrentWorkItemId() + extraLatency);
  return true;
}

bool d3d::get_event_query_status(d3d::EventQuery *fence, bool flush)
{
  G_UNUSED(flush);
  if (DAGOR_UNLIKELY(!fence))
    return true;
  auto tf = (AsyncCompletionState *)fence;
  // external API may ask for query status without issuing it
  // must be trated as "completed"
  return tf->isCompletedOrNotRequested();
}
