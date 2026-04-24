// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsObjectEditor.h"
#include "ecsEntityObject.h"
#include "ecsSceneObject.h"
#include "ecsEditorCm.h"
#include "ecsEditorCopyDlg.h"
#include "ecsEditorMain.h"
#include "ecsHierarchicalUndoGroup.h"
#include "ecsSceneOutliner/ecsSceneOutlinerPanel.h"
#include <ecsPropPanel/ecsOrderedTemplatesGroups.h>
#include <ecsPropPanel/ecsTemplateSelectorDialog.h>
#include <ecsPropPanel/ecsEditorTemplates.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/io/blk.h>
#include <de3_interface.h>
#include <oldEditor/de_workspace.h>
#include <debug/dag_debug3d.h>
#include <daECS/core/utility/ecsRecreate.h>
#include <ecs/rendInst/riExtra.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_IObjectCreator.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_files.h>
#include <memory/dag_framemem.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

void ecs_editor_release_ri_extra_substitutes();
bool ecs_editor_ignore_entity_in_collision_test_begin(ecs::EntityId eid);
void ecs_editor_ignore_entity_in_collision_test_end(ecs::EntityId eid, const TMatrix &original_tm);
void ecs_editor_ignore_entities_for_export_begin();
void ecs_editor_ignore_entities_for_export_end();

namespace
{
enum
{
  SCENE_OUTLINER_EDITOR_WTYPE = 330,
};

enum
{
  SCENE_OUTLINER_WIDTH = 280,
};

const ecs::HashedConstString EDITABLE_TEMPLATE_COMP = ECS_HASH("editableTemplate");
ECSObjectEditor *ecs_object_editor_instance = nullptr;

struct ECSSceneItemSaveObj
{
  ecs::Scene::SceneRecord::OrderedEntry entry;
  const ecs::Scene::EntityRecord *erec = nullptr;
  uint64_t position;
  uint64_t priority;
  bool operator<(const ECSSceneItemSaveObj &rhs) const
  {
    if (priority != rhs.priority)
      return priority < rhs.priority;
    return position < rhs.position;
  }
};

uint64_t calc_save_priority(const char *template_name, const ECSObjectEditor::SaveOrderRules &rules)
{
  for (auto &prefix : rules)
  {
    bool found = false;
    for_each_sub_template_name(template_name, [&](const char *tpl_name) {
      if (found)
        return;
      const auto &db = g_entity_mgr->getTemplateDB();
      const ecs::Template *pTemplate = db.getTemplateByName(tpl_name);
      if (!pTemplate)
        return;
      db.data().iterate_template_parents(*pTemplate, [&](const ecs::Template &tmpl) {
        if (found)
          return;
        const auto &comps = tmpl.getComponentsMap();
        for (const auto &c : comps)
        {
          const char *compName = db.getComponentName(c.first);
          if (compName && strncmp(prefix.c_str(), compName, prefix.length()) == 0)
          {
            found = true;
            return;
          }
        }
      });
    });
    if (found)
      return eastl::distance(rules.begin(), &prefix);
  }
  return rules.size();
}

inline bool is_create_entity_mode(int m) { return m == CM_ECS_EDITOR_CREATE_ENTITY; }

inline bool is_entity_editable(ecs::EntityId eid) { return g_entity_mgr->getOr(eid, ECS_HASH("editableObj"), true); }

inline bool is_scene_entity(ECSEntityObject *o)
{
  if (!o)
    return false;
  const auto *erec = ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid());
  return erec;
}

const char *check_singleton_mutex_exists(const ecs::Template *templ, const ecs::TemplatesData &templates_data)
{
  auto &c = templ->getComponent(ECS_HASH("singleton_mutex"), templates_data);
  if (!c.isNull())
    if (ECSObjectEditor::checkSingletonMutexExists(c.getOr("")))
      return c.getOr("");
  return nullptr;
}

Point2 cmp_obj_z_pt;
IGenViewportWnd *cmp_obj_z_wnd = nullptr;
int cmp_obj_z(RenderableEditableObject *const *a, RenderableEditableObject *const *b)
{
  Point3 pt_a = a[0]->getPos();
  Point3 pt_b = b[0]->getPos();

  Point3 tmp;
  Point3 dir;
  cmp_obj_z_wnd->clientToWorld(cmp_obj_z_pt, tmp, dir);

  BBox3 box;
  if (a[0]->getWorldBox(box))
    pt_a += dir * length(box.width()) / 2;
  if (b[0]->getWorldBox(box))
    pt_b += dir * length(box.width()) / 2;

  Point2 scr_a, scr_b;
  float a_z = 0, b_z = 0;
  if (!cmp_obj_z_wnd->worldToClient(pt_a, scr_a, &a_z))
    a_z = 100000;
  else if (a_z < 0)
    a_z = 100000 - a_z;
  if (!cmp_obj_z_wnd->worldToClient(pt_b, scr_b, &b_z))
    b_z = 100000;
  else if (b_z < 0)
    b_z = 100000 - b_z;

  return (a_z < b_z) ? 1 : (a_z > b_z ? -1 : 0);
}

// Check if setting the child's parent to parent would result in a loop.
bool is_safe_to_set_parent(ECSEntityObject &child, ECSEntityObject &parent)
{
  ECSEntityObject *loopObject = &child;
  while (true)
  {
    ECSEntityObject *newParent = loopObject == &child ? &parent : loopObject->getParentObject();
    if (!newParent)
      return true;
    if (newParent == &child)
      return false;

    loopObject = newParent;
  }
}

inline bool can_transform_object_freely(RenderableEditableObject *object)
{
  if (auto *entityObj = RTTI_cast<ECSEntityObject>(object))
    return entityObj->canTransformFreely();

  return true;
}
} // namespace

ECSObjectEditor::ECSObjectEditor()
{
  ecs_object_editor_instance = this;
  addEditableEntities();

  if (dd_check_named_mount_in_path_valid("%gameBase") || dd_check_named_mount_in_path_valid("%gameBasePkgDev"))
  {
    ecs::resolve_import_path = [](const char *import_path, String &import_buffer) -> const char * {
      if (dd_file_exist(import_path))
      {
        return import_path;
      }

      import_buffer.printf(0, "%%gameBase/%s", import_path);
      if (dd_file_exist(import_buffer))
      {
        return import_buffer.c_str();
      }

      import_buffer.printf(0, "%%gameBasePkgDev/%s", import_path);
      if (dd_file_exist(import_buffer))
      {
        return import_buffer.c_str();
      }
      return nullptr;
    };
  }
}

ECSObjectEditor::~ECSObjectEditor()
{
  setCreateBySampleMode(nullptr); //-V1053
  selectNewObjEntity(nullptr);
  reset();
  ecs_object_editor_instance = nullptr;
}

void ECSObjectEditor::reset()
{
  removeAllObjects(false);
  setCreateBySampleMode(nullptr);
  selectNewObjEntity(nullptr);

  clear_and_shrink(objects);
  clear_and_shrink(selection);
  eidsCreated.clear();
  eidToEditableObject.clear();
  sidsCreated.clear();
  sidToEditableObject.clear();
  hiddenTemplates.clear();
  objsToRemove.clear();

  if (sceneOutliner)
  {
    sceneOutliner->resetPanel([this](const eastl::function<void(ecs::EntityId eid)> &cb) { onOutlinerPanelFill(cb); });
  }

  // Make sure that nothing leaks to the next loaded level.
  g_entity_mgr->tick(true);

  if (ecs::g_scenes)
    ecs::g_scenes->clearScene();
  ecs::SceneManager::resetEntityCreationCounter();

  ecs_editor_release_ri_extra_substitutes();
}

template <typename F>
inline void ECSObjectEditor::forEachEditableObject(F fn)
{
  for (EditableObject *eo : objects)
    if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(eo))
      fn(o);
}

template <typename F>
inline void ECSObjectEditor::forEachEditableObjectWithTemplate(F fn)
{
  for (EditableObject *eo : objects)
    if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(eo))
      if (const char *entTempl = g_entity_mgr->getEntityTemplateName(o->getEid()))
        fn(o, entTempl);
}

