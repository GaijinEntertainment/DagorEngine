//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>

class LightsResourcesManager final
{
public:
  LightsResourcesManager(const eastl::string &name_suffix);
  const char *getResName(const char *name) const;

private:
  eastl::string nameSuffix;
  mutable eastl::string resNameTmp;
};
