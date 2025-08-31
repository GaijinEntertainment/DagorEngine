// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace ecs
{
class Array;
class Object;
class EntityComponentRef;
class EntityId;
} // namespace ecs

namespace PropPanel
{
class ContainerPropertyControl;
}

class ECSBasicObjectEditor;

class ECSEditableObjectPropertyPanel
{
public:
  explicit ECSEditableObjectPropertyPanel(PropPanel::ContainerPropertyControl &main_panel);

  void fillProps(ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed);
  void onPPChange(int pid, bool edit_finished, ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed);
  void onPPBtnPressed(int pid, ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed);

private:
  void onAddTemplatePressed(ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed);
  void onRemoveTemplatePressed(ecs::EntityId editable_object_eid, ECSBasicObjectEditor *editable_object_objed);
  void updateTemplateRemoveButton();
  void setComponentValueByPropertyPanel(int control_id, ecs::EntityComponentRef &component);

  static bool addComponentArrayElementPressed(ecs::Array &component_array);
  static bool removeComponentArrayElementPressed(ecs::Array &component_array, int element_index);
  static bool addComponentObjectMemberPressed(ecs::Object &component_object);
  static bool removeComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name);
  static bool renameComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name);
  static void fillComponentPropertyPanel(PropPanel::ContainerPropertyControl &panel, int &control_id,
    const ecs::EntityComponentRef &component, const char *name, int parent_control_id);

  PropPanel::ContainerPropertyControl &mainPanel;
};