void ECSObjectEditor::updateImgui()
{
  if (sceneOutliner)
  {
    PropPanel::PanelWindowPropertyControl *panelWindow = sceneOutliner->getPanel();

    bool open = true;
    DAEDITOR3.imguiBegin(*panelWindow, &open);
    sceneOutliner->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open && sceneOutliner)
    {
      EDITORCORE->removePropPanel(sceneOutliner.get());
      EDITORCORE->managePropPanels();
    }
  }

  Base::updateImgui();
}

RenderableEditableObject *ECSObjectEditor::getObjectFromSceneId(ecs::Scene::SceneId sid) const
{
  auto find = sidToEditableObject.find(sid);
  return find != sidToEditableObject.end() ? find->second : nullptr;
}

RenderableEditableObject *ECSObjectEditor::getObjectFromEntityId(ecs::EntityId eid) const
{
  auto find = eidToEditableObject.find(eid);
  return find != eidToEditableObject.end() ? find->second : nullptr;
}

uint32_t ECSObjectEditor::writeSceneToBlk(DataBlock &blk, const ecs::Scene::SceneRecord &record, ECSObjectEditor &editor)
{
  const bool isMain = record.importDepth == 0;
  eastl::vector<ECSSceneItemSaveObj, framemem_allocator> saveItems;
  saveItems.reserve(record.orderedEntries.size());

  const ecs::TemplateDB &templates = g_entity_mgr->getTemplateDB();
  int hierarchySaveId = 2;
  ska::flat_hash_map<ecs::EntityId, int, ecs::EidHash> hiearchySaveIds;

  for (uint64_t i = 0; i < record.orderedEntries.size(); ++i)
  {
    const ecs::Scene::SceneRecord::OrderedEntry &entry = record.orderedEntries[i];
    const ecs::Scene::EntityRecord *erec = nullptr;
    if (entry.isEntity)
    {
      erec = ecs::g_scenes->getActiveScene().findEntityRecord(entry.eid);
    }

    if (entry.isEntity && !erec)
    {
      continue;
    }

    saveItems.push_back(ECSSceneItemSaveObj{.entry = entry,
      .erec = erec,
      .position = i,
      .priority = entry.isEntity ? calc_save_priority(erec->templateName.c_str(), editor.getSaveOrderRules())
                                 : editor.getSaveOrderRules().size()});
  }

  if (isMain)
  {
    editor.forEachEditableObject([&](ECSEntityObject *o) {
      if (auto erec = ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid()); erec)
      {
        if (record.entities.find(o->getEid()) == record.entities.end() && erec->loadType == ecs::Scene::UNKNOWN &&
            erec->sceneId == ecs::Scene::C_INVALID_SCENE_ID)
        {
          saveItems.push_back(ECSSceneItemSaveObj{.entry = ecs::Scene::SceneRecord::OrderedEntry{.eid = o->getEid(), .isEntity = true},
            .erec = erec,
            .position = saveItems.size(),
            .priority = calc_save_priority(erec->templateName.c_str(), editor.getSaveOrderRules())});
        }
      }
    });
  }

  for (const ECSSceneItemSaveObj &item : saveItems)
  {
    if (item.erec)
    {
      eastl::vector<eastl::string, framemem_allocator> subTemplsToRemove;
      if (const char *entTempl = g_entity_mgr->getEntityTemplateName(item.entry.eid))
        if (const ecs::Template *templ = templates.getTemplateByName(entTempl))
          for (const uint32_t p : templ->getParents())
            if (auto *t = g_entity_mgr->getTemplateDB().getTemplateById(p))
              if (auto *cc = t->getComponentsMap().find(EDITABLE_TEMPLATE_COMP))
                if (!cc->getOr(true))
                  subTemplsToRemove.push_back(t->getName());
      for (const auto &subTempl : subTemplsToRemove)
        remove_sub_template_async(item.entry.eid, subTempl.c_str());

      const ecs::EntityId *hierarchyEid = g_entity_mgr->getNullable<ecs::EntityId>(item.entry.eid, ECS_HASH("hierarchy_eid"));
      if (hierarchyEid && *hierarchyEid)
      {
        int finalHierarchySaveId = hierarchySaveId;
        G_ASSERT((finalHierarchySaveId & 1) == 0);

        const char *templateName = g_entity_mgr->getEntityTemplateName(item.entry.eid);
        if (find_sub_template_name(templateName, ECS_EDITOR_HIERARCHIAL_FREE_TRANSFORM_TEMPLATE))
          finalHierarchySaveId |= 1;

        bool inserted;
        eastl::tie(eastl::ignore, inserted) = hiearchySaveIds.insert({item.entry.eid, finalHierarchySaveId});
        G_ASSERT(inserted);
        hierarchySaveId += 2;
      }
    }
  }

  g_entity_mgr->tick();                            // so we apply all remove sub template
  eastl::sort(saveItems.begin(), saveItems.end()); // sort by save priority and scene order, new entites go last

  TMatrix parentTm = TMatrix::IDENT;
  if (record.loadType == ecs::Scene::IMPORT)
  {
    parentTm = record.rootTransform.value_or(TMatrix::IDENT);
    ecs::Scene::SceneId parentId = record.parent;
    while (parentId != ecs::Scene::SceneRecord::NO_PARENT)
    {
      if (const ecs::Scene::SceneRecord *parentRec = ecs::g_scenes->getActiveScene().getSceneRecordById(parentId))
      {
        parentTm = parentRec->rootTransform.value_or(TMatrix::IDENT) * parentTm;
        parentId = parentRec->parent;
      }
      else
      {
        break;
      }
    }
  }

  if (!record.transformable) // do not write true, it is default value
  {
    blk.addBool("transformable_scene", record.transformable);
  }
  if (record.pivot != Point3::ZERO)
  {
    blk.addPoint3("pivot_scene", record.pivot);
  }
  if (!record.prettyName.empty())
  {
    blk.addStr("pretty_name_scene", record.prettyName);
  }

  for (const ECSSceneItemSaveObj &item : saveItems)
  {
    if (item.erec)
    {
      G_ASSERT(item.entry.isEntity);

      DataBlock *entityBlock = blk.addNewBlock("entity");
      if (record.loadType == ecs::Scene::IMPORT && item.erec->wasTransformChanged)
      {
        auto recordCopy = *item.erec;

        if (auto it = eastl::find_if(recordCopy.clist.begin(), recordCopy.clist.end(),
              [](const auto &value) { return value.first == "transform"; });
            it != recordCopy.clist.end())
        {
          TMatrix &currentTm = it->second.getRW<TMatrix>();
          currentTm = currentTm * inverse(parentTm);
        }

        ECSEntityObject::save(*entityBlock, recordCopy);
      }
      else
      {
        ECSEntityObject::save(*entityBlock, *item.erec);
      }

      entityBlock->removeParam("hierarchy_unresolved_id");
      entityBlock->removeParam("hierarchy_unresolved_parent_id");

      auto hiearchySaveIdIt = hiearchySaveIds.find(item.entry.eid);
      if (hiearchySaveIdIt != hiearchySaveIds.end())
      {
        const int unresolvedId = hiearchySaveIdIt->second;

        // We remove the hierarchial_free_transform template and restore it manually after loading. This is needed to
        // prevent having wrong transforms due to async ECS events interfering with the loaded data.
        if ((unresolvedId & 1) == 1)
        {
          if (const char *templateName = entityBlock->getStr("_template"))
          {
            auto finalTemplateName = remove_sub_template_name(templateName, ECS_EDITOR_HIERARCHIAL_FREE_TRANSFORM_TEMPLATE);
            entityBlock->setStr("_template", finalTemplateName.c_str());
          }
        }

        entityBlock->addInt("hierarchy_unresolved_id", unresolvedId);

        const ecs::EntityId *parentId = g_entity_mgr->getNullable<ecs::EntityId>(item.entry.eid, ECS_HASH("hierarchy_parent"));
        if (parentId && *parentId)
        {
          auto hiearchySaveIdParentIt = hiearchySaveIds.find(*parentId);
          if (hiearchySaveIdParentIt != hiearchySaveIds.end())
            entityBlock->addInt("hierarchy_unresolved_parent_id", hiearchySaveIdParentIt->second);
        }
      }
    }
    else
    {
      if (const ecs::Scene::SceneRecord *srec = ecs::g_scenes->getActiveScene().getSceneRecordById(item.entry.sid))
      {
        DataBlock *importBlock = blk.addNewBlock("import");
        importBlock->addStr("scene", srec->path.c_str());
        if (srec->rootTransform && srec->rootTransform.value() != TMatrix::IDENT)
        {
          importBlock->addTm("transform", srec->rootTransform.value());
        }
      }
    }
  }

  return saveItems.size();
}

