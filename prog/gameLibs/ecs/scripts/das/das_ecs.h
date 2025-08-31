// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/runtime_matrices.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/core/entitySystem.h>
#include <dag/dag_vectorSet.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasScriptsLoader.h>
#include <util/dag_simpleString.h>
#include "das_es.h"


MAKE_TYPE_FACTORY(Event, ecs::Event)

struct DasEcsStatistics
{
  uint32_t systemsCount = 0u;
  uint32_t aotSystemsCount = 0u;
  uint32_t loadTimeMs = 0u;
  int64_t memoryUsage = 0;
};

namespace bind_dascript
{


class ECS final : public das::Module
{
public:
  ECS();
  virtual void addPrerequisits(das::ModuleLibrary &ml) const override;
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override;
  void addES(das::ModuleLibrary &ml);
  void addEntityRead(das::ModuleLibrary &ml);
  void addEntityWrite(das::ModuleLibrary &ml);
  void addEntityWriteOptional(das::ModuleLibrary &ml);
  void addEntityRW(das::ModuleLibrary &ml);
  void addContainerAnnotations(das::ModuleLibrary &ml);
  void addChildComponent(das::ModuleLibrary &ml);
  void addArray(das::ModuleLibrary &ml);
  void addObjectRead(das::ModuleLibrary &ml);
  void addObjectWrite(das::ModuleLibrary &ml);
  void addObjectRW(das::ModuleLibrary &ml);
  void addList(das::ModuleLibrary &ml);
  void addListFn(das::ModuleLibrary &ml);
  void addInitializers(das::ModuleLibrary &lib);
  void addCompMap(das::ModuleLibrary &lib);
  void addEvents(das::ModuleLibrary &lib);
  void addTemplates(das::ModuleLibrary &);
};
} // namespace bind_dascript
