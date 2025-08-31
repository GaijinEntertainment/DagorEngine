// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecsPropPanel/ecsBasicObjectEditor.h>
#include <ecsPropPanel/ecsOrderedTemplatesGroups.h>
#include "ecsUndoDeleteTemplate.h"

#include <daECS/core/entityManager.h>
#include <daECS/scene/scene.h>
#include <ecs/core/utility/ecsRecreate.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

static const ecs::HashedConstString EDITABLE_TEMPLATE_COMP = ECS_HASH("editableTemplate");

static void removeSelectedTemplateName(eastl::string &templ_name)
{
  templ_name = remove_sub_template_name(templ_name.c_str(), ECS_EDITOR_SELECTED_TEMPLATE);
}

ECSBasicObjectEditor::ECSBasicObjectEditor() {}
ECSBasicObjectEditor::~ECSBasicObjectEditor() {}

dag::Vector<String> ECSBasicObjectEditor::getEcsTemplates(const char *group_name)
{
  if (!orderedTemplatesGroups)
    orderedTemplatesGroups.reset(new ECSOrderedTemplatesGroups());
  return orderedTemplatesGroups->getEcsTemplates(group_name);
}

dag::Vector<String> ECSBasicObjectEditor::getEcsTemplatesGroups()
{
  if (!orderedTemplatesGroups)
    orderedTemplatesGroups.reset(new ECSOrderedTemplatesGroups());
  return orderedTemplatesGroups->getEcsTemplatesGroups();
}

bool ECSBasicObjectEditor::hasTemplateComponent(const char *template_name, const char *comp_name)
{
  if (!template_name || !comp_name)
    return false;
  bool found = false;
  const auto hash = ECS_HASH_SLOW(comp_name);
  for_each_sub_template_name(template_name, [&hash, &found](const char *tpl_name) {
    if (found)
      return;
    const ecs::Template *pTemplate = g_entity_mgr->getTemplateDB().getTemplateByName(tpl_name);
    if (pTemplate && !pTemplate->getComponent(hash, g_entity_mgr->getTemplateDB().data()).isNull())
      found = true;
  });
  return found;
}

bool ECSBasicObjectEditor::getSavedComponents(ecs::EntityId eid, eastl::vector<eastl::string> &out_comps)
{
  out_comps.clear();
  if (!ecs::g_scenes)
    return false;
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec || !erec->toBeSaved)
    return false;
  for (const auto &comp : erec->clist)
    out_comps.push_back(comp.first);
  return true;
}

void ECSBasicObjectEditor::resetComponent(ecs::EntityId eid, const char *comp_name)
{
  const auto tid = g_entity_mgr->getEntityTemplateId(eid);
  const char *tpl_name = g_entity_mgr->getTemplateName(tid);
  if (!tpl_name)
    return;
  const ecs::Template *tpl = g_entity_mgr->getTemplateDB().getTemplateByName(tpl_name);
  if (!tpl)
    return;
  const ecs::ChildComponent &tpl_comp = tpl->getComponent(ECS_HASH_SLOW(comp_name), g_entity_mgr->getTemplateDB().data());
  if (tpl_comp.getUserType() == 0) // no type means some complex component here
    return;
  g_entity_mgr->set(eid, ECS_HASH_SLOW(comp_name), eastl::move(ecs::ChildComponent(tpl_comp)));

  if (!ecs::g_scenes)
    return;
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  auto it = eastl::find_if(erec->clist.begin(), erec->clist.end(),
    [comp_name](const ecs::ComponentsList::value_type &kv) { return kv.first == comp_name; });
  if (it == erec->clist.end())
    return;
  erec->clist.erase(it);
  if (erec->toBeSaved)
    scene.setNewChangesApplied(true);
}

void ECSBasicObjectEditor::saveComponent(ecs::EntityId eid, const char *comp_name)
{
  if (!ecs::g_scenes)
    return;
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  if (!hasTemplateComponent(erec->templateName.c_str(), comp_name))
    return;
  const auto &comp = g_entity_mgr->getComponentRef(eid, ECS_HASH_SLOW(comp_name));
  auto it = eastl::find_if(erec->clist.begin(), erec->clist.end(),
    [comp_name](const ecs::ComponentsList::value_type &kv) { return kv.first == comp_name; });
  if (it != erec->clist.end())
    it->second = eastl::move(ecs::ChildComponent(*g_entity_mgr, comp));
  else
    erec->clist.emplace_back(comp_name, ecs::ChildComponent(*g_entity_mgr, comp));
  if (erec->toBeSaved)
    scene.setNewChangesApplied(true);
}

void ECSBasicObjectEditor::saveAddTemplate(ecs::EntityId eid, const char *templ_name)
{
  if (!ecs::g_scenes)
    return;
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  erec->templateName = add_sub_template_name(erec->templateName.c_str(), templ_name).c_str();
  removeSelectedTemplateName(erec->templateName);
  if (erec->toBeSaved)
    scene.setNewChangesApplied(true);
}

void ECSBasicObjectEditor::saveDelTemplate(ecs::EntityId eid, const char *templ_name, bool use_undo)
{
  if (!ecs::g_scenes)
    return;
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  const eastl::string newTemplName = remove_sub_template_name(erec->templateName.c_str(), templ_name).c_str();
  if (erec->templateName == newTemplName)
    return;

  ECSUndoDeleteTemplate *undo = nullptr;
  if (use_undo && IEditorCoreEngine::get()->getUndoSystem()->is_holding() && eid != ecs::INVALID_ENTITY_ID)
    undo = new ECSUndoDeleteTemplate(templ_name, eid);
  erec->templateName = newTemplName;
  removeSelectedTemplateName(erec->templateName);
  for (auto it = erec->clist.begin(); it != erec->clist.end();)
  {
    const char *comp_name = it->first.c_str();
    if (!hasTemplateComponent(erec->templateName.c_str(), comp_name))
    {
      if (undo)
        undo->componentList.push_back(eastl::make_pair(comp_name, std::move(it->second)));
      it = erec->clist.erase(it);
    }
    else
      ++it;
  }
  if (erec->toBeSaved)
    scene.setNewChangesApplied(true);
  if (undo)
    IEditorCoreEngine::get()->getUndoSystem()->put(undo);
}