void ECSObjectEditor::registerEditorCommands(IEditorCommandSystem &command_system)
{
  ECSBasicObjectEditor::registerEditorCommands(command_system);

  command_system.addCommand(EditorCommandIds::ECS_EDITOR_CREATE_ENTITY);
  command_system.addCommand(EditorCommandIds::ECS_EDITOR_SET_PARENT, ImGuiMod_Ctrl | ImGuiKey_P);
  command_system.addCommand(EditorCommandIds::ECS_EDITOR_CLEAR_PARENT, ImGuiMod_Alt | ImGuiKey_P);
  command_system.addCommand(EditorCommandIds::ECS_EDITOR_TOGGLE_FREE_TRANSFORM);
  command_system.addCommand(EditorCommandIds::ECS_EDITOR_SCENE_OUTLINER, ImGuiMod_Ctrl | ImGuiKey_O);
}

void ECSObjectEditor::registerViewportAccelerators(IWndManager &wndManager)
{
  ObjectEditor::registerViewportAccelerators(wndManager);

  wndManager.addViewportAccelerator(CM_ECS_EDITOR_CREATE_ENTITY, EditorCommandIds::ECS_EDITOR_CREATE_ENTITY);
  wndManager.addViewportAccelerator(CM_ECS_EDITOR_TOGGLE_FREE_TRANSFORM, EditorCommandIds::ECS_EDITOR_TOGGLE_FREE_TRANSFORM);
  wndManager.addViewportAccelerator(CM_ECS_EDITOR_SET_PARENT, EditorCommandIds::ECS_EDITOR_SET_PARENT);
  wndManager.addViewportAccelerator(CM_ECS_EDITOR_CLEAR_PARENT, EditorCommandIds::ECS_EDITOR_CLEAR_PARENT);
  wndManager.addViewportAccelerator(CM_ECS_EDITOR_SCENE_OUTLINER, EditorCommandIds::ECS_EDITOR_SCENE_OUTLINER);
}

void ECSObjectEditor::fillToolBar(PropPanel::ContainerPropertyControl *toolbar)
{
  ObjectEditor::fillToolBar(toolbar);

  PropPanel::ContainerPropertyControl *tb1 = toolbar->createToolbarPanel(0, "");
  addEditorCommandButton(tb1, CM_ECS_EDITOR_CREATE_ENTITY, EditorCommandIds::ECS_EDITOR_CREATE_ENTITY, "create_lib_ent",
    "Create entity", true);

  addEditorCommandButton(tb1, CM_ECS_EDITOR_SET_PARENT, EditorCommandIds::ECS_EDITOR_SET_PARENT, "area_link",
    "Set parent\n\nThe next clicked object will be set as the parent of the currently selected objects.", true);
  addEditorCommandButton(tb1, CM_ECS_EDITOR_CLEAR_PARENT, EditorCommandIds::ECS_EDITOR_CLEAR_PARENT, "area_split", "Clear parent");
  addEditorCommandButton(tb1, CM_ECS_EDITOR_TOGGLE_FREE_TRANSFORM, EditorCommandIds::ECS_EDITOR_TOGGLE_FREE_TRANSFORM,
    "asset_a2d_changed", "Toggle between free and fixed transform in a child entity");
  addEditorCommandButton(tb1, CM_ECS_EDITOR_SCENE_OUTLINER, EditorCommandIds::ECS_EDITOR_SCENE_OUTLINER, "scene_hierarchy",
    "Show scene outliner", true);
}

void ECSObjectEditor::updateToolbarButtons()
{
  ObjectEditor::updateToolbarButtons();

  setRadioButton(CM_ECS_EDITOR_CREATE_ENTITY, getEditMode());
  setRadioButton(CM_ECS_EDITOR_SET_PARENT, getEditMode());
}

void ECSObjectEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == CM_ECS_EDITOR_CREATE_ENTITY)
  {
    setEditMode(CM_ECS_EDITOR_CREATE_ENTITY);

    if (!templateSelectorDialog)
    {
      templateSelectorDialog.reset(new ECSTemplateSelectorDialog("Template selector###AssetSelectorPanel", this));
      templateSelectorDialog->showButtonPanel(false);
      templateSelectorDialog->setClient(this);
    }

    templateSelectorDialog->show();

    const char *templateName = templateSelectorDialog->getSelectedTemplate();
    if (templateName != nullptr && *templateName != 0)
      onTemplateSelectorTemplateSelected(templateName);
  }
  else if (pcb_id == CM_ECS_EDITOR_SET_PARENT)
  {
    objectsForParenting.clear();

    for (EditableObject *editableObject : selection)
    {
      G_ASSERT(editableObject);
      ECSEntityObject *ecsEntityObject = RTTI_cast<ECSEntityObject>(editableObject);
      G_ASSERT(ecsEntityObject);
      G_ASSERT(ecsEntityObject->getEid());
      objectsForParenting.push_back(ecsEntityObject->getEid());
    }

    if (objectsForParenting.empty())
    {
      ecs_editor_set_viewport_message("Not entity is selected, cannot set parent", /*auto_hide = */ true);
      updateToolbarButtons(); // uncheck the "Set parent" button
    }
    else
    {
      setEditMode(CM_ECS_EDITOR_SET_PARENT);
      ecs_editor_set_viewport_message("Select the parent object");
    }
  }
  else if (pcb_id == CM_ECS_EDITOR_CLEAR_PARENT)
  {
    clearParentForSelection();
    invalidateObjectProps();
    ecs_editor_set_viewport_message("Parent cleared", /*auto_hide = */ true);
  }
  else if (pcb_id == CM_ECS_EDITOR_TOGGLE_FREE_TRANSFORM)
  {
    toggleFreeTransformForSelection();
    invalidateObjectProps();
    ecs_editor_set_viewport_message("Toggled free transform", /*auto_hide = */ true);
  }
  else if (pcb_id == CM_ECS_EDITOR_SCENE_OUTLINER)
  {
    if (!sceneOutliner)
    {
      EDITORCORE->addPropPanel(SCENE_OUTLINER_EDITOR_WTYPE, hdpi::_pxScaled(SCENE_OUTLINER_WIDTH));
    }
    else
    {
      EDITORCORE->removePropPanel(sceneOutliner.get());
    }

    EDITORCORE->managePropPanels();
  }
  else
  {
    ObjectEditor::onClick(pcb_id, panel);
  }
}

ecs::EntityId ECSObjectEditor::createEntitySample(ECSEntityCreateData *data)
{
  data->eid = ecs::INVALID_ENTITY_ID;
  ECSEntityCreateData::createCb(data);
  if (data->eid == ecs::INVALID_ENTITY_ID)
    logerr("failed to create sample entity");
  return data->eid;
}

ecs::EntityId ECSObjectEditor::createEntityDirect(ECSEntityCreateData *data)
{
  data->eid = ecs::INVALID_ENTITY_ID;
  removeSelectedTemplateName(data->templName);
  ECSEntityCreateData::createCb(data);
  if (data->eid != ecs::INVALID_ENTITY_ID)
    ecs::g_scenes->getActiveScene().insertEmptyEntityRecord(data->eid, data->templName.c_str());
  else
    logerr("failed to create new entity");
  return data->eid;
}

