//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/functional.h>

// called once per update before generating packet in order to query scope status of all objects
namespace net
{
class Connection;
typedef eastl::function<void(Connection *)> scope_query_cb_t;
} // namespace net
