// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsResources.h>
#include <util/dag_compilerDefs.h>

LightsResourcesManager::LightsResourcesManager(const eastl::string &name_suffix) : nameSuffix(name_suffix) {}

const char *LightsResourcesManager::getResName(const char *name) const
{
  if (DAGOR_LIKELY(nameSuffix.empty()))
    return name;
  resNameTmp = name;
  resNameTmp += nameSuffix;
  return resNameTmp.c_str();
}