void ECSObjectEditor::destroyEntityDirect(ecs::EntityId eid)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return;
  eidsCreated.erase(eid);
  if (ecs::g_scenes.get())
    ecs::g_scenes->getActiveScene().eraseEntityRecord(eid);
  g_entity_mgr->destroyEntity(eid);
}

void ECSObjectEditor::addEntityEx(ecs::EntityId eid, bool use_undo)
{
  const bool isUndoHolding = getUndoSystem()->is_holding();
  if (use_undo && !isUndoHolding)
    getUndoSystem()->begin();

  eidsCreated.insert(eid);

  ECSEntityObject *o = new ECSEntityObject(eid);
  addObject(o, use_undo);

  if (showOnlySceneEntities && !is_scene_entity(o))
    o->hideObject(true);

  if (ecs::g_scenes.get())
  {
    if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(o->getEid()))
    {
      removeSelectedTemplateName(pRec->templateName);
      remove_sub_template_async(o->getEid(), ECS_EDITOR_SELECTED_TEMPLATE);
    }
  }

  if (use_undo && !isUndoHolding)
    getUndoSystem()->accept("Add Entity");
}

void ECSObjectEditor::addScene(ecs::Scene::SceneId sid)
{
  ecs::Scene &activeScene = ecs::g_scenes->getActiveScene();
  if (const ecs::Scene::SceneRecord *srecord = activeScene.getSceneRecordById(sid); srecord && sidToEditableObject.count(sid) == 0)
  {
    auto sceneObj = new ECSSceneObject(srecord->id);
    sidsCreated.insert(sid);
    addObject(sceneObj, false);
    sceneObj->createEntityObjects(this);
  }

  updateEcsScenes();
}

void ECSObjectEditor::removeScene(ecs::Scene::SceneId sid)
{
  if (sidsCreated.count(sid) != 0)
  {
    auto it = sidToEditableObject.find(sid);
    if (it == sidToEditableObject.cend())
    {
      logerr("daEd4: Failed to remove scene object %d. It does not exist in mapping", sid);
      return;
    }

    sidsCreated.erase(sid);
    RenderableEditableObject *sceneObj = it->second;
    removeObject(sceneObj, false);
  }

  updateEcsScenes();
  {
    ecs::Scene::SceneId id = ecs::g_scenes->getActiveScene().getTargetSceneId();
    if (id == sid)
    {
      ecs::g_scenes->getActiveScene().setTargetSceneId(ecs::Scene::C_INVALID_SCENE_ID);
    }
  }
  if (templateSelectorDialog)
  {
    templateSelectorDialog->updateScenes();
  }
}

bool ECSObjectEditor::setParentForObjects(dag::ConstSpan<ECSEntityObject *> children, ECSEntityObject &parent)
{
  if (children.empty())
    return false;

  bool parentSet = false;
  for (ECSEntityObject *child : children)
    if (is_safe_to_set_parent(*child, parent))
    {
      parentSet = true;

      ecs::ComponentsInitializer comps;
      comps[ECS_HASH("hierarchy_parent")] = parent.getEid();
      add_sub_template_async(child->getEid(), "hierarchial_entity", eastl::move(comps));
      saveAddTemplate(child->getEid(), "hierarchial_entity");
      saveAddTemplate(parent.getEid(), "hierarchial_entity");
    }

  return parentSet;
}

void ECSObjectEditor::clearParentForSelection()
{
  for (int i = 0; i < selection.size(); ++i)
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(selection[i]))
    {
      remove_sub_template_async(entityObj->getEid(), "hierarchial_entity");
      saveDelTemplate(entityObj->getEid(), "hierarchial_entity");
    }
}

void ECSObjectEditor::toggleFreeTransformForSelection()
{
  if (selection.empty())
    return;

  ECSEntityObject *referenceEntityObj = RTTI_cast<ECSEntityObject>(selection[selection.size() - 1]);
  if (!referenceEntityObj)
    return;

  const char *freeComponentName = "hierarchial_free_transform";
  const char *referenceTemplateName = g_entity_mgr->getEntityTemplateName(referenceEntityObj->getEid());
  const bool referenceIsFree = find_sub_template_name(referenceTemplateName, freeComponentName);

  for (int i = 0; i < selection.size(); ++i)
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(selection[i]))
    {
      if (referenceIsFree)
      {
        remove_sub_template_async(entityObj->getEid(), freeComponentName);
        saveDelTemplate(entityObj->getEid(), freeComponentName);
      }
      else
      {
        add_sub_template_async(entityObj->getEid(), freeComponentName);
        saveAddTemplate(entityObj->getEid(), freeComponentName);
      }
    }
}

void ECSObjectEditor::setEditMode(int cm)
{
  if (getEditMode() == cm)
    return;

  if (is_create_entity_mode(getEditMode()))
  {
    setCreateBySampleMode(nullptr);
    selectNewObjEntity(nullptr);
  }

  ObjectEditor::setEditMode(cm);

  if (is_create_entity_mode(getEditMode()))
  {
    if (!newObj.get())
    {
      newObj = new ECSEntityObject(ecs::INVALID_ENTITY_ID);
      newObjOk = true;
      setCreateBySampleMode(newObj);
    }
  }
  else
  {
    setCreateBySampleMode(nullptr);
    selectNewObjEntity(nullptr);

    if (templateSelectorDialog)
      templateSelectorDialog->hide();
  }

  parentSelectionHoveredObject = nullptr;

  ecs_editor_set_viewport_message("");
}

void ECSObjectEditor::selectNewObjEntity(const char *name)
{
  if (!newObj.get())
    return;

  newObjTemplate = name;

  if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(newObj))
  {
    if (name && *name)
    {
      String previewTemplateName = String(name);
      if (g_entity_mgr->getTemplateDB().getTemplateByName(ECS_EDITOR_PREVIEW_TEMPLATE))
        previewTemplateName = String(0, "%s+%s", name, ECS_EDITOR_PREVIEW_TEMPLATE);

      const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(previewTemplateName);
      if (templ && templ->isSingleton())
      {
        o->resetEid();
        if (g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(name)) != ecs::INVALID_ENTITY_ID)
        {
          DAEDITOR3.conWarning("singleton <%s> already created", name);
          newObjTemplate.clear();
          name = nullptr;
        }
        else if (const char *mutex_name = check_singleton_mutex_exists(templ, g_entity_mgr->getTemplateDB().data()))
        {
          DAEDITOR3.conWarning("entity with singleton_mutex=<%s> already created, <%s> cannot be created", mutex_name, name);
          newObjTemplate.clear();
          name = nullptr;
        }
      }
      else
      {
        const TMatrix tm = o->getWtm();

        if (o->getEid() != ecs::INVALID_ENTITY_ID)
          g_entity_mgr->destroyEntity(o->getEid());

        ECSEntityCreateData cd(previewTemplateName, &tm);
        o->replaceEid(createEntitySample(&cd));
        o->setWtm(tm);

        newObjOk = o->getEid() != ecs::INVALID_ENTITY_ID;
      }
    }
    else
    {
      const TMatrix tm = o->getWtm();

      if (o->getEid() != ecs::INVALID_ENTITY_ID)
        g_entity_mgr->destroyEntity(o->getEid());

      o->resetEid();
      o->setWtm(tm);

      newObjOk = true;
    }
  }

  if (!name && !is_create_entity_mode(getEditMode()))
  {
    newObj = nullptr;
    newObjOk = false;
  }
}

