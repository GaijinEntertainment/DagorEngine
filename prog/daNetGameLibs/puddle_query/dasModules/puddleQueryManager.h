// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <puddle_query/render/puddleQueryManager.h>

namespace bind_dascript
{
inline int puddle_query_start(Point3 pos) { return get_puddle_query_mgr()->queryPoint(pos); }
inline GpuReadbackResultState puddle_query_value(int query_id, float &value)
{
  return get_puddle_query_mgr()->getQueryValue(query_id, value);
}
} // namespace bind_dascript
