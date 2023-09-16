//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/unique_ptr.h>

struct EventFenceDeleter
{
  void operator()(D3dEventQuery *ptr) { ptr ? d3d::release_event_query(ptr) : (void)0; }
};
using EventQueryHolder = eastl::unique_ptr<D3dEventQuery, EventFenceDeleter>;