void ECSObjectEditor::createObjectBySample(RenderableEditableObject *so)
{
  ECSEntityObject *o = RTTI_cast<ECSEntityObject>(so);
  G_ASSERT(o);

  if (newObjTemplate.empty())
    return;

  RenderableEditableObject *no = nullptr;
  eastl::string templName = newObjTemplate.c_str();
  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(newObjTemplate);
  bool singl = templ && templ->isSingleton();

  if (singl)
  {
    G_ASSERTF_RETURN(o->getEid() == ecs::INVALID_ENTITY_ID, , "o->getEid()=%u but template is singleton",
      (unsigned)(ecs::entity_id_t)o->getEid());

    ECSEntityCreateData cd(templName.c_str());
    no = new ECSEntityObject(createEntityDirect(&cd));
    newObjTemplate.clear();
  }

  if (!no)
  {
    if (o->getEid() == ecs::INVALID_ENTITY_ID)
    {
      if (!newObjOk)
        DAEDITOR3.conWarning("%sentity for (%s) not created", singl ? "singleton " : "", templName.c_str());
      return;
    }
    no = o->cloneObject(templName.c_str());
    if (!no)
      return;
  }

  no->setWtm(o->getWtm());

  getUndoSystem()->begin();
  addObject(no);
  unselectAll();
  no->selectObject();
  if (ECSEntityObject *noEntityObj = RTTI_cast<ECSEntityObject>(no))
  {
    eidsCreated.insert(noEntityObj->getEid());

    if (noEntityObj->hasTransform())
    {
      saveComponent(noEntityObj->getEid(), "transform");

      lastCreatedObj = no;
      lastCreateObjFrames = 5;
      lastCreatedObjPos = lastCreatedObj->getPos();
    }

    if (singl || !noEntityObj->hasTransform())
      DAEDITOR3.conNote("created %sentity %u (%s)", singl ? "singleton " : "", noEntityObj->getEid(), templName.c_str());
  }
  getUndoSystem()->accept("Add Entity");
}

bool ECSObjectEditor::cancelCreateMode()
{
  if (!is_create_entity_mode(getEditMode()))
    return false;

  if (!newObj.get())
    return false;

  bool wasSample = false;
  if (ECSEntityObject *newEntObj = RTTI_cast<ECSEntityObject>(newObj))
    if (newEntObj->getEid() != ecs::INVALID_ENTITY_ID)
      wasSample = true;

  setCreateBySampleMode(nullptr);
  selectNewObjEntity(nullptr);

  if (wasSample)
  {
    newObj = new ECSEntityObject(ecs::INVALID_ENTITY_ID);
    newObjOk = true;
    setCreateBySampleMode(newObj);
  }
  else
  {
    setEditMode(CM_OBJED_MODE_SELECT);
  }

  return true;
}

bool ECSObjectEditor::isSampleObject(const RenderableEditableObject &object) const { return &object == newObj.get(); }

void ECSObjectEditor::render()
{
  ::begin_draw_cached_debug_lines(false, false);
  ObjectEditor::render();
  if (newObj)
    newObj->render();
  ::end_draw_cached_debug_lines();
}

void ECSObjectEditor::hideSelectedTemplate()
{
  if (selection.empty())
  {
    if (hiddenTemplates.empty())
      return;

    auto &hideTemplates = hiddenTemplates;
    forEachEditableObjectWithTemplate([&hideTemplates](ECSEntityObject *o, const char *templName) {
      eastl::string tplName = templName;
      removeSelectedTemplateName(tplName);
      for (const auto &hideTempl : hideTemplates)
        if (hideTempl == tplName)
        {
          o->hideObject(true);
          break;
        }
    });

    hiddenTemplates.clear();
    return;
  }

  hiddenTemplates.clear();
  auto &hideTemplates = hiddenTemplates;
  forEachEditableObjectWithTemplate([&hideTemplates](ECSEntityObject *o, const char *templName) {
    if (o->isSelected())
    {
      o->hideObject(true);

      eastl::string tplName = templName;
      removeSelectedTemplateName(tplName);
      hideTemplates.push_back(eastl::move(tplName));
    }
  });
}

void ECSObjectEditor::unhideAll()
{
  bool showAll = !showOnlySceneEntities;
  forEachEditableObject([&showAll](ECSEntityObject *o) {
    if (showAll || is_scene_entity(o))
      o->hideObject(false);
  });

  hiddenTemplates.clear();
}

int ECSObjectEditor::saveObjectsInternal(const char *fpath, ecs::Scene::SceneId scene_id, bool save_children)
{
  if (!fpath || !*fpath)
    return -1;

  const ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesConstPtrList &importsRecords = saveScene.getScenes(ecs::Scene::LoadType::IMPORT);
  const ecs::Scene::SceneRecord *record = saveScene.getSceneRecordById(scene_id);

  if (!record)
  {
    return -1;
  }

  int savedEntityCount = 0;
  if (record->isDirty)
  {
    DataBlock blk;
    savedEntityCount += writeSceneToBlk(blk, *record, *this);

    String importPathBuffer;
    fpath = ecs::resolve_import_path(fpath, importPathBuffer);

    if (!blk.saveToTextFile(fpath))
    {
      return -1;
    }
  }

  if (save_children)
  {
    for (uint32_t i = 0; i < importsRecords.size(); ++i)
    {
      const ecs::Scene::SceneRecord *childRecord = importsRecords.at(i);
      if (scene_id == childRecord->parent)
      {
        savedEntityCount += saveObjectsInternal(childRecord->path.c_str(), childRecord->id, true);
      }
    }
  }

  return savedEntityCount;
}

void ECSObjectEditor::saveObjects(const char *new_fpath)
{
  if (!hasUnsavedChanges())
    return;

  int numObjects = -1;
  const ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesConstPtrList &importsRecords = saveScene.getScenes(ecs::Scene::LoadType::IMPORT);
  if (const auto it = eastl::find_if(importsRecords.begin(), importsRecords.end(), [](auto &&val) { return val->importDepth == 0; });
      it != importsRecords.end())
  {
    numObjects = saveObjectsInternal(new_fpath, (*it)->id, true);
  }
  else
  {
    DAEDITOR3.conError("Failed to save scene '%s'. Main import was not found", new_fpath);
    return;
  }

  if (numObjects < 0)
  {
    DAEDITOR3.conError("Saving scene to '%s' has failed.", new_fpath);
    return;
  }

  ecs::g_scenes->getActiveScene().setAllChangesWereSaved();
}

void ECSObjectEditor::load(const char *blk_path)
{
  reset();
  DataBlock sceneBlk;
  dblk::load(sceneBlk, blk_path, dblk::ReadFlag::ROBUST);

  ecs::g_scenes.demandInit();
  ecs::SceneManager::loadScene(ecs::g_scenes.get(), sceneBlk, blk_path, &ecs::SceneManager::entityCreationCounter, 0,
    ecs::Scene::IMPORT);

  g_entity_mgr->broadcastEvent(EventRendinstsLoaded());
  g_entity_mgr->tick(true);
  refreshEids();
  ecs::g_scenes->getActiveScene().setTargetSceneId(ecs::Scene::C_INVALID_SCENE_ID);
}

bool ECSObjectEditor::hasUnsavedChanges() const { return ecs::g_scenes->getActiveScene().hasUnsavedChanges(); }

void ECSObjectEditor::onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags)
{
  ObjectEditor::onObjectFlagsChange(obj, changed_flags);
  if (changed_flags & RenderableEditableObject::FLG_SELECTED)
  {
    if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(obj))
    {
      if (!o->isValid())
        return;

      if (!o->isSelected())
        remove_sub_template_async(o->getEid(), ECS_EDITOR_SELECTED_TEMPLATE);
      else
        add_sub_template_async(o->getEid(), ECS_EDITOR_SELECTED_TEMPLATE);
    }
  }
}

void ECSObjectEditor::selectionChanged()
{
  ObjectEditor::selectionChanged();

  // Let's start with single active entity for now
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  if (!selection.empty())
    if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(selection[0]))
      if (o->isValid())
        eid = o->getEid();

  if (sceneOutliner)
  {
    sceneOutliner->syncSelection();
  }
}

