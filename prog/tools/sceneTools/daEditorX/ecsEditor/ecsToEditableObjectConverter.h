// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/scene/scene.h>
#include <daECS/core/entityId.h>

class RenderableEditableObject;

class IECSToEditableObjectConverter
{
public:
  virtual RenderableEditableObject *getObjectFromSceneId(ecs::Scene::SceneId sid) const = 0;
  virtual RenderableEditableObject *getObjectFromEntityId(ecs::EntityId eid) const = 0;
};
