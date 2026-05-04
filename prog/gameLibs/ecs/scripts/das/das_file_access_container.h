// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "das_loading_container.h"
#include <dasModules/dasFsFileAccess.h>

namespace bind_dascript
{
template <int ID = 0>
struct DagFileAccessContainer
{
  LoadingContainer<DagFileAccess, ID> access;
  das::string dasProject;
  HotReload hotReloadMode = HotReload::DISABLED;

  void clear() { access.clear(); }

  void thisThreadDone() { access.thisThreadDone(); }

  DagFileAccess *get()
  {
    auto &value = access.getTLSValue();
    if (!value && !dasProject.empty())
      value = eastl::make_unique<DagFileAccess>(dasProject.c_str(), hotReloadMode);
    return value.get();
  }
  void freeSourceData()
  {
    if (auto &value = access.getTLSValue(); value)
      value->freeSourceData();
    for (auto &fa : access.cache)
      fa->freeSourceData();
  }
};
} // namespace bind_dascript