void ECSObjectEditor::update(float dt)
{
  ObjectEditor::update(dt);
  if (objsToRemove.size())
  {
    removeObjects(objsToRemove.data(), objsToRemove.size(), false);
    objsToRemove.clear();
  }

  if (newObj.get() && !newObj->isValid() && is_create_entity_mode(getEditMode()))
    cancelCreateMode();

  if (lastCreatedObj.get())
  {
    const float MOVED_TOO_FAR_DISTANCE = 3.0f;
    lastCreateObjFrames -= 1;
    if (lastCreateObjFrames < 0)
      lastCreatedObj = nullptr;
    else if ((lastCreatedObj->getPos() - lastCreatedObjPos).length() > MOVED_TOO_FAR_DISTANCE)
    {
      lastCreatedObjPos = lastCreatedObj->getPos();
      BBox3 box(lastCreatedObjPos, 1.0f);
      if (IGenViewportWnd *viewport = IEditorCoreEngine::get()->getCurrentViewport())
        viewport->zoomAndCenter(box);
      setEditMode(CM_OBJED_MODE_SELECT);

      if (ECSEntityObject *movedObj = RTTI_cast<ECSEntityObject>(lastCreatedObj.get()))
        saveComponent(movedObj->getEid(), "transform");
    }
  }

  ecs::g_scenes->recalculateWbbs();
}

void ECSObjectEditor::fillDeleteDepsObjects(Tab<RenderableEditableObject *> &list) const
{
  const uint32_t originalSize = list.size();
  EditorToDeleteEntitiesEvent evt{{}};
  for (uint32_t i = 0; i < originalSize; ++i)
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(list[i]))
      g_entity_mgr->sendEventImmediate(entityObj->getEid(), evt);

  const ecs::EidList &linkedEntitiesToDelete = evt.get<0>();
  list.reserve(originalSize + linkedEntitiesToDelete.size());
  for (const ecs::EntityId &eid : linkedEntitiesToDelete)
    if (RenderableEditableObject *object = getObjectFromEntityId(eid))
      list.emplace_back(object);
  list.erase(eastl::unique(list.begin(), list.end()), list.end());
}

void ECSObjectEditor::deleteSelectedObjects(bool use_undo)
{
  Tab<RenderableEditableObject *> list(tmpmem);
  list.reserve(selection.size());

  getUndoSystem()->begin();

  for (int i = 0; i < selection.size(); ++i)
    if (selection[i]->mayDelete())
      list.push_back(selection[i]);

  if (!list.size())
  {
    getUndoSystem()->cancel();
    return;
  }

  fillDeleteDepsObjects(list);
  removeObjects(&list[0], list.size(), use_undo);
  getUndoSystem()->accept("Delete");
}

void ECSObjectEditor::cloneSelection()
{
  PtrTab<RenderableEditableObject> oldSelection(selection);
  unselectAll();

  for (RenderableEditableObject *renderableEditableObject : oldSelection)
  {
    ECSEntityObject *editableObject = RTTI_cast<ECSEntityObject>(renderableEditableObject);
    G_ASSERT(editableObject);

    ECSEntityObject *cloneObject = editableObject->cloneObject();
    addObject(cloneObject);
    cloneObject->selectObject();

    cloneObjs.push_back(cloneObject);
    eidsCreated.insert(cloneObject->getEid());
  }
}

bool ECSObjectEditor::checkSceneEntities(const char *rule)
{
  if (!rule)
    return false;

  if (!orderedTemplatesGroups)
    orderedTemplatesGroups.reset(new ECSOrderedTemplatesGroups());

  bool result = false;
  forEachEditableObject([this, &result, &rule](ECSEntityObject *o) {
    if (!is_scene_entity(o))
      return;
    const char *templName = g_entity_mgr->getEntityTemplateName(o->getEid());
    if (!templName)
      return;
    const ecs::Template *pTemplate = g_entity_mgr->getTemplateDB().getTemplateByName(templName);
    if (!pTemplate)
      return;
    ECSOrderedTemplatesGroups::TemplatesGroup tmp;
    if (orderedTemplatesGroups->testEcsTemplateCondition(*pTemplate, rule, tmp, 1))
      result = true;
  });
  return result;
}

int ECSObjectEditor::getEntityRecordLoadType(ecs::EntityId eid)
{
  auto *record = ecs::g_scenes->getActiveScene().findEntityRecord(eid);
  if (record)
    return record->loadType;

  return ecs::Scene::UNKNOWN;
}

int ECSObjectEditor::getEntityRecordSceneId(ecs::EntityId eid)
{
  auto *record = ecs::g_scenes->getActiveScene().findEntityRecord(eid);
  if (record)
    return record->sceneId;

  return -1;
}

ecs::EntityId ECSObjectEditor::reCreateEditorEntity(ecs::EntityId eid)
{
  ECSEntityObject *obj = nullptr;
  forEachEditableObject([&eid, &obj](ECSEntityObject *o) {
    if (o->getEid() == eid)
      obj = o;
  });

  if (!obj)
    return ecs::INVALID_ENTITY_ID;

  unmapEidFromEditableObject(obj);
  const bool was_selected = obj->isSelected();
  obj->recreateObject(this);
  if (obj->getEid() != ecs::INVALID_ENTITY_ID)
  {
    eidsCreated.insert(obj->getEid());
    mapEidToEditableObject(obj);

    if (sceneOutliner)
    {
      sceneOutliner->onEntityAdded(eid);
    }
  }
  if (was_selected)
    obj->selectObject();
  return obj->getEid();
}

ecs::EntityId ECSObjectEditor::makeSingletonEntity(const char *tplName)
{
  if (!tplName || !tplName[0])
    return ecs::INVALID_ENTITY_ID;

  const int tplNameLen = strlen(tplName);
  ecs::EntityId foundEid = ecs::INVALID_ENTITY_ID;
  forEachEditableObject([&tplName, &tplNameLen, &foundEid](ECSEntityObject *o) {
    if (!is_scene_entity(o))
      return;
    const char *templName = g_entity_mgr->getEntityTemplateName(o->getEid());
    if (templName && strncmp(templName, tplName, tplNameLen) == 0)
      foundEid = o->getEid();
  });
  if (foundEid != ecs::INVALID_ENTITY_ID)
    return foundEid;

  if (!g_entity_mgr->getTemplateDB().getTemplateByName(tplName))
    return ecs::INVALID_ENTITY_ID;

  ECSEntityCreateData cd(tplName);
  ecs::EntityId objEid = createEntityDirect(&cd);
  if (objEid == ecs::INVALID_ENTITY_ID)
    return ecs::INVALID_ENTITY_ID;

  ECSEntityObject *obj = new ECSEntityObject(objEid);
  addObject(obj, false);
  eidsCreated.insert(obj->getEid());
  return obj->getEid();
}

void ECSObjectEditor::mapEidToEditableObject(ECSEntityObject *obj)
{
  if (obj == nullptr || newObj == obj || obj->getEid() == ecs::INVALID_ENTITY_ID)
    return;

  const auto inserted = eidToEditableObject.emplace(obj->getEid(), obj);
  if (!inserted.second)
    logerr("[%s] Entity with eid <%i> already exist in eidToEditableObject.", __FUNCTION__, (ecs::entity_id_t)obj->getEid());
}

void ECSObjectEditor::unmapEidFromEditableObject(ECSEntityObject *obj)
{
  if (obj == nullptr || newObj == obj || obj->getEid() == ecs::INVALID_ENTITY_ID)
    return;

  const ska::hash_size_t result = eidToEditableObject.erase(obj->getEid());
  if (result == 0)
    logerr("[%s] Entity with eid <%i> not exist in eidToEditableObject.", __FUNCTION__, (ecs::entity_id_t)obj->getEid());
}

void ECSObjectEditor::mapSidToEditableObject(ECSSceneObject *obj)
{
  if (obj == nullptr || obj->getSceneId() == ecs::Scene::C_INVALID_SCENE_ID)
    return;

  const auto inserted = sidToEditableObject.emplace(obj->getSceneId(), obj);
  if (!inserted.second)
    logerr("[%s] Scene with sid <%i> already exist in sidToEditableObject.", __FUNCTION__, obj->getSceneId());
}

