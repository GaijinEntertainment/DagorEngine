// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/template.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>
#include <EditorCore/ec_ObjectEditor.h>

#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class ECSOrderedTemplatesGroups;

#define ECS_EDITOR_SELECTED_TEMPLATE "daeditor_selected"

class ECSBasicObjectEditor : public ObjectEditor
{
public:
  ECSBasicObjectEditor();
  ~ECSBasicObjectEditor() override;

  dag::Vector<String> getEcsTemplates(const char *group_name);
  dag::Vector<String> getEcsTemplatesGroups();

  static bool hasTemplateComponent(const char *template_name, const char *comp_name);
  static bool getSavedComponents(ecs::EntityId eid, eastl::vector<eastl::string> &out_comps);
  static void resetComponent(ecs::EntityId eid, const char *comp_name);
  static void saveComponent(ecs::EntityId eid, const char *comp_name);
  static void saveAddTemplate(ecs::EntityId eid, const char *templ_name);
  static void saveDelTemplate(ecs::EntityId eid, const char *templ_name, bool use_undo = false);

protected:
  eastl::unique_ptr<ECSOrderedTemplatesGroups> orderedTemplatesGroups;
};
