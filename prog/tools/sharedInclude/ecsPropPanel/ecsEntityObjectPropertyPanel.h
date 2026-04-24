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

class ECSEntityObjectPropertyPanel
{
public:
  explicit ECSEntityObjectPropertyPanel(ECSBasicObjectEditor &editor, PropPanel::ContainerPropertyControl &main_panel);

  void fillProps(ecs::EntityId editable_object_eid, bool read_only = false);
  void onPPChange(int pid, bool edit_finished, ecs::EntityId editable_object_eid);
  void onPPBtnPressed(int pid, ecs::EntityId editable_object_eid);

private:
  void onAddTemplatePressed(ecs::EntityId editable_object_eid);
  void onRemoveTemplatePressed(ecs::EntityId editable_object_eid);
  void updateTemplateRemoveButton();
  void setComponentValueByPropertyPanel(int control_id, ecs::EntityComponentRef &component);
  void createSceneControl(ecs::EntityId editable_object_eid);

  static bool addComponentArrayElementPressed(ecs::Array &component_array);
  static bool removeComponentArrayElementPressed(ecs::Array &component_array, int element_index);
  static bool addComponentObjectMemberPressed(ecs::Object &component_object);
  static bool removeComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name);
  static bool renameComponentObjectMemberPressed(ecs::Object &component_object, const char *member_name);
  static void fillComponentPropertyPanel(PropPanel::ContainerPropertyControl &panel, int &control_id,
    const ecs::EntityComponentRef &component, const char *name, int parent_control_id, bool read_only);

  ECSBasicObjectEditor &editor;
  PropPanel::ContainerPropertyControl &mainPanel;
};