void ECSObjectEditor::unmapSidToEditableObject(ECSSceneObject *obj)
{
  if (obj == nullptr || obj->getSceneId() == ecs::Scene::C_INVALID_SCENE_ID)
    return;

  const ska::hash_size_t result = sidToEditableObject.erase(obj->getSceneId());
  if (result == 0)
    logerr("[%s] Scene with sid <%i> not exist in sidToEditableObject.", __FUNCTION__, obj->getSceneId());
}

void ECSObjectEditor::_addObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  ObjectEditor::_addObjects(obj, num, use_undo);
  for (int i = 0; i < num; ++i)
  {
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(obj[i]))
    {
      mapEidToEditableObject(entityObj);
      if (sceneOutliner)
      {
        sceneOutliner->onEntityAdded(entityObj->getEid());
      }
    }
    else if (ECSSceneObject *sceneObj = RTTI_cast<ECSSceneObject>(obj[i]))
    {
      mapSidToEditableObject(sceneObj);
      if (sceneOutliner)
      {
        sceneOutliner->onSceneAdded(sceneObj->getSceneId());
      }
    }
  }
}

void ECSObjectEditor::_removeObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  for (int i = 0; i < num; ++i)
  {
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(obj[i]))
    {
      if (sceneOutliner)
      {
        sceneOutliner->onEntityRemoved(entityObj->getEid());
      }
      unmapEidFromEditableObject(entityObj);
    }
    else if (ECSSceneObject *sceneObj = RTTI_cast<ECSSceneObject>(obj[i]))
    {
      if (sceneOutliner)
      {
        sceneOutliner->onSceneRemoved(sceneObj->getSceneId());
      }
      unmapSidToEditableObject(sceneObj);
    }
  }
  ObjectEditor::_removeObjects(obj, num, use_undo);
}

void ECSObjectEditor::updateSingleHierarchyTransform(RenderableEditableObject &object, bool calculate_from_relative_transform)
{
  ECSEntityObject *entObj = RTTI_cast<ECSEntityObject>(&object);
  if (!entObj)
  {
    return;
  }

  const ecs::EntityId eid = entObj->getEid();
  TMatrix *transform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("transform"));
  if (!transform)
    return;

  TMatrix *hierarchyTransform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("hierarchy_transform"));
  if (!hierarchyTransform)
    return;

  TMatrix *hierarchyParentLastTransform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("hierarchy_parent_last_transform"));
  if (!hierarchyParentLastTransform)
    return;

  const ECSEntityObject *parentObject = entObj->getParentObject();
  const TMatrix parentTm = parentObject ? parentObject->getWtm() : TMatrix::IDENT;

  if (calculate_from_relative_transform)
  {
    TMatrix newTransform = parentTm * *hierarchyTransform;
    object.setWtm(newTransform);
    *transform = newTransform; // Needed because setWtm() only sets the transform in ECS if the object is selected.
  }
  else
  {
    if (parentObject)
      *hierarchyTransform = inverse(parentTm) * object.getWtm();

    *transform = object.getWtm(); // Needed because setWtm() only sets the transform in ECS if the object is selected.
  }

  if (parentObject)
    *hierarchyParentLastTransform = parentTm;
}

ECSObjectEditor::HierarchyItem *ECSObjectEditor::makeHierarchyItemParent(RenderableEditableObject &object,
  HierarchyItem &hierarchy_root, ska::flat_hash_map<EditableObject *, HierarchyItem *> &object_to_hierarchy_item_map)
{
  auto it = object_to_hierarchy_item_map.find(&object);
  if (it != object_to_hierarchy_item_map.end())
    return it->second;

  HierarchyItem *hierarchyItemParent = &hierarchy_root;
  if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(&object))
  {
    if (ECSEntityObject *objectParent = entityObj->getParentObject())
    {
      hierarchyItemParent = makeHierarchyItemParent(*objectParent, hierarchy_root, object_to_hierarchy_item_map);
    }
  }

  HierarchyItem *hierarchyItem = hierarchyItemParent->addChild(object);
  auto result = object_to_hierarchy_item_map.insert({&object, hierarchyItem});
  return result.first->second;
}

void ECSObjectEditor::fillObjectHierarchy(HierarchyItem &hierarchy_root)
{
  G_ASSERT(hierarchy_root.isRoot());

  ska::flat_hash_map<EditableObject *, HierarchyItem *> objectToHierarchyItemMap;

  for (EditableObject *editableObject : objects)
  {
    G_ASSERT(editableObject);
    if (RenderableEditableObject *rendObj = RTTI_cast<RenderableEditableObject>(editableObject))
    {
      makeHierarchyItemParent(*rendObj, hierarchy_root, objectToHierarchyItemMap);
    }
  }
}

void ECSObjectEditor::applyChange(RenderableEditableObject &object, const Point3 &delta)
{
  switch (getEditMode())
  {
    case CM_OBJED_MODE_MOVE: object.moveObject(delta, IEditorCoreEngine::get()->getGizmoBasisType()); break;
    case CM_OBJED_MODE_SURF_MOVE: object.moveSurfObject(delta, IEditorCoreEngine::get()->getGizmoBasisType()); break;
    case CM_OBJED_MODE_ROTATE: object.rotateObject(-delta, gizmoOrigin, IEditorCoreEngine::get()->getGizmoBasisType()); break;
    case CM_OBJED_MODE_SCALE: object.scaleObject(delta, gizmoOrigin, IEditorCoreEngine::get()->getGizmoBasisType()); break;
  }
}

void ECSObjectEditor::applyChangeHierarchically(const HierarchyItem &parent, const Point3 &delta, bool parent_updated)
{
  for (const HierarchyItem *child : parent.children)
  {
    if (child->object->isSelected() && can_transform_object_freely(child->object))
    {
      applyChange(*child->object, delta);
      updateSingleHierarchyTransform(*child->object, false);
      applyChangeHierarchically(*child, delta, true);
    }
    else
    {
      if (parent_updated && can_transform_object_freely(child->object))
        updateSingleHierarchyTransform(*child->object, true);

      applyChangeHierarchically(*child, delta, parent_updated);
    }
  }
}

void ECSObjectEditor::makeTransformUndoForHierarchySelection(const HierarchyItem &parent, bool parent_updated)
{
  for (const HierarchyItem *child : parent.children)
  {
    if (child->object->isSelected() && can_transform_object_freely(child->object))
    {
      hierarchicalUndoGroup->addDirectlyChangedObject(*child->object);
      makeTransformUndoForHierarchySelection(*child, true);
    }
    else
    {
      if (parent_updated)
      {
        if (can_transform_object_freely(child->object))
          hierarchicalUndoGroup->addDirectlyChangedObject(*child->object);
        else
          hierarchicalUndoGroup->addIndirectlyChangedObject(*child->object);
      }

      makeTransformUndoForHierarchySelection(*child, parent_updated);
    }
  }
}

bool ECSObjectEditor::getAxes(Point3 &ax, Point3 &ay, Point3 &az)
{
  IEditorCoreEngine::BasisType basis = IEditorCoreEngine::get()->getGizmoBasisType();

  if (basis == IEditorCoreEngine::BASIS_Parent && selection.size() > 0)
  {
    if (ECSEntityObject *entityObj = RTTI_cast<ECSEntityObject>(selection[0]))
      if (ECSEntityObject *parentObject = entityObj->getParentObject())
      {
        const TMatrix tm = parentObject->getWtm();
        ax = normalize(tm.getcol(0));
        ay = normalize(tm.getcol(1));
        az = normalize(tm.getcol(2));
        return true;
      }

    basis = IEditorCoreEngine::BASIS_Local;
  }

  if (basis == IEditorCoreEngine::BASIS_Local && selection.size() > 0)
  {
    const TMatrix tm = selection[0]->getWtm();
    ax = normalize(tm.getcol(0));
    ay = normalize(tm.getcol(1));
    az = normalize(tm.getcol(2));
    return true;
  }

  ax = Point3(1, 0, 0);
  ay = Point3(0, 1, 0);
  az = Point3(0, 0, 1);
  return true;
}

