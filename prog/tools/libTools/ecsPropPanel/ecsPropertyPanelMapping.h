// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityComponent.h>
#include <daECS/core/entityId.h>
#include <util/dag_string.h>

#include <EASTL/vector_map.h>

namespace ecs
{
class Array;
class Object;
} // namespace ecs

struct ECSEditorPropertyInfo
{
  enum class PropertyType
  {
    Component,           // entity component, object member or array element
    ComponentArrayGroup, // ecs::Array
    ComponentArrayExtensibleContainer,
    ComponentArrayAddButton,
    ComponentObjectGroup, // ecs::Object
    ComponentObjectExtensibleContainer,
    ComponentObjectAddButton,
  };

  bool hasParent() const { return parentControlId != 0; }

  String componentName;
  int parentControlId;
  PropertyType propertyType;
  int arrayIndex;
};

class ECSPropertyPanelMapping
{
public:
  void clear() { controlMap.clear(); }

  void addControl(int control_id, const char *name, int parent_control_id,
    ECSEditorPropertyInfo::PropertyType property_type = ECSEditorPropertyInfo::PropertyType::Component, int array_index = -1);
  const ECSEditorPropertyInfo *getControl(int control_id) const;

  ecs::EntityComponentRef getComponentRef(ecs::EntityId eid, int control_id, const char *&root_component_name);
  ecs::Array *getComponentArray(ecs::EntityId eid, int control_id, const char *&root_component_name);
  ecs::Object *getComponentObject(ecs::EntityId eid, int control_id, const char *&root_component_name);

private:
  ecs::EntityComponentRef getComponentRefInternal(ecs::EntityId eid, int control_id, const char *&root_component_name);

  eastl::vector_map<int, ECSEditorPropertyInfo> controlMap;
};
