//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_query.h>
#include <EASTL/unique_ptr.h>

struct EventFenceDeleter
{
  void operator()(D3dEventQuery *ptr) { ptr ? d3d::release_event_query(ptr) : (void)0; }
};
using EventQueryHolder = eastl::unique_ptr<D3dEventQuery, EventFenceDeleter>;