void ECSObjectEditor::changed(const Point3 &delta)
{
  // Work from the initial state because delta is total delta since startGizmo().
  G_ASSERT(hierarchicalUndoGroup);
  hierarchicalUndoGroup->undo();
  hierarchicalUndoGroup->clear();
  makeTransformUndoForHierarchySelection(temporaryHierarchyForGizmoChange);

  IEditorCoreEngine::BasisType basis = IEditorCoreEngine::get()->getGizmoBasisType();

  switch (getEditMode())
  {
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE:
      applyChangeHierarchically(temporaryHierarchyForGizmoChange, delta);
      updateGizmo(basis);
      break;

    case CM_OBJED_MODE_ROTATE:
      applyChangeHierarchically(temporaryHierarchyForGizmoChange, delta);
      gizmoRot = gizmoRotO + delta;
      break;

    case CM_OBJED_MODE_SCALE:
      applyChangeHierarchically(temporaryHierarchyForGizmoChange, delta);
      gizmoScl = gizmoSclO + delta;
      break;
  }
}

void ECSObjectEditor::gizmoStarted()
{
  gizmoOrigin = gizmoPt;
  isGizmoStarted = true;
  temporaryHierarchyForGizmoChange.clearChildren();
  hierarchicalUndoGroup.reset(new ECSHierarchicalUndoGroup());

  if ((ImGui::GetIO().KeyMods & ImGuiMod_Shift) != 0 && getEditMode() == CM_OBJED_MODE_MOVE)
  {
    cloneMode = true;
    cloneStartPosition = getPt();
  }

  getUndoSystem()->begin();
  if (cloneMode)
  {
    clear_and_shrink(cloneObjs);
    cloneSelection();
    fillObjectHierarchy(temporaryHierarchyForGizmoChange);
    makeTransformUndoForHierarchySelection(temporaryHierarchyForGizmoChange);
  }
  else
  {
    fillObjectHierarchy(temporaryHierarchyForGizmoChange);

    switch (getEditMode())
    {
      case CM_OBJED_MODE_MOVE:
      case CM_OBJED_MODE_ROTATE:
      case CM_OBJED_MODE_SCALE: makeTransformUndoForHierarchySelection(temporaryHierarchyForGizmoChange); break;

      case CM_OBJED_MODE_SURF_MOVE:
        makeTransformUndoForHierarchySelection(temporaryHierarchyForGizmoChange);
        for (int i = 0; i < selection.size(); ++i)
          selection[i]->rememberSurfaceDist();
        break;
    }
  }
}

void ECSObjectEditor::gizmoEnded(bool apply)
{
  // Because the non-root objects are moved by their parents we have to save their transforms after the hierarchy
  // ECS events ran.
  if (apply)
    hierarchicalUndoGroup->saveTransformComponentOfAllObjects();

  getUndoSystem()->put(hierarchicalUndoGroup.detach());

  if (!cloneMode)
  {
    ObjectEditor::gizmoEnded(apply);
    return;
  }

  int cloneCount = 1;
  if (apply && selection.size() > 0 && ECSEditorCopyDlg::show(cloneCount))
  {
    const Point3 delta = getPt() - cloneStartPosition;

    for (int cloneIndex = 2; cloneIndex <= cloneCount; ++cloneIndex)
    {
      cloneSelection();
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->moveObject(delta, IEditorCoreEngine::BASIS_World);
    }

    getUndoSystem()->accept("Clone object(s)");
  }
  else
  {
    getUndoSystem()->cancel();
  }

  cloneMode = false;
  isGizmoStarted = false;
  updateGizmo();
}

int ECSObjectEditor::getAvailableTypes()
{
  int types = ObjectEditor::getAvailableTypes();
  if (types & IEditorCoreEngine::BASIS_Local)
    types |= IEditorCoreEngine::BASIS_Parent;
  return types;
}

void ECSObjectEditor::onTemplateSelectorTemplateSelected(const char *template_name) { selectNewObjEntity(template_name); }

void *ECSObjectEditor::onWmCreateWindow(int type)
{
  switch (type)
  {
    case SCENE_OUTLINER_EDITOR_WTYPE:
    {
      if (sceneOutliner)
      {
        return nullptr;
      }

      sceneOutliner = eastl::make_unique<ECSSceneOutlinerPanel>(*this, *this, nullptr, "Scene Outliner");
      sceneOutliner->fillPanel([this](const eastl::function<void(ecs::EntityId eid)> &cb) { onOutlinerPanelFill(cb); });

      setButton(CM_ECS_EDITOR_SCENE_OUTLINER, true);
      return sceneOutliner.get();
    }
    break;
  }

  return Base::onWmCreateWindow(type);
}

bool ECSObjectEditor::onWmDestroyWindow(void *window)
{
  if (window == sceneOutliner.get())
  {
    sceneOutliner.reset();
    setButton(CM_ECS_EDITOR_SCENE_OUTLINER, false);
    return true;
  }

  return Base::onWmDestroyWindow(window);
}

bool ECSObjectEditor::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (getEditMode() == CM_ECS_EDITOR_SET_PARENT)
  {
    Tab<RenderableEditableObject *> objs(tmpmem);
    if (pickObjects(wnd, x, y, objs))
    {
      ECSEntityObject *parentObject = RTTI_cast<ECSEntityObject>(objs[0]);
      G_ASSERT(parentObject);

      dag::Vector<ECSEntityObject *> ecsObjectsForParenting;
      for (ecs::EntityId eid : objectsForParenting)
      {
        ECSEntityObject *ecsEntityObject = RTTI_cast<ECSEntityObject>(getObjectFromEntityId(eid));
        if (ecsEntityObject && ecsEntityObject != parentObject)
          ecsObjectsForParenting.push_back(ecsEntityObject);
      }

      if (setParentForObjects(ecsObjectsForParenting, *parentObject))
      {
        invalidateObjectProps();
        setEditMode(CM_OBJED_MODE_SELECT);
        ecs_editor_set_viewport_message("Parent object set", /*auto_hide = */ true);
      }
    }

    return true;
  }

  return ObjectEditor::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}

bool ECSObjectEditor::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  // In War Thunder perInstDataDwords is not enabled so is_rendinst_marked_collision_ignored does not do anything.
  // To prevent hitting the object that is being placed when tracing rays we move away the object first, do the tracing,
  // and then move back the object.
  if (is_create_entity_mode(getEditMode()) && newObj)
  {
    ECSEntityObject *object = RTTI_cast<ECSEntityObject>(newObj);
    G_ASSERT(object);
    const TMatrix originalWtm = object->getWtm();
    const bool ignoreSucceeded = ecs_editor_ignore_entity_in_collision_test_begin(object->getEid());

    const bool result = ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);

    if (ignoreSucceeded)
      ecs_editor_ignore_entity_in_collision_test_end(object->getEid(), originalWtm);

    return result;
  }
  else if (getEditMode() == CM_ECS_EDITOR_SET_PARENT)
  {
    Tab<RenderableEditableObject *> pickedObjects(tmpmem);
    if (pickObjects(wnd, x, y, pickedObjects))
    {
      ECSEntityObject *parentObject = RTTI_cast<ECSEntityObject>(pickedObjects[0]);
      G_ASSERT(parentObject);
      parentSelectionHoveredObject = parentObject;
    }
    else
      parentSelectionHoveredObject = nullptr;
  }

  return ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}

void ECSObjectEditor::onBeforeExport() { ecs_editor_ignore_entities_for_export_begin(); }

void ECSObjectEditor::onAfterExport() { ecs_editor_ignore_entities_for_export_end(); }

void ECSObjectEditor::onOutlinerPanelFill(const eastl::function<void(ecs::EntityId eid)> &cb)
{
  for (EditableObject *eo : objects)
  {
    if (ECSEntityObject *o = RTTI_cast<ECSEntityObject>(eo))
    {
      if (!ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid()))
      {
        cb(o->getEid());
      }
    }
  }
}
