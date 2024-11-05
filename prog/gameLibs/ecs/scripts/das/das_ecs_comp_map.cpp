// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <daECS/core/componentsMap.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{
struct ComponentsMapAnnotation final : das::ManagedStructureAnnotation<ecs::ComponentsMap, false>
{
  ComponentsMapAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ComponentsMap", ml)
  {
    cppName = " ::ecs::ComponentsMap";
  }
};


void ECS::addCompMap(das::ModuleLibrary &lib) { addAnnotation(das::make_smart<ComponentsMapAnnotation>(lib)); }
} // namespace bind_dascript
