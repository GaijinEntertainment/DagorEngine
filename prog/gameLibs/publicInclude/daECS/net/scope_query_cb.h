//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>

// called once per update before generating packet in order to query scope status of all objects
namespace net
{
class Connection;
typedef eastl::fixed_function<sizeof(void *) * 2, void(Connection *)> scope_query_cb_t;
} // namespace net
