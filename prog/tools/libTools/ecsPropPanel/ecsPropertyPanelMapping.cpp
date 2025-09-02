// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsPropertyPanelMapping.h"

#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>

class NullEntityComponentRef : public ecs::EntityComponentRef
{
public:
  NullEntityComponentRef() { reset(); }
};

void ECSPropertyPanelMapping::addControl(int control_id, const char *name, int parent_control_id,
  ECSEditorPropertyInfo::PropertyType property_type, int array_index)
{
  G_ASSERT(array_index < 0 || property_type == ECSEditorPropertyInfo::PropertyType::ComponentArrayExtensibleContainer);

  ECSEditorPropertyInfo editorPropertyInfo;
  editorPropertyInfo.componentName = name;
  editorPropertyInfo.parentControlId = parent_control_id;
  editorPropertyInfo.propertyType = property_type;
  editorPropertyInfo.arrayIndex = array_index;
  controlMap.insert({control_id, editorPropertyInfo});
}

const ECSEditorPropertyInfo *ECSPropertyPanelMapping::getControl(int control_id) const
{
  auto it = controlMap.find(control_id);
  return it == controlMap.end() ? nullptr : &it->second;
}

ecs::EntityComponentRef ECSPropertyPanelMapping::getComponentRefInternal(ecs::EntityId eid, int control_id,
  const char *&root_component_name)
{
  auto controlId = controlMap.find(control_id);
  if (controlId == controlMap.end())
    return NullEntityComponentRef();

  const ECSEditorPropertyInfo &editorPropertyInfo = controlId->second;

  if (editorPropertyInfo.hasParent())
  {
    G_ASSERT(editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::Component ||
             editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup ||
             editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);

    controlId = controlMap.find(editorPropertyInfo.parentControlId);
    G_ASSERT(controlId != controlMap.end());
    const ECSEditorPropertyInfo &parentPropertyInfo = controlId->second;

    if (parentPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayExtensibleContainer)
    {
      G_ASSERT(parentPropertyInfo.hasParent());
      controlId = controlMap.find(parentPropertyInfo.parentControlId);
      G_ASSERT(controlId != controlMap.end());
      G_ASSERT(controlId->second.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup);
      ecs::EntityComponentRef parentComponentRef =
        getComponentRefInternal(eid, parentPropertyInfo.parentControlId, root_component_name);
      if (parentComponentRef.isNull())
        return parentComponentRef;

      ecs::Array &componentArray = parentComponentRef.getRW<ecs::Array>();
      G_ASSERT(parentPropertyInfo.arrayIndex >= 0);
      return componentArray.at(parentPropertyInfo.arrayIndex).getEntityComponentRef();
    }
    else if (parentPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectExtensibleContainer)
    {
      G_ASSERT(parentPropertyInfo.hasParent());
      controlId = controlMap.find(parentPropertyInfo.parentControlId);
      G_ASSERT(controlId != controlMap.end());
      G_ASSERT(controlId->second.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);
      ecs::EntityComponentRef parentComponentRef =
        getComponentRefInternal(eid, parentPropertyInfo.parentControlId, root_component_name);
      if (parentComponentRef.isNull())
        return parentComponentRef;

      ecs::Object &componentObject = parentComponentRef.getRW<ecs::Object>();
      ecs::Object::const_iterator componentObjectIt = componentObject.find_as(editorPropertyInfo.componentName);
      if (componentObjectIt == componentObject.end())
        return NullEntityComponentRef();
      return componentObjectIt->second.getEntityComponentRef();
    }
    else
    {
      G_ASSERT(false);
      return NullEntityComponentRef();
    }
  }
  else
  {
    G_ASSERT(editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::Component ||
             editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup ||
             editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);

    ecs::component_index_t componentIndex =
      g_entity_mgr->getDataComponents().findComponentId(ECS_HASH_SLOW(editorPropertyInfo.componentName).hash);
    ecs::EntityComponentRef componentRef = g_entity_mgr->getComponentRefRW(eid, componentIndex);
    root_component_name = editorPropertyInfo.componentName;
    return componentRef;
  }
}

ecs::EntityComponentRef ECSPropertyPanelMapping::getComponentRef(ecs::EntityId eid, int control_id, const char *&root_component_name)
{
  auto it = controlMap.find(control_id);
  if (it == controlMap.end())
    return NullEntityComponentRef();

  const ECSEditorPropertyInfo &editorPropertyInfo = it->second;
  G_ASSERT(editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::Component ||
           editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup ||
           editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);

  return getComponentRefInternal(eid, control_id, root_component_name);
}

ecs::Array *ECSPropertyPanelMapping::getComponentArray(ecs::EntityId eid, int control_id, const char *&root_component_name)
{
  auto it = controlMap.find(control_id);
  if (it == controlMap.end())
    return nullptr;

  const ECSEditorPropertyInfo &editorPropertyInfo = it->second;

  if (editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayExtensibleContainer ||
      editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayAddButton)
  {
    G_ASSERT(editorPropertyInfo.hasParent());
    it = controlMap.find(editorPropertyInfo.parentControlId);
    G_ASSERT(it != controlMap.end());
    G_ASSERT(it->second.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup);
    return getComponentArray(eid, editorPropertyInfo.parentControlId, root_component_name);
  }
  else if (editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentArrayGroup)
  {
    ecs::EntityComponentRef componentRef = getComponentRef(eid, control_id, root_component_name);
    return componentRef.isNull() ? nullptr : &componentRef.getRW<ecs::Array>();
  }

  return nullptr;
}

ecs::Object *ECSPropertyPanelMapping::getComponentObject(ecs::EntityId eid, int control_id, const char *&root_component_name)
{
  auto it = controlMap.find(control_id);
  if (it == controlMap.end())
    return nullptr;

  const ECSEditorPropertyInfo &editorPropertyInfo = it->second;

  if (editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectExtensibleContainer ||
      editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectAddButton)
  {
    G_ASSERT(editorPropertyInfo.hasParent());
    it = controlMap.find(editorPropertyInfo.parentControlId);
    G_ASSERT(it != controlMap.end());
    G_ASSERT(it->second.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup);
    return getComponentObject(eid, editorPropertyInfo.parentControlId, root_component_name);
  }
  else if (editorPropertyInfo.propertyType == ECSEditorPropertyInfo::PropertyType::ComponentObjectGroup)
  {
    ecs::EntityComponentRef componentRef = getComponentRef(eid, control_id, root_component_name);
    return componentRef.isNull() ? nullptr : &componentRef.getRW<ecs::Object>();
  }

  return nullptr;
}
