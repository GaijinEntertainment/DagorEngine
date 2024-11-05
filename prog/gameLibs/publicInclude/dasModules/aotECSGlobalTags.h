//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <generic/dag_tab.h>

Tab<const char *> ecs_get_global_tags_context();

namespace bind_dascript
{
inline bool ecs_has_tag(const char *tag)
{
  if (!tag)
    return false;
  const Tab<const char *> tags = ecs_get_global_tags_context();
  auto pred = [tag](const char *str) -> bool { return strcmp(str, tag) == 0; };
  return eastl::find_if(tags.begin(), tags.end(), pred) != tags.end();
}
} // namespace bind_dascript
