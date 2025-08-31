// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecsPropPanel/ecsBasicObjectEditor.h>

#include <daECS/scene/scene.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <libTools/util/undo.h>

class ECSUndoDeleteTemplate : public UndoRedoObject
{
private:
  eastl::string removedTemplate_;
  ecs::EntityId eid;

public:
  ecs::ComponentsList componentList;

  ECSUndoDeleteTemplate(const char *removedTemplate, ecs::EntityId _eid) : removedTemplate_(removedTemplate), eid(_eid) {}

  void restore(bool /*save_redo*/) override
  {
    ecs::ComponentsInitializer init;
    eastl::vector<eastl::string> componentsName;
    componentsName.reserve(componentList.size());
    for (const auto &comp : componentList)
    {
      init[ECS_HASH_SLOW(comp.first.c_str())] = comp.second;
      componentsName.push_back(comp.first);
    }
    add_sub_template_async(eid, removedTemplate_.c_str(), std::move(init),
      [componentsName = std::move(componentsName)](ecs::EntityId eid) {
        for (const auto &comp : componentsName)
          ECSBasicObjectEditor::saveComponent(eid, comp.c_str());
      });
    ECSBasicObjectEditor::saveAddTemplate(eid, removedTemplate_.c_str());
  }

  void redo() override
  {
    remove_sub_template_async(eid, removedTemplate_.c_str());
    ECSBasicObjectEditor::saveDelTemplate(eid, removedTemplate_.c_str(), false /*use_undo*/);
  }

  size_t size() override { return sizeof(*this) + removedTemplate_.capacity() + componentList.capacity(); }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoRedoObject"; }
};