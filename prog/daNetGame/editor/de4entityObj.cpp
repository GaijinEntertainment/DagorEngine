// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys/physEvents.h"

#include <daEditorE/editorCommon/entityObj.h>

inline void makeSafeEntityComponentsRestoredFromUndo(EntCreateData *removedEntData)
{
  // always disable cameras to avoid conflicts with free_cam
  const ecs::component_type_t typeBool = ECS_HASH("bool").hash;
  const ecs::component_t nameCamActive = ECS_HASH("camera__active").hash;
  if (g_entity_mgr->getTemplateDB().getComponentType(nameCamActive) == typeBool)
  {
    if (removedEntData)
      for (auto &attr : removedEntData->attrs)
        if (attr.name == nameCamActive)
          attr.second = false;
  }
}

#include <daEditorE/editorCommon/entityObj.inc.cpp>
