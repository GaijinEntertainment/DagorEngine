// required externals:
//   void select_entity_webui(const ecs::EntityId &eid)

#include "entityEditor.h"
#include "entityObj.h"
#include "sceneObj.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/unordered_map.h>
#include <generic/dag_sort.h>
#include <ecs/scripts/sqEntity.h>
#include <stddef.h>
#include <../../tools/sharedInclude/libTools/util/strUtil.h>
#include <daEditorE/de_objCreator.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <util/dag_convar.h>
#include <daECS/core/utility/ecsRecreate.h>
#include <daECS/core/utility/ecsBlkUtils.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstExtra.h>

#include <debug/dag_debug3d.h>
#include <gamePhys/collision/collisionLib.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <json/json.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <gui/dag_visualLog.h>

CONSOLE_BOOL_VAL("daEd4", pick_rendinst, false);

#define SELECTED_TEMPLATE                   "daeditor_selected"
#define PREVIEW_TEMPLATE                    "daeditor_preview_entity"
#define HIERARCHIAL_FREE_TRANSFORM_TEMPLATE "hierarchial_free_transform"

namespace
{
void removeSelectedTemplateName(eastl::string &templ_name)
{
  templ_name = remove_sub_template_name(templ_name.c_str(), SELECTED_TEMPLATE);
}

const ecs::HashedConstString EDITABLE_TEMPLATE_COMP = ECS_HASH("editableTemplate");

inline bool is_entity_editable(ecs::EntityId eid) { return g_entity_mgr->getOr(eid, ECS_HASH("editableObj"), true); }

inline bool is_scene_entity(EntityObj *o)
{
  if (!o)
    return false;
  const auto *erec = ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid());
  return erec;
}

inline bool can_transform_object_freely(EditableObject *object)
{
  if (EntityObj *entityObj = RTTI_cast<EntityObj>(object))
    return entityObj->canTransformFreely();

  return true;
}

// Check if setting the child's parent to parent would result in a loop.
bool is_safe_to_set_parent(EntityObj &child, EntityObj &parent)
{
  EntityObj *loopObject = &child;
  while (true)
  {
    EntityObj *newParent = loopObject == &child ? &parent : loopObject->getParentObject();
    if (!newParent)
      return true;
    if (newParent == &child)
      return false;

    loopObject = newParent;
  }
}

const char *check_singleton_mutex_exists(const ecs::Template *templ, const ecs::TemplatesData &templates_data)
{
  auto &c = templ->getComponent(ECS_HASH("singleton_mutex"), templates_data);
  if (!c.isNull())
    if (EntityObjEditor::checkSingletonMutexExists(c.getOr("")))
      return c.getOr("");
  return nullptr;
}

class UndoDelTemplate : public UndoRedoObject
{
private:
  eastl::string removedTemplate_;
  EntityObj *entityObj_;

public:
  ecs::ComponentsList componentList;

  UndoDelTemplate(const char *removedTemplate, EntityObj *entityObj) : removedTemplate_(removedTemplate), entityObj_(entityObj) {}

  virtual void restore(bool /*save_redo*/)
  {
    const ecs::EntityId eid = entityObj_->getEid();
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
          EntityObjEditor::saveComponent(eid, comp.c_str());
      });
    EntityObjEditor::saveAddTemplate(eid, removedTemplate_.c_str());
  }

  virtual void redo()
  {
    const ecs::EntityId eid = entityObj_->getEid();
    remove_sub_template_async(eid, removedTemplate_.c_str());
    EntityObjEditor::saveDelTemplate(eid, removedTemplate_.c_str(), false /*use_undo*/);
  }

  virtual size_t size() { return sizeof(*this) + removedTemplate_.capacity() + componentList.capacity(); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoRedoObject"; }
};

class SceneNameUndoRedo : public UndoRedoObject
{
public:
  explicit SceneNameUndoRedo(ecs::Scene::SceneId scene_id, SimpleString new_name) noexcept :
    sceneId(scene_id), newName(std::move(new_name))
  {
    lastName = ecs::g_scenes->getScenePrettyName(sceneId);
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setScenePrettyName(sceneId, lastName);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setScenePrettyName(sceneId, newName);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SceneNameUndoRedo"; }

private:
  ecs::Scene::SceneId sceneId;
  SimpleString newName;
  SimpleString lastName;
};

class ScenePivotUndoRedo : public UndoRedoObject
{
public:
  explicit ScenePivotUndoRedo(ecs::Scene::SceneId scene_id, Point3 new_pivot) noexcept : sceneId(scene_id), newPivot(new_pivot)
  {
    lastPivot = ecs::g_scenes->getScenePivot(sceneId);
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setScenePivot(sceneId, lastPivot);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setScenePivot(sceneId, newPivot);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "ScenePivotUndoRedo"; }

private:
  ecs::Scene::SceneId sceneId;
  Point3 newPivot;
  Point3 lastPivot;
};

class SceneTransformableUndoRedo : public UndoRedoObject
{
public:
  explicit SceneTransformableUndoRedo(ecs::Scene::SceneId scene_id) noexcept : sceneId(scene_id)
  {
    lastValue = ecs::g_scenes->isSceneTransformable(sceneId);
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setSceneTransformable(sceneId, lastValue);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setSceneTransformable(sceneId, !lastValue);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SceneTransformableUndoRedo"; }

private:
  ecs::Scene::SceneId sceneId;
  bool lastValue;
};

struct SceneItemSaveObj
{
  ecs::Scene::SceneRecord::OrderedEntry entry{};
  const ecs::Scene::EntityRecord *erec = nullptr;
  uint64_t position = 0;
  uint64_t priority = 0;
  bool operator<(const SceneItemSaveObj &rhs) const
  {
    if (priority != rhs.priority)
      return priority < rhs.priority;
    return position < rhs.position;
  }
};

uint64_t calc_save_priority(const char *template_name, const EntityObjEditor::SaveOrderRules &rules)
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

bool saveSceneToBlk(const char *fpath, const ecs::Scene::SceneRecord &record, EntityObjEditor &editor)
{
  DataBlock blk;
  EntityObjEditor::writeSceneToBlk(blk, record, editor);

  if (dd_file_exist(fpath))
  {
    fpath = df_get_abs_fname(fpath);
  }

  return blk.saveToTextFile(fpath);
}

class SceneAddUndoRedo : public UndoRedoObject
{
public:
  using RemoveEntityCallback = eastl::function<void(ecs::EntityId)>;

  explicit SceneAddUndoRedo(ecs::Scene::SceneId scene_id, EntityObjEditor &editor) noexcept : sceneId(scene_id), editor(editor)
  {
    if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(scene_id))
    {
      parentId = record->parent;
      path = record->path;
      order = ecs::g_scenes->getSceneOrder(scene_id);
    }
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    editor.removeScene(sceneId);

    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    DataBlock block(path.c_str());
    if (block.isValid())
    {
      const ecs::Scene::SceneId newId = ecs::g_scenes->addImportScene(parentId, block, path.c_str());
      if (newId != ecs::Scene::C_INVALID_SCENE_ID)
      {
        if (order != ecs::Scene::C_INVALID_SCENE_ID)
        {
          ecs::g_scenes->setSceneOrder(newId, order);
        }
      }
    }
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SceneAddUndoRedo"; }

private:
  const ecs::Scene::SceneId sceneId;
  ecs::Scene::SceneId parentId = ecs::Scene::C_INVALID_SCENE_ID;
  eastl::string path;
  uint32_t order = ecs::Scene::C_INVALID_SCENE_ID;
  EntityObjEditor &editor;
};

class SceneOrderUndoRedo : public UndoRedoObject
{
public:
  explicit SceneOrderUndoRedo(ecs::Scene::SceneId scene_id, uint32_t new_order) noexcept : sceneId(scene_id), newOrder(new_order)
  {
    oldOrder = ecs::g_scenes->getSceneOrder(scene_id);
    if (new_order < oldOrder)
    {
      oldOrder += 1;
    }
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setSceneOrder(sceneId, oldOrder);

    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setSceneOrder(sceneId, newOrder);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SceneOrderUndoRedo"; }

private:
  const ecs::Scene::SceneId sceneId;
  const uint32_t newOrder;
  uint32_t oldOrder;
};

class EntityOrderUndoRedo : public UndoRedoObject
{
public:
  explicit EntityOrderUndoRedo(ecs::EntityId eid, uint32_t new_order) noexcept : eid(eid), newOrder(new_order)
  {
    oldOrder = ecs::g_scenes->getEntityOrder(eid);
    if (new_order < oldOrder)
    {
      oldOrder += 1;
    }
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setEntityOrder(eid, oldOrder);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setEntityOrder(eid, newOrder);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "EntityOrderUndoRedo"; }

private:
  const ecs::EntityId eid;
  const uint32_t newOrder;
  uint32_t oldOrder = 0;
};

class SceneSetParentUndoRedo : public UndoRedoObject
{
public:
  explicit SceneSetParentUndoRedo(ecs::Scene::SceneId scene_id, ecs::Scene::SceneId new_parent) noexcept :
    sceneId(scene_id), newParent(new_parent)
  {
    if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(scene_id))
    {
      oldParent = record->parent;
      order = ecs::g_scenes->getSceneOrder(scene_id);
    }
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setNewParent(sceneId, oldParent);
    ecs::g_scenes->setSceneOrder(sceneId, order);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setNewParent(sceneId, newParent);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SceneSetParentUndoRedo"; }

private:
  const ecs::Scene::SceneId sceneId;
  const ecs::Scene::SceneId newParent;
  uint32_t order = 0;
  ecs::Scene::SceneId oldParent = ecs::Scene::C_INVALID_SCENE_ID;
};

class EntitySetParentUndoRedo : public UndoRedoObject
{
public:
  explicit EntitySetParentUndoRedo(ecs::EntityId eid, ecs::Scene::SceneId new_parent) noexcept : eid(eid), newParent(new_parent)
  {
    if (const ecs::Scene::EntityRecord *erecord = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    {
      if (const ecs::Scene::SceneRecord *srecord = ecs::g_scenes->getActiveScene().getSceneRecordById(erecord->sceneId))
      {
        oldParent = srecord->id;
        const auto it = eastl::find_if(srecord->orderedEntries.begin(), srecord->orderedEntries.end(),
          [eid](auto &&val) { return val.eid == eid && val.isEntity; });

        order = eastl::distance(srecord->orderedEntries.begin(), it);
      }
    }
  }

  void restore([[maybe_unused]] bool save_redo_data) override
  {
    ecs::g_scenes->setEntityParent(oldParent, {eid});
    ecs::g_scenes->setEntityOrder(eid, order);
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  void redo() override
  {
    ecs::g_scenes->setEntityParent(newParent, {eid});
    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "EntitySetParentUndoRedo"; }

private:
  const ecs::EntityId eid;
  const ecs::Scene::SceneId newParent;
  uint32_t order = 0;
  ecs::Scene::SceneId oldParent = ecs::Scene::C_INVALID_SCENE_ID;
};

struct SqObjectData
{
  bool isEntity;
  union
  {
    ecs::EntityId eid;
    ecs::Scene::SceneId sid;
  };
  uint32_t loadType;
};

eastl::vector<SqObjectData> parseSqObjectDataArray(const Sqrat::Array &items)
{
  eastl::vector<SqObjectData> result;
  result.reserve(items.GetSize());

  for (uint32_t i = 0; i < items.GetSize(); ++i)
  {
    SqObjectData d{};

    const auto isEntityObj = items[i].GetSlot("isEntity");
    if (isEntityObj.IsNull())
    {
      return {};
    }

    d.isEntity = isEntityObj.GetVar<bool>().value;

    const auto idObj = items[i].GetSlot("id");
    if (idObj.IsNull())
    {
      return {};
    }

    if (d.isEntity)
    {
      d.eid = idObj.GetVar<ecs::EntityId>().value;
    }
    else
    {
      d.sid = idObj.GetVar<ecs::Scene::SceneId>().value;
    }

    if (!d.isEntity)
    {
      const auto loadType = items[i].GetSlot("loadType");
      if (loadType.IsNull())
      {
        return {};
      }

      d.loadType = loadType.GetVar<uint32_t>().value;
    }

    result.emplace_back(std::move(d));
  }

  return result;
}

template <typename T>
void iterateSceneAncestorsAsEdObjects(EntityObjEditor &editor, ecs::Scene::SceneId scene_id, T &&callback)
{
  ecs::Scene::SceneId currentScene = scene_id;
  while (currentScene != ecs::Scene::C_INVALID_SCENE_ID)
  {
    if (EditableObject *eo = editor.getSceneObjectById(currentScene))
    {
      callback(eo);
    }

    ecs::Scene::SceneRecord *srecord = ecs::g_scenes->getActiveScene().getSceneRecordById(currentScene);
    if (!srecord)
    {
      break;
    }

    currentScene = srecord->parent;
  }
}
} // namespace

class HierarchicalUndoGroup : public UndoRedoObject
{
public:
  void clear()
  {
    directlyChangedObjects.clear();
    indirectlyChangedObjects.clear();
  }

  // objects directly changed by the operation (for example they were selected and the gizmo moved them)
  void addDirectlyChangedObject(EditableObject &object)
  {
    UndoRedoData &data = directlyChangedObjects.push_back();
    data.object = &object;
    data.oldData.setFromEntity(object);
    data.redoData = data.oldData;
  }

  // objects indirectly changed by the operation (e.g.: parent moves children)
  void addIndirectlyChangedObject(EditableObject &object) { indirectlyChangedObjects.push_back(&object); }

  void saveTransformComponentOfAllObjects()
  {
    // We need hierarchy_attached_entity_transform_es() to run to be able to get the final transform values.
    g_entity_mgr->tick(true);
    g_entity_mgr->update(ecs::UpdateStageInfoAct(0.0f, 0.0f));

    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      if (EntityObj *entityObj = RTTI_cast<EntityObj>(undoRedoObject.object))
        EntityObjEditor::saveComponent(entityObj->getEid(), "transform");

    for (Ptr<EditableObject> &object : indirectlyChangedObjects)
      if (EntityObj *entityObj = RTTI_cast<EntityObj>(object))
        EntityObjEditor::saveComponent(entityObj->getEid(), "transform");
  }

  void undo()
  {
    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.oldData.applyToEntity(*undoRedoObject.object);
  }

private:
  struct Data
  {
    void setFromEntity(EditableObject &o)
    {
      wtm = o.getWtm();

      if (EntityObj *entityObj = RTTI_cast<EntityObj>(&o))
      {
        const TMatrix *ht = g_entity_mgr->getNullable<TMatrix>(entityObj->getEid(), ECS_HASH("hierarchy_transform"));
        hierarchyTransform = ht ? *ht : TMatrix::IDENT;

        const TMatrix *hplt = g_entity_mgr->getNullable<TMatrix>(entityObj->getEid(), ECS_HASH("hierarchy_parent_last_transform"));
        hierarchyParentLastTransform = hplt ? *hplt : TMatrix::IDENT;
      }
    }

    void applyToEntity(EditableObject &o)
    {
      o.setWtm(wtm);

      if (EntityObj *entityObj = RTTI_cast<EntityObj>(&o))
      {
        TMatrix *t = g_entity_mgr->getNullableRW<TMatrix>(entityObj->getEid(), ECS_HASH("transform"));
        if (t)
          *t = wtm; // Needed because setWtm() only sets the transform in ECS if the object is selected.

        TMatrix *ht = g_entity_mgr->getNullableRW<TMatrix>(entityObj->getEid(), ECS_HASH("hierarchy_transform"));
        if (ht)
          *ht = hierarchyTransform;

        TMatrix *hplt = g_entity_mgr->getNullableRW<TMatrix>(entityObj->getEid(), ECS_HASH("hierarchy_parent_last_transform"));
        if (hplt)
          *hplt = hierarchyParentLastTransform;
      }
    }

    TMatrix wtm;
    TMatrix hierarchyTransform;
    TMatrix hierarchyParentLastTransform;
  };

  struct UndoRedoData
  {
    Ptr<EditableObject> object;
    Data oldData, redoData;
  };

  void restore(bool save_redo) override
  {
    if (save_redo)
      for (UndoRedoData &undoRedoObject : directlyChangedObjects)
        undoRedoObject.redoData.setFromEntity(*undoRedoObject.object);

    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.oldData.applyToEntity(*undoRedoObject.object);

    saveTransformComponentOfAllObjects();
  }

  void redo() override
  {
    for (UndoRedoData &undoRedoObject : directlyChangedObjects)
      undoRedoObject.redoData.applyToEntity(*undoRedoObject.object);

    saveTransformComponentOfAllObjects();
  }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "HierarchicalUndoEnd"; }

  dag::Vector<UndoRedoData> directlyChangedObjects;
  PtrTab<EditableObject> indirectlyChangedObjects;
};

EntCreateData::EntCreateData(ecs::EntityId e, const char *template_name)
{
  if (g_entity_mgr->has(e, ECS_HASH("transform")))
    attrs[ECS_HASH("transform")] = g_entity_mgr->get<TMatrix>(e, ECS_HASH("transform"));
  if (const ecs::Scene::EntityRecord *erec = ecs::g_scenes->getActiveScene().findEntityRecord(e))
  {
    for (auto &comp : erec->clist)
      attrs[ECS_HASH_SLOW(comp.first.c_str())] = ecs::ChildComponent(comp.second);
    templName = erec->templateName;
    scene = erec->sceneId;
    sceneLoadType = erec->loadType;
    orderInScene = ecs::g_scenes->getEntityOrder(e);
  }
  else
  {
    if (template_name && *template_name)
      templName = template_name;
    else
      templName = g_entity_mgr->getEntityTemplateName(e);
  }
}
EntCreateData::EntCreateData(const char *template_name, const TMatrix *tm)
{
  templName = template_name;
  if (tm)
    attrs[ECS_HASH("transform")] = *tm;
}
EntCreateData::EntCreateData(const char *template_name, const TMatrix *tm, const char *riex_name)
{
  templName = template_name;
  if (tm)
    attrs[ECS_HASH("transform")] = *tm;
  if (riex_name)
    attrs[ECS_HASH("ri_extra__name")] = riex_name;
}
void EntCreateData::createCb(EntCreateData *ctx)
{
  ctx->eid = g_entity_mgr->createEntitySync(ctx->templName.c_str(), ecs::ComponentsInitializer(ctx->attrs));
}

template <typename F>
inline void EntityObjEditor::forEachEntityObj(F fn)
{
  for (EditableObject *eo : objects)
    if (EntityObj *o = RTTI_cast<EntityObj>(eo))
      fn(o);
}

template <typename F>
inline void EntityObjEditor::forEachEntityObjWithTemplate(F fn)
{
  for (EditableObject *eo : objects)
    if (EntityObj *o = RTTI_cast<EntityObj>(eo))
      if (const char *entTempl = g_entity_mgr->getEntityTemplateName(o->getEid()))
        fn(o, entTempl);
}

template <typename F>
inline void EntityObjEditor::forEachSceneObj(F fn)
{
  for (EditableObject *eo : objects)
    if (SceneObj *o = RTTI_cast<SceneObj>(eo))
      fn(o);
}

EntityObjEditor::~EntityObjEditor()
{
  setCreateBySampleMode(NULL); //-V1053
  selectNewObjEntity(NULL);
  del_con_proc(this);
  delete hierarchicalUndoGroup;
}

EditableObject *EntityObjEditor::cloneObject(EditableObject *o, bool use_undo) { return ObjectEditor::cloneObject(o, use_undo); }

ecs::EntityId EntityObjEditor::createEntitySample(EntCreateData *data)
{
  data->eid = ecs::INVALID_ENTITY_ID;
  EntCreateData::createCb(data);
  if (data->eid == ecs::INVALID_ENTITY_ID)
    logerr("failed to create sample entity");
  return data->eid;
}

ecs::EntityId EntityObjEditor::createEntityDirect(EntCreateData *data)
{
  data->eid = ecs::INVALID_ENTITY_ID;
  removeSelectedTemplateName(data->templName);
  EntCreateData::createCb(data);
  if (data->eid != ecs::INVALID_ENTITY_ID)
    ecs::g_scenes->getActiveScene().insertEmptyEntityRecord(data->eid, data->templName.c_str());
  else
    logerr("failed to create new entity");
  return data->eid;
}

void EntityObjEditor::destroyEntityDirect(ecs::EntityId eid)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return;
  eidsCreated.erase(eid);
  ecs::g_scenes->getActiveScene().eraseEntityRecord(eid);
  g_entity_mgr->destroyEntity(eid);
}

void EntityObjEditor::addEntityEx(ecs::EntityId eid, bool use_undo)
{
  if (isCreatingSampleEntity)
    return;

  const bool isUndoHolding = getUndoSystem()->is_holding();
  if (use_undo && !isUndoHolding)
    getUndoSystem()->begin();

  eidsCreated.insert(eid);

  String name(0, "%s_%u", g_entity_mgr->getOr(eid, ECS_HASH("animchar__res"), g_entity_mgr->getEntityTemplateName(eid)),
    (ecs::entity_id_t)eid);
  EntityObj *o = new EntityObj(name, eid);
  addObject(o, use_undo);

  if (showOnlySceneEntities && !is_scene_entity(o))
    o->hideObject(true);

  if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(o->getEid()))
  {
    removeSelectedTemplateName(pRec->templateName);
    remove_sub_template_async(o->getEid(), SELECTED_TEMPLATE);
  }

  if (use_undo && !isUndoHolding)
    getUndoSystem()->accept("Add Entity");
}

void EntityObjEditor::selectEntity(ecs::EntityId eid, bool selected)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return;

  forEachEntityObj([&eid, &selected](EntityObj *o) {
    if (o && o->getEid() == eid && o->isSelected() != selected)
      o->selectObject(selected);
  });
  updateSelection();
  updateGizmo();
}

void EntityObjEditor::setFocusedEntity(ecs::EntityId eid)
{
  EntityObj *found = nullptr;
  if (eid != ecs::INVALID_ENTITY_ID)
  {
    forEachEntityObj([&eid, &found](EntityObj *o) {
      if (o && o->getEid() == eid && o->isSelected())
        found = o;
    });
  }
  setSelectionFocus(found);
}

ecs::EntityId EntityObjEditor::getFirstSelectedEntity()
{
  EditableObject *selectedObject = getSelected(0);
  if (selectedObject == nullptr)
    return ecs::INVALID_ENTITY_ID;

  return RTTI_cast<EntityObj>(selectedObject)->getEid();
}

void EntityObjEditor::initSceneObjects()
{
  ecs::Scene &activeScene = ecs::g_scenes->getActiveScene();
  for (const ecs::Scene::SceneRecord &srecord : activeScene.getAllScenes())
  {
    addScene(srecord.id);
  }
}

void EntityObjEditor::addScene(ecs::Scene::SceneId sid)
{
  ecs::Scene &activeScene = ecs::g_scenes->getActiveScene();
  if (const ecs::Scene::SceneRecord *srecord = activeScene.getSceneRecordById(sid); srecord && sceneIdToEObj.count(sid) == 0)
  {
    auto sceneObj = new SceneObj(srecord->id);
    addObject(sceneObj, false);

    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }
}

void EntityObjEditor::removeScene(ecs::Scene::SceneId sid)
{
  if (createdSceneObjs.count(sid) != 0)
  {
    auto it = sceneIdToEObj.find(sid);
    if (it == sceneIdToEObj.cend())
    {
      visuallog::logmsg("daEd4: Failed to remove scene object %d. It does not exist in mapping", sid);
      return;
    }

    EditableObject *sceneObj = it->second;
    removeObject(sceneObj, false);

    sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
  }
}

void EntityObjEditor::setParentForSelection()
{
  if (selection.size() < 2)
    return;

  EntityObj *parentEntityObj = RTTI_cast<EntityObj>(selection[selection.size() - 1]);
  if (!parentEntityObj)
    return;

  for (int i = 0; i < (selection.size() - 1); ++i)
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(selection[i]))
      if (is_safe_to_set_parent(*entityObj, *parentEntityObj))
      {
        ecs::ComponentsInitializer comps;
        comps[ECS_HASH("hierarchy_parent")] = parentEntityObj->getEid();
        add_sub_template_async(entityObj->getEid(), "hierarchial_entity", eastl::move(comps));
        saveAddTemplate(entityObj->getEid(), "hierarchial_entity");
        saveAddTemplate(parentEntityObj->getEid(), "hierarchial_entity");
      }
}

void EntityObjEditor::clearParentForSelection()
{
  for (int i = 0; i < selection.size(); ++i)
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(selection[i]))
    {
      remove_sub_template_async(entityObj->getEid(), "hierarchial_entity");
      saveDelTemplate(entityObj->getEid(), "hierarchial_entity");
    }
}

void EntityObjEditor::toggleFreeTransformForSelection()
{
  if (selection.empty())
    return;

  EntityObj *referenceEntityObj = RTTI_cast<EntityObj>(selection[selection.size() - 1]);
  if (!referenceEntityObj)
    return;

  const char *freeComponentName = "hierarchial_free_transform";
  const char *referenceTemplateName = g_entity_mgr->getEntityTemplateName(referenceEntityObj->getEid());
  const bool referenceIsFree = find_sub_template_name(referenceTemplateName, freeComponentName);

  for (int i = 0; i < selection.size(); ++i)
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(selection[i]))
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

void EntityObjEditor::setWorkMode(const char *mode)
{
  ObjectEditor::setWorkMode(mode);

  const bool mode_Designer = (mode && strcmp(mode, "Designer") == 0);

  const bool show_only_scene = mode_Designer;
  if (showOnlySceneEntities != show_only_scene)
  {
    showOnlySceneEntities = show_only_scene;
    forEachEntityObj([&show_only_scene](EntityObj *o) {
      if (!is_scene_entity(o))
        o->hideObject(show_only_scene);
    });
  }
}

void EntityObjEditor::setEditMode(int cm)
{
  if (getEditMode() == cm)
    return;

  if (getEditMode() == CM_OBJED_MODE_CREATE_ENTITY)
  {
    setCreateBySampleMode(NULL);
    objCreator.reset();
    selectNewObjEntity(NULL);
  }

  ObjectEditor::setEditMode(cm);

  if (getEditMode() == CM_OBJED_MODE_CREATE_ENTITY)
  {
    if (!newObj.get())
    {
      newObj = new EntityObj("new", ecs::INVALID_ENTITY_ID);
      newObjOk = true;
      setCreateBySampleMode(newObj);
    }
  }
  else
  {
    setCreateBySampleMode(NULL);
    objCreator.reset();
    selectNewObjEntity(NULL);
  }
}

void EntityObjEditor::selectNewObjEntity(const char *name)
{
  if (!newObj.get())
    return;

  if (EntityObj *o = RTTI_cast<EntityObj>(newObj))
  {
    TMatrix tm = o->getWtm();
    o->setName(name);
    if (name && *name)
    {
      String previewTemplateName = String(name);
      if (g_entity_mgr->getTemplateDB().getTemplateByName(PREVIEW_TEMPLATE))
        previewTemplateName = String(0, "%s+%s", name, PREVIEW_TEMPLATE);

      const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(previewTemplateName);
      if (templ && templ->isSingleton())
      {
        o->resetEid();
        if (g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(name)) != ecs::INVALID_ENTITY_ID)
        {
          visuallog::logmsg(String(0, "singleton <%s> already created", name), E3DCOLOR(200, 0, 0, 255));
          o->setName(name = nullptr);
        }
        else if (const char *mutex_name = check_singleton_mutex_exists(templ, g_entity_mgr->getTemplateDB().data()))
        {
          visuallog::logmsg(String(0, "entity with singleton_mutex=<%s> already created, <%s> cannot be created", mutex_name, name),
            E3DCOLOR(200, 0, 0, 255));
          o->setName(name = nullptr);
        }
      }
      else
      {
        if (o->getEid() != ecs::INVALID_ENTITY_ID)
          g_entity_mgr->destroyEntity(o->getEid());

        EntCreateData cd(previewTemplateName, &tm);
        isCreatingSampleEntity = true;
        o->replaceEid(createEntitySample(&cd));
        isCreatingSampleEntity = false;
        o->setWtm(tm);

        newObjOk = o->getEid() == ecs::INVALID_ENTITY_ID;
      }
    }
    else
    {
      TMatrix tm = o->getWtm();

      if (o->getEid() != ecs::INVALID_ENTITY_ID)
        g_entity_mgr->destroyEntity(o->getEid());

      o->resetEid();
      o->setWtm(tm);

      newObjOk = true;
    }
  }

  if (!name && !is_creating_entity_mode(getEditMode()))
  {
    newObj = NULL;
    newObjOk = false;
  }
}

void EntityObjEditor::createObjectBySample(EditableObject *so)
{
  EntityObj *o = RTTI_cast<EntityObj>(so);
  G_ASSERT(o);

  if (!*o->getName())
    return;

  EditableObject *no = nullptr;
  eastl::string templName = o->getName();
  const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(templName);
  bool singl = templ && templ->isSingleton();

  if (singl)
  {
    G_ASSERTF_RETURN(o->getEid() == ecs::INVALID_ENTITY_ID, , "o->getEid()=%u but template is singleton",
      (unsigned)(ecs::entity_id_t)o->getEid());

    EntCreateData cd(templName.c_str());
    no = new EntityObj(templName.c_str(), createEntityDirect(&cd));
    o->setName(nullptr);
  }

  if (!no)
  {
    if (o->getEid() == ecs::INVALID_ENTITY_ID)
    {
      if (!newObjOk)
        visuallog::logmsg(String(0, "%sentity for (%s) not created", singl ? "singleton " : "", templName.c_str()),
          E3DCOLOR(200, 0, 0, 255));
      return;
    }
    ecs::EntityId previewEid = o->getEid();
    if (g_entity_mgr->getTemplateDB().getTemplateByName(PREVIEW_TEMPLATE))
    {
      remove_sub_template_async(previewEid, PREVIEW_TEMPLATE);
      g_entity_mgr->tick();
    }

    ecs::g_scenes->getActiveScene().insertEmptyEntityRecord(previewEid, templName.c_str());
    no = new EntityObj(templName.c_str(), previewEid);
    o->resetEid();
  }

  no->setWtm(o->getWtm());

  getUndoSystem()->begin();
  addObject(no);
  if (EntityObj *noEntityObj = RTTI_cast<EntityObj>(no))
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
      visuallog::logmsg(String(0, "created %sentity %u (%s)", singl ? "singleton " : "", noEntityObj->getEid(), templName.c_str()));

    sqeventbus::send_event("entity_editor.onEntityNewBySample", Json::Value((ecs::entity_id_t)noEntityObj->getEid()));
  }
  getUndoSystem()->accept("Add Entity");

  if (!singl)
    selectNewObjEntity(templName.c_str());
}

bool EntityObjEditor::cancelCreateMode()
{
  if (getEditMode() != CM_OBJED_MODE_CREATE_ENTITY)
    return false;

  if (!newObj.get())
    return false;

  bool wasSample = false;
  if (EntityObj *newEntObj = RTTI_cast<EntityObj>(newObj))
    if (newEntObj->getEid() != ecs::INVALID_ENTITY_ID)
      wasSample = true;

  setCreateBySampleMode(NULL);
  objCreator.reset();
  selectNewObjEntity(NULL);

  if (wasSample)
  {
    newObj = new EntityObj("new", ecs::INVALID_ENTITY_ID);
    newObjOk = true;
    setCreateBySampleMode(newObj);
  }
  else
  {
    setEditMode(CM_OBJED_MODE_SELECT);
  }

  return true;
}

void EntityObjEditor::beforeRender()
{
  if (is_editor_in_reload() && !--delay_to_update_eids)
  {
    refreshEids();
    initSceneObjects();
    finish_editor_reload();
    delay_to_update_eids = DELAY_TO_UPDATE_EIDS;
  }
  ObjectEditor::beforeRender();
}

void EntityObjEditor::render(const Frustum &frustum, const Point3 &camera_pos)
{
  ObjectEditor::render(frustum, camera_pos);
  if (objCreator)
    objCreator->render();
  if (newObj)
    newObj->render();
}

#define POINTACTION_PREVIEW_KINDS                    \
  POINTACTION_PREVIEW_KIND(OFF)                      \
  POINTACTION_PREVIEW_KIND(HMAP_CIRCLE_WHITE)        \
  POINTACTION_PREVIEW_KIND(HMAP_DOUBLE_CIRCLE_WHITE) \
  POINTACTION_PREVIEW_KIND(HMAP_DASHED_CIRCLE_WHITE) \
  POINTACTION_PREVIEW_KIND(HMAP_CIRCLE_GREEN)        \
  POINTACTION_PREVIEW_KIND(HMAP_DOUBLE_CIRCLE_GREEN) \
  POINTACTION_PREVIEW_KIND(HMAP_DASHED_CIRCLE_GREEN)
enum
{
#define POINTACTION_PREVIEW_KIND(name) POINTACTION_PREVIEW_##name,
  POINTACTION_PREVIEW_KINDS
#undef POINTACTION_PREVIEW_KIND
};
int EntityObjEditor::parsePointActionPreviewShape(const char *shape)
{
#define POINTACTION_PREVIEW_KIND(name) \
  if (strcmp(shape, #name) == 0)       \
    return POINTACTION_PREVIEW_##name;
  POINTACTION_PREVIEW_KINDS
#undef POINTACTION_PREVIEW_KIND
  return -1;
}

static void draw_cached_debug_circle(const Point3 &pos, float radius, bool by_hmap, bool dashed, E3DCOLOR color_front,
  E3DCOLOR color_behind)
{
  TMatrix wtm;
  wtm.identity();
  ::set_cached_debug_lines_wtm(wtm);

  int circle_parts = clamp((int)floorf(radius * 8.0f), 16, 1024);
  if (circle_parts & 1)
    ++circle_parts;

  for (int i = 0; i < circle_parts; ++i)
  {
    if (dashed && (i & 1) == 0)
      continue;

    const int j = (i + 1) % circle_parts;
    const float a1 = (TWOPI * i) / circle_parts;
    const float a2 = (TWOPI * j) / circle_parts;
    const float x1 = pos.x + radius * cosf(a1);
    const float z1 = pos.z + radius * sinf(a1);
    const float x2 = pos.x + radius * cosf(a2);
    const float z2 = pos.z + radius * sinf(a2);

    float y1 = 0.0f;
    float y2 = 0.0f;
    if (by_hmap)
    {
      const float htBias = 0.10f;
      y1 = dacoll::traceht_hmap(Point2(x1, z1)) + htBias;
      y2 = dacoll::traceht_hmap(Point2(x2, z2)) + htBias;
    }

    const Point3 p1(x1, y1, z1);
    const Point3 p2(x2, y2, z2);
    ::draw_cached_debug_line_twocolored(p1, p2, color_front, color_behind);
  }
}
void EntityObjEditor::renderPointActionPreviewShape(int shape, const Point3 &pos, float param)
{
  const E3DCOLOR white = E3DCOLOR_MAKE(255, 255, 255, 255);
  const E3DCOLOR green = E3DCOLOR_MAKE(32, 255, 32, 255);
  const E3DCOLOR white2 = E3DCOLOR_MAKE(255, 255, 255, 64);
  const E3DCOLOR green2 = E3DCOLOR_MAKE(32, 255, 32, 64);

  switch (shape)
  {
    case POINTACTION_PREVIEW_HMAP_CIRCLE_WHITE: draw_cached_debug_circle(pos, param, true, false, white, white2); break;
    case POINTACTION_PREVIEW_HMAP_DOUBLE_CIRCLE_WHITE:
      draw_cached_debug_circle(pos, param, true, false, white, white2);
      draw_cached_debug_circle(pos, param * 0.95f, true, false, white, white2);
      break;
    case POINTACTION_PREVIEW_HMAP_DASHED_CIRCLE_WHITE: draw_cached_debug_circle(pos, param, true, true, white, white2); break;
    case POINTACTION_PREVIEW_HMAP_CIRCLE_GREEN: draw_cached_debug_circle(pos, param, true, false, green, green2); break;
    case POINTACTION_PREVIEW_HMAP_DOUBLE_CIRCLE_GREEN:
      draw_cached_debug_circle(pos, param, true, false, green, green2);
      draw_cached_debug_circle(pos, param * 0.95f, true, false, green, green2);
      break;
    case POINTACTION_PREVIEW_HMAP_DASHED_CIRCLE_GREEN: draw_cached_debug_circle(pos, param, true, true, green, green2); break;
    default: break;
  }
}

void EntityObjEditor::hideSelectedTemplate()
{
  if (selection.empty())
  {
    if (hiddenTemplates.empty())
      return;

    auto &hideTemplates = hiddenTemplates;
    forEachEntityObjWithTemplate([&hideTemplates](EntityObj *o, const char *templName) {
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
  forEachEntityObjWithTemplate([&hideTemplates](EntityObj *o, const char *templName) {
    if (o->isSelected())
    {
      o->hideObject(true);

      eastl::string tplName = templName;
      removeSelectedTemplateName(tplName);
      hideTemplates.push_back(eastl::move(tplName));
    }
  });
}

void EntityObjEditor::hideUnmarkedEntities(Sqrat::Array eids_arr)
{
  if (eids_arr.Length())
  {
    ska::flat_hash_set<ecs::EntityId, ecs::EidHash> marked_eids;
    for (int i = 0; i < eids_arr.Length(); ++i)
      marked_eids.emplace(eids_arr.GetValue<ecs::EntityId>(i));

    hiddenTemplates.clear();
    auto &hideTemplates = hiddenTemplates;
    forEachEntityObjWithTemplate([&marked_eids, &hideTemplates](EntityObj *o, const char *templName) {
      if (marked_eids.find(o->getEid()) == marked_eids.end())
      {
        o->hideObject(true);

        eastl::string tplName = templName;
        removeSelectedTemplateName(tplName);
        hideTemplates.push_back(eastl::move(tplName));
      }
    });
  }
}

void EntityObjEditor::unhideAll()
{
  bool showAll = !showOnlySceneEntities;
  forEachEntityObj([&showAll](EntityObj *o) {
    if (showAll || is_scene_entity(o))
      o->hideObject(false);
  });

  hiddenTemplates.clear();
}

void EntityObjEditor::zoomAndCenter()
{
  BBox3 box;
  if (getSelectionBoxFocused(box))
    DAEDITOR4.zoomAndCenter(box);
}

void EntityObjEditor::setupNewScene(const char *fpath)
{
  sceneFilePath = fpath;

  if (!sceneInit)
    sceneInit = true;
  else
  {
    setCreateBySampleMode(NULL);
    objCreator.reset();
    selectNewObjEntity(NULL);

    clear_and_shrink(objects);
    clear_and_shrink(selection);
    selectionFocus = nullptr;

    eidsCreated.clear();
    eidToEntityObj.clear();
    hiddenTemplates.clear();

    createdSceneObjs.clear();
    sceneIdToEObj.clear();
    objsToRemove.clear();
    DAEDITOR4.undoSys.clear();
  }
}

void EntityObjEditor::saveEntityToBlk(ecs::EntityId eid, DataBlock &blk) const
{
  const ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::EntityRecord *erec = scene.findEntityRecord(eid);
  if (!erec)
    return;

  const Ptr<EditableObject> *editableObject = eastl::find_if(objects.begin(), objects.end(), [&eid](EditableObject *const lhs) {
    if (EntityObj *const o = RTTI_cast<EntityObj>(lhs))
      return o->getEid() == eid;
    return false;
  });

  if (editableObject)
    if (EntityObj *const entityObject = RTTI_cast<EntityObj>(editableObject->get()))
      entityObject->save(blk, *erec);
}

void EntityObjEditor::setMainScenePath(const char *fpath)
{
  if (!fpath)
  {
    return;
  }

  sceneFilePath = fpath;
  ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesPtrList &importRecords = saveScene.getScenesForModify(ecs::Scene::LoadType::IMPORT);
  for (uint32_t importsIdx = 0; importsIdx < importRecords.size(); ++importsIdx)
  {
    if (importRecords[importsIdx]->importDepth == 0)
    {
      saveScene.setNewChangesApplied(importRecords[importsIdx]->id);
    }
  }
}

void EntityObjEditor::saveMainSceneCopy(const char *fpath, const char *reason)
{
  if (!reason || !*reason)
    reason = "copy";

  if (!fpath || !*fpath)
  {
    logerr("daEd4: bad scene saving path '%s' (for %s)", fpath, reason);
    return;
  }

  ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesConstPtrList &importRecords = saveScene.getScenes(ecs::Scene::LoadType::IMPORT);
  const auto it = eastl::find_if(importRecords.begin(), importRecords.end(), [](auto &&val) { return val->importDepth == 0; });
  if (it == importRecords.end())
  {
    logerr("daEd4: failed to save main scene. Scene with importDepth == 0 was not found");
    return;
  }

  if (saveSceneToBlk(fpath, **it, *this))
  {
    visuallog::logmsg(String(0, "daEd4: Scene copy saved successfully (for %s)", reason));
  }
  else
  {
    logerr("daEd4: failed to save main scene. BLK save failed. Path %s", fpath);
  }
}

void EntityObjEditor::saveMainScene()
{
  ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesPtrList &importRecords = saveScene.getScenesForModify(ecs::Scene::LoadType::IMPORT);

  const auto it = eastl::find_if(importRecords.begin(), importRecords.end(), [](auto &&val) { return val->importDepth == 0; });
  if (it == importRecords.end())
  {
    logerr("daEd4: failed to save main scene. Scene with importDepth == 0 was not found");
    return;
  }

  if (saveSceneToBlk((*it)->path.c_str(), **it, *this))
  {
    visuallog::logmsg("daEd4: MAIN scene saved successfully");
  }
  else
  {
    logerr("daEd4: failed to save main scene %s. BLK save failed. Path %s", (*it)->path.c_str());
  }

  (*it)->isDirty = false;
}

void EntityObjEditor::saveDirtyScenes()
{
  ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesPtrList &importRecords = saveScene.getScenesForModify(ecs::Scene::LoadType::IMPORT);
  bool wasAnythingSaved = false;
  bool hasErrors = false;

  for (ecs::Scene::SceneRecord *record : importRecords)
  {
    if (!record->isDirty)
    {
      continue;
    }

    if (!saveSceneToBlk(record->path.c_str(), *record, *this))
    {
      logerr("daEd4: failed to save %s scene. BLK save failed. Path %s", record->path.c_str());
      hasErrors = true;
    }
    else
    {
      record->isDirty = false;
    }

    wasAnythingSaved = true;
  }

  if (wasAnythingSaved)
  {
    visuallog::logmsg(String(0, "daEd4: Dirty scenes were saved%s", hasErrors ? " with errors" : ""));
  }
}

void EntityObjEditor::saveScenes(const eastl::vector<ecs::Scene::SceneId> scenes_to_save)
{
  ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();
  bool hasErrors = false;
  for (const ecs::Scene::SceneId sceneId : scenes_to_save)
  {
    if (ecs::Scene::SceneRecord *record = saveScene.getSceneRecordById(sceneId))
    {
      if (!saveSceneToBlk(record->path.c_str(), *record, *this))
      {
        logerr("daEd4: failed to save %s scene. BLK save failed. Path %s", record->path.c_str());
        hasErrors = true;
      }
      else
      {
        record->isDirty = false;
      }
    }
  }

  if (!scenes_to_save.empty())
  {
    visuallog::logmsg(String(0, "daEd4: Scenes were saved%s", hasErrors ? " with errors" : ""));
  }
}

bool EntityObjEditor::hasUnsavedChanges() const { return ecs::g_scenes->getActiveScene().hasUnsavedChanges(); }

bool EntityObjEditor::isChildScene(ecs::Scene::SceneId id) { return ecs::g_scenes->getActiveScene().isChildScene(id); }

void EntityObjEditor::setTargetScene(ecs::Scene::SceneId id) { ecs::g_scenes->getActiveScene().setTargetSceneId(id); }

bool EntityObjEditor::processCommand(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("daEd4", "saveObjects", 1, 2)
  {
    setMainScenePath(argc > 1 ? argv[1] : NULL);
    saveMainScene();
  } //-V547
  CONSOLE_CHECK_NAME("daEd4", "showPath", 1, 1) { console::print_d("daEd4: current object filepath: %s", sceneFilePath); }
  CONSOLE_CHECK_NAME("daEd4", "listObj", 1, 2)
  {
    int cnt = 0;
    forEachEntityObjWithTemplate([&cnt, argc, argv](EntityObj *o, const char *templName) {
      if (argc == 1 || (strcmp(argv[1], templName) == 0))
      {
        console::print_d("  <%s> t=%s", o->getName(), templName);
        cnt++;
      }
    });
    console::print_d("found %d objects (of %d total)", cnt, objects.size());
  }
  CONSOLE_CHECK_NAME("daEd4", "zoom", 1, 5)
  {
    BBox3 box;
    if (argc == 5) // daEd4.zoom X Y Z R
    {
      box = BBox3(Point3(atof(argv[1]), atof(argv[2]), atof(argv[3])), atof(argv[4]));
      DAEDITOR4.zoomAndCenter(box);
    }
    else if (getSelectionBox(box))
    {
      DAEDITOR4.zoomAndCenter(box);
    }
  }
  CONSOLE_CHECK_NAME("daEd4", "selectObj", 2, 2)
  {
    DAEDITOR4.undoSys.begin();
    unselectAll();
    forEachEntityObjWithTemplate([argv](EntityObj *o, const char *templName) {
      if (strstr(o->getName(), argv[1]))
      {
        o->selectObject();
        console::print_d("  select <%s> of template <%s>", o->getName(), templName);
      }
    });
    updateSelection();
    updateGizmo();
    DAEDITOR4.undoSys.accept("Select");
    console::print_d("selected %d objects (of %d total) by substr %s", selection.size(), objects.size(), argv[1]);
  }
  CONSOLE_CHECK_NAME("daEd4", "selectEid", 2, 4)
  {
    DAEDITOR4.undoSys.begin();
    const bool select = argc < 3 || console::to_bool(argv[2]);
    const bool deselect = argc < 4 || console::to_bool(argv[3]);
    if (deselect)
      unselectAll();
    const ecs::EntityId eid = ecs::EntityId((ecs::entity_id_t)atoi(argv[1]));
    forEachEntityObjWithTemplate([eid, select](EntityObj *o, const char *templName) {
      if (o->getEid() == eid)
      {
        o->selectObject(select);
        console::print_d("  select <%s> of template <%s>", o->getName(), templName);
      }
    });
    updateSelection();
    updateGizmo();
    DAEDITOR4.undoSys.accept("Select");
    console::print_d("selected %d objects (of %d total) by eid %d", selection.size(), objects.size(), (ecs::entity_id_t)eid);
  }
  CONSOLE_CHECK_NAME("daEd4", "set_debug_group", 1, 2)
  {
    forEachEntityObj([argc, argv](EntityObj *o) {
      if (is_scene_entity(o))
        o->hideObject(argc > 1 ? strcmp(argv[1], g_entity_mgr->getOr(o->getEid(), ECS_HASH("groupName"), "")) != 0 : false);
    });
  }
  CONSOLE_CHECK_NAME_EX("daEd4", "show_nonscene_entities", 1, 2,
    "Make selectable and searchable not only this scene entities, but also "
    "all added entities from level, generated doors, common scenes entities, "
    "singletons and etc.",
    "[show=true]")
  {
    const bool show = argc > 1 ? (atoi(argv[1]) > 0) : true;
    showOnlySceneEntities = !show;
    forEachEntityObj([&show](EntityObj *o) {
      if (!is_scene_entity(o))
        o->hideObject(!show);
    });
  }
  return found;
}


void EntityObjEditor::onObjectFlagsChange(EditableObject *obj, int changed_flags)
{
  ObjectEditor::onObjectFlagsChange(obj, changed_flags);
  if (changed_flags & EditableObject::FLG_SELECTED)
  {
    if (EntityObj *o = RTTI_cast<EntityObj>(obj))
    {
      if (!o->isValid())
        return;

      if (!o->isSelected())
        remove_sub_template_async(o->getEid(), SELECTED_TEMPLATE);
      else
        add_sub_template_async(o->getEid(), SELECTED_TEMPLATE);
    }
  }

  sqeventbus::send_event("entity_editor.edObjectFlagsUpdateTrigger");
}

void EntityObjEditor::selectionChanged()
{
  ObjectEditor::selectionChanged();

  // Let's start with single active entity for now
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  if (!selection.empty())
    if (EntityObj *o = RTTI_cast<EntityObj>(selection[0]))
      if (o->isValid())
        eid = o->getEid();

  select_entity_webui(eid);
}

void EntityObjEditor::update(float dt)
{
  ObjectEditor::update(dt);
  if (objsToRemove.size())
  {
    removeObjects(objsToRemove.data(), objsToRemove.size(), false);
    objsToRemove.clear();
  }

  if (newObj.get() && !newObj->isValid() && getEditMode() == CM_OBJED_MODE_CREATE_ENTITY)
  {
    cancelCreateMode();
  }

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
      DAEDITOR4.zoomAndCenter(box);
      setEditMode(CM_OBJED_MODE_SELECT);

      if (EntityObj *movedObj = RTTI_cast<EntityObj>(lastCreatedObj.get()))
        saveComponent(movedObj->getEid(), "transform");
    }
  }

  ecs::g_scenes->recalculateWbbs();
}

bool EntityObjEditor::isPickingRendinst() { return pick_rendinst.get(); }

bool EntityObjEditor::pickObjects(int x, int y, Tab<EditableObject *> &objs)
{
  bool ret = ObjectEditor::pickObjects(x, y, objs);
  if (!isPickingRendinst())
    return ret; // early exit
  Point3 from, dir, norm;
  float dist = 1500;
  Point2 mouse = Point2(x, y);

  DAEDITOR4.clientToWorld(mouse, from, dir);
  DAEDITOR4.traceRay(from, dir, dist);
  rendinst::RendInstDesc ri_desc;
  if (rendinst::traceRayRendInstsNormalized(from, dir, dist, norm, false, false, &ri_desc))
    if (ri_desc.isValid() && ri_desc.isRiExtra())
    {
      for (int i = 0; i < objects.size(); i++)
        if (RendInstObj *o = RTTI_cast<RendInstObj>(objects[i]))
          if (ri_desc.getRiExtraHandle() == o->getRiEx())
            return ret;
      RendInstObj *o = new RendInstObj(ri_desc);
      addObject(o, false);
      objs.push_back(o);
      ret = true;
      sort(objs, &cmp_obj_z);
      o->selectObject();
      const char *resName = rendinst::getRIGenExtraName(ri_desc.pool);
      console::print_d("daEd4: picked rendinst <%s>", resName ? resName : "?");
      visuallog::logmsg(String(0, "daEd4: picked rendinst <%s>", resName ? resName : "?"));
    }
  return ret;
}

void EntityObjEditor::fillDeleteDepsObjects(Tab<EditableObject *> &list) const
{
  const uint32_t originalSize = list.size();
  EditorToDeleteEntitiesEvent evt{{}};
  for (uint32_t i = 0; i < originalSize; ++i)
  {
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(list[i]))
    {
      g_entity_mgr->sendEventImmediate(entityObj->getEid(), evt);
    }
  }

  const ecs::EidList &linkedEntitiesToDelete = evt.get<0>();
  list.reserve(originalSize + linkedEntitiesToDelete.size());
  for (const ecs::EntityId &eid : linkedEntitiesToDelete)
  {
    if (EditableObject *object = getObjectByEid(eid))
      list.emplace_back(object);
  }
  list.erase(eastl::unique(list.begin(), list.end()), list.end());
}


void EntityObjEditor::cloneSelection()
{
  ObjectEditor::cloneSelection();
  for (int i = 0; i < cloneObjs.size(); ++i)
    if (EntityObj *cloneObjEntityObj = RTTI_cast<EntityObj>(cloneObjs[i]))
      eidsCreated.insert(cloneObjEntityObj->getEid());
}

bool EntityObjEditor::isSceneEntity(ecs::EntityId eid)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return false;
  const char *templName = g_entity_mgr->getEntityTemplateName(eid);
  if (!templName)
    return false;
  const ecs::Template *pTemplate = g_entity_mgr->getTemplateDB().getTemplateByName(templName);
  if (!pTemplate)
    return false;

  return true;
}

bool EntityObjEditor::checkSceneEntities(const char *rule)
{
  if (!rule)
    return false;
  bool result = false;
  forEachEntityObj([this, &result, &rule](EntityObj *o) {
    if (!is_scene_entity(o))
      return;
    const char *templName = g_entity_mgr->getEntityTemplateName(o->getEid());
    if (!templName)
      return;
    const ecs::Template *pTemplate = g_entity_mgr->getTemplateDB().getTemplateByName(templName);
    if (!pTemplate)
      return;
    TemplatesGroup tmp;
    if (testEcsTemplateCondition(*pTemplate, rule, tmp, 1))
      result = true;
  });
  return result;
}

int EntityObjEditor::getEntityRecordLoadType(ecs::EntityId eid)
{
  auto *record = ecs::g_scenes->getActiveScene().findEntityRecord(eid);
  if (record)
    return record->loadType;

  return ecs::Scene::UNKNOWN;
}

ecs::Scene::SceneId EntityObjEditor::getEntityRecordSceneId(ecs::EntityId eid) const
{
  auto *record = ecs::g_scenes->getActiveScene().findEntityRecord(eid);
  if (record)
    return record->sceneId;

  return ecs::Scene::C_INVALID_SCENE_ID;
}

void EntityObjEditor::addImportScene(ecs::Scene::SceneId parent_id, const char *fpath)
{
  if (!fpath)
  {
    return;
  }

  DataBlock block(fpath);
  if (block.isValid())
  {
    const ecs::Scene::SceneId sceneId = ecs::g_scenes->addImportScene(parent_id, block, fpath);
    if (sceneId != ecs::Scene::C_INVALID_SCENE_ID)
    {
      DAEDITOR4.undoSys.begin();

      DAEDITOR4.undoSys.put(new SceneAddUndoRedo(sceneId, *this));
      DAEDITOR4.undoSys.accept("addImportScene");
    }
  }
}

bool EntityObjEditor::isEntityTransformable(ecs::EntityId eid) const
{
  if (EntityObj *eo = RTTI_cast<EntityObj>(getObjectByEid(eid)))
  {
    return eo->hasTransform();
  }

  return false;
}

bool EntityObjEditor::isSceneInTransformableHierarchy(ecs::Scene::SceneId scene_id) const
{
  return ecs::g_scenes->isSceneInTransformableHierarchy(scene_id);
}

// Pivot and Transformable getters/setters work on Import scenes
bool EntityObjEditor::isSceneTransformable(ecs::Scene::SceneId scene_id) const
{
  return ecs::g_scenes->isSceneTransformable(scene_id);
}

void EntityObjEditor::setSceneTransformable(ecs::Scene::SceneId scene_id, bool val)
{
  if (ecs::g_scenes->isSceneTransformable(scene_id) == val)
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  DAEDITOR4.undoSys.put(new SceneTransformableUndoRedo(scene_id));

  ecs::g_scenes->setSceneTransformable(scene_id, val);

  DAEDITOR4.undoSys.accept("setSceneTransformable");

  sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
}

Point3 EntityObjEditor::getScenePivot(ecs::Scene::SceneId scene_id) const { return ecs::g_scenes->getScenePivot(scene_id); }

void EntityObjEditor::setScenePivot(ecs::Scene::SceneId scene_id, const Point3 &pivot)
{
  DAEDITOR4.undoSys.begin();

  DAEDITOR4.undoSys.put(new ScenePivotUndoRedo(scene_id, pivot));
  ecs::g_scenes->setScenePivot(scene_id, pivot);

  DAEDITOR4.undoSys.accept("setScenePivot");
}

const char *EntityObjEditor::getScenePrettyName(ecs::Scene::SceneId scene_id) const
{
  return ecs::g_scenes->getScenePrettyName(scene_id);
}

void EntityObjEditor::setScenePrettyName(ecs::Scene::SceneId scene_id, const char *name)
{
  DAEDITOR4.undoSys.begin();

  DAEDITOR4.undoSys.put(new SceneNameUndoRedo(scene_id, SimpleString{name}));
  ecs::g_scenes->setScenePrettyName(scene_id, name);

  DAEDITOR4.undoSys.accept("setScenePrettyName");

  sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
}

void EntityObjEditor::setSceneNewParent(ecs::Scene::SceneId new_parent_id, Sqrat::Array items)
{
  const eastl::vector<SqObjectData> itemData = parseSqObjectDataArray(items);

  if (itemData.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (const SqObjectData &item : itemData)
  {
    if (item.isEntity)
    {
      DAEDITOR4.undoSys.put(new EntitySetParentUndoRedo(item.eid, new_parent_id));
      ecs::g_scenes->setEntityParent(new_parent_id, {item.eid});
    }
    else
    {
      DAEDITOR4.undoSys.put(new SceneSetParentUndoRedo(item.sid, new_parent_id));
      ecs::g_scenes->setNewParent(item.sid, new_parent_id);
    }
  }

  DAEDITOR4.undoSys.accept("setSceneNewParent");

  sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
}

void EntityObjEditor::setSceneNewParentAndOrder(ecs::Scene::SceneId new_parent_id, uint32_t order, Sqrat::Array items)
{
  const eastl::vector<SqObjectData> itemData = parseSqObjectDataArray(items);

  if (itemData.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (uint32_t i = 0; i < itemData.size(); ++i)
  {
    const SqObjectData &item = itemData[i];
    if (item.isEntity)
    {
      if (getEntityRecordSceneId(item.eid) != new_parent_id)
      {
        DAEDITOR4.undoSys.put(new EntitySetParentUndoRedo(item.eid, new_parent_id));
        ecs::g_scenes->setEntityParent(new_parent_id, {item.eid});
      }

      DAEDITOR4.undoSys.put(new EntityOrderUndoRedo(item.eid, order + i));
      ecs::g_scenes->setEntityOrder(item.eid, order + i);
    }
    else
    {
      ecs::Scene::SceneId currentParent = ecs::Scene::C_INVALID_SCENE_ID;
      if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(item.sid))
      {
        currentParent = record->parent;
      }

      if (currentParent != new_parent_id)
      {
        DAEDITOR4.undoSys.put(new SceneSetParentUndoRedo(item.sid, new_parent_id));
        ecs::g_scenes->setNewParent(item.sid, new_parent_id);
      }

      DAEDITOR4.undoSys.put(new SceneOrderUndoRedo(item.sid, order + i));
      ecs::g_scenes->setSceneOrder(item.sid, order + i);
    }
  }

  DAEDITOR4.undoSys.accept("setSceneParentAndOrder");

  sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
}

void EntityObjEditor::setSceneOrder(ecs::Scene::SceneId id, uint32_t order)
{
  DAEDITOR4.undoSys.begin();

  DAEDITOR4.undoSys.put(new SceneOrderUndoRedo(id, order));
  ecs::g_scenes->setSceneOrder(id, order);

  DAEDITOR4.undoSys.accept("setSceneOrder");

  sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
}

uint32_t EntityObjEditor::getSceneOrder(ecs::Scene::SceneId id) const { return ecs::g_scenes->getSceneOrder(id); }

uint32_t EntityObjEditor::getSceneEntityCount(ecs::Scene::SceneId id) const
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(id))
  {
    return record->entities.size();
  }

  return 0;
}

Sqrat::Array EntityObjEditor::getSceneOrderedEntries(HSQUIRRELVM vm, ecs::Scene::SceneId id)
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(id))
  {
    Sqrat::Array res(vm, record->orderedEntries.size());
    for (uint32_t i = 0; i < res.GetSize(); ++i)
    {
      Sqrat::Table table(vm);
      if (record->orderedEntries[i].isEntity)
      {
        table.SetValue("eid", record->orderedEntries[i].eid);
      }
      else
      {
        table.SetValue("sid", record->orderedEntries[i].sid);
      }
      table.SetValue("isEntity", record->orderedEntries[i].isEntity);

      res.SetValue(i, table);
    }

    return res;
  }

  return {};
}

void EntityObjEditor::removeObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> data = parseSqObjectDataArray(objects_ids);

  if (data.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  // remove entities first
  {
    Tab<EditableObject *> entityList;
    for (const SqObjectData &objData : data)
    {
      if (objData.isEntity)
      {
        if (EditableObject *eo = getObjectByEid(objData.eid))
        {
          entityList.push_back(eo);
        }
      }
    }

    if (!entityList.empty())
    {
      fillDeleteDepsObjects(entityList);
      _removeObjects(&entityList[0], entityList.size(), true);
    }
  }

  // remove scenes
  {
    Tab<EditableObject *> sceneList;
    for (const SqObjectData &objData : data)
    {
      if (!objData.isEntity)
      {
        if (EditableObject *so = getSceneObjectById(objData.sid))
        {
          sceneList.push_back(so);
        }
      }
    }

    if (!sceneList.empty())
    {
      _removeObjects(&sceneList[0], sceneList.size(), true);
    }
  }

  DAEDITOR4.undoSys.accept("removeObjects");
}

void EntityObjEditor::hideObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> data = parseSqObjectDataArray(objects_ids);

  if (data.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (const SqObjectData &objData : data)
  {
    if (objData.isEntity)
    {
      if (EditableObject *eo = getObjectByEid(objData.eid))
      {
        eo->hideObject(true);
      }
    }
    else
    {
      if (objData.sid == ecs::Scene::C_INVALID_SCENE_ID)
      {
        forEachEntityObj([&](EntityObj *eo) {
          if (auto *er = ecs::g_scenes->getActiveScene().findEntityRecord(eo->getEid()); !er)
          {
            eo->hideObject(true);
          }
        });
      }
      else
      {
        ecs::g_scenes->iterateHierarchy(objData.sid, [&](const ecs::Scene::SceneRecord &srecord) {
          if (EditableObject *so = getSceneObjectById(srecord.id))
          {
            so->hideObject(true);
          }

          for (const ecs::EntityId eid : srecord.entities)
          {
            if (EditableObject *eo = getObjectByEid(eid))
            {
              eo->hideObject(true);
            }
          }
        });
      }
    }
  }

  DAEDITOR4.undoSys.accept("hideObjects");
}

void EntityObjEditor::unhideObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> data = parseSqObjectDataArray(objects_ids);

  if (data.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (const SqObjectData &objData : data)
  {
    if (objData.isEntity)
    {
      if (EditableObject *eo = getObjectByEid(objData.eid))
      {
        eo->hideObject(false);

        if (const ecs::Scene::EntityRecord *er = ecs::g_scenes->getActiveScene().findEntityRecord(objData.eid))
        {
          iterateSceneAncestorsAsEdObjects(*this, er->sceneId, [](EditableObject *eo) { eo->hideObject(false); });
        }
      }
    }
    else
    {
      if (objData.sid == ecs::Scene::C_INVALID_SCENE_ID)
      {
        forEachEntityObj([&](EntityObj *eo) {
          if (auto *er = ecs::g_scenes->getActiveScene().findEntityRecord(eo->getEid()); !er)
          {
            eo->hideObject(false);
          }
        });
      }
      else
      {
        ecs::g_scenes->iterateHierarchy(objData.sid, [&](const ecs::Scene::SceneRecord &srecord) {
          if (EditableObject *so = getSceneObjectById(srecord.id))
          {
            so->hideObject(false);
          }

          for (const ecs::EntityId eid : srecord.entities)
          {
            if (EditableObject *eo = getObjectByEid(eid))
            {
              eo->hideObject(false);
            }
          }
        });

        iterateSceneAncestorsAsEdObjects(*this, objData.sid, [](EditableObject *eo) { eo->hideObject(false); });
      }
    }
  }

  DAEDITOR4.undoSys.accept("unhideObjects");
}

void EntityObjEditor::lockObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> data = parseSqObjectDataArray(objects_ids);

  if (data.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (const SqObjectData &objData : data)
  {
    if (objData.isEntity)
    {
      if (EditableObject *eo = getObjectByEid(objData.eid))
      {
        eo->lockObject(true);
      }
    }
    else
    {
      if (objData.sid == ecs::Scene::C_INVALID_SCENE_ID)
      {
        forEachEntityObj([&](EntityObj *eo) {
          if (auto *er = ecs::g_scenes->getActiveScene().findEntityRecord(eo->getEid()); !er)
          {
            eo->lockObject(true);
          }
        });
      }
      else
      {
        ecs::g_scenes->iterateHierarchy(objData.sid, [&](const ecs::Scene::SceneRecord &srecord) {
          if (EditableObject *so = getSceneObjectById(srecord.id))
          {
            so->lockObject(true);
          }

          for (const ecs::EntityId eid : srecord.entities)
          {
            if (EditableObject *eo = getObjectByEid(eid))
            {
              eo->lockObject(true);
            }
          }
        });
      }
    }
  }

  DAEDITOR4.undoSys.accept("lockObjects");
}

void EntityObjEditor::unlockObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> data = parseSqObjectDataArray(objects_ids);

  if (data.empty())
  {
    return;
  }

  DAEDITOR4.undoSys.begin();

  for (const SqObjectData &objData : data)
  {
    if (objData.isEntity)
    {
      if (EditableObject *eo = getObjectByEid(objData.eid))
      {
        eo->lockObject(false);

        if (const ecs::Scene::EntityRecord *er = ecs::g_scenes->getActiveScene().findEntityRecord(objData.eid))
        {
          iterateSceneAncestorsAsEdObjects(*this, er->sceneId, [](EditableObject *eo) { eo->lockObject(false); });
        }
      }
    }
    else
    {
      if (objData.sid == ecs::Scene::C_INVALID_SCENE_ID)
      {
        forEachEntityObj([&](EntityObj *eo) {
          if (auto *er = ecs::g_scenes->getActiveScene().findEntityRecord(eo->getEid()); !er)
          {
            eo->lockObject(false);
          }
        });
      }
      else
      {
        ecs::g_scenes->iterateHierarchy(objData.sid, [&](const ecs::Scene::SceneRecord &srecord) {
          if (EditableObject *so = getSceneObjectById(srecord.id))
          {
            so->lockObject(false);
          }

          for (const ecs::EntityId eid : srecord.entities)
          {
            if (EditableObject *eo = getObjectByEid(eid))
            {
              eo->lockObject(false);
            }
          }
        });

        iterateSceneAncestorsAsEdObjects(*this, objData.sid, [](EditableObject *eo) { eo->lockObject(false); });
      }
    }
  }

  DAEDITOR4.undoSys.accept("unlockObjects");
}

ecs::EntityId EntityObjEditor::reCreateEditorEntity(ecs::EntityId eid)
{
  EntityObj *obj = nullptr;
  forEachEntityObj([&eid, &obj](EntityObj *o) {
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
  }
  if (was_selected)
    obj->selectObject();
  return obj->getEid();
}

ecs::EntityId EntityObjEditor::makeSingletonEntity(const char *tplName)
{
  if (!tplName || !tplName[0])
    return ecs::INVALID_ENTITY_ID;

  const int tplNameLen = strlen(tplName);
  ecs::EntityId foundEid = ecs::INVALID_ENTITY_ID;
  forEachEntityObj([&tplName, &tplNameLen, &foundEid](EntityObj *o) {
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

  EntCreateData cd(tplName);
  ecs::EntityId objEid = createEntityDirect(&cd);
  if (objEid == ecs::INVALID_ENTITY_ID)
    return ecs::INVALID_ENTITY_ID;

  EntityObj *obj = new EntityObj(tplName, objEid);
  addObject(obj, false);
  eidsCreated.insert(obj->getEid());
  return obj->getEid();
}

void EntityObjEditor::selectEntities(Sqrat::Array eids_arr)
{
  DAEDITOR4.undoSys.begin();
  unselectAll();

  if (eids_arr.Length())
  {
    Tab<ecs::EntityId> eids(framemem_ptr());
    eids.resize(eids_arr.Length());

    eids_arr.GetArray(eids.data(), eids_arr.Length());

    forEachEntityObj([&eids](EntityObj *o) {
      if (eastl::find(eids.begin(), eids.end(), o->getEid()) != eids.end())
        o->selectObject();
    });
  }

  updateSelection();
  updateGizmo();
  DAEDITOR4.undoSys.accept("Select");
}

SQInteger EntityObjEditor::get_modified_scene_ids(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getModifiedSceneIds(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

Sqrat::Array EntityObjEditor::getModifiedSceneIds(HSQUIRRELVM vm) const
{
  eastl::vector<ecs::Scene::SceneId> modifiedIds;
  const ecs::Scene::ScenesConstPtrList &imports = ecs::g_scenes->getActiveScene().getScenes(ecs::Scene::LoadType::IMPORT);
  for (const ecs::Scene::SceneRecord *record : imports)
  {
    if (record->isDirty)
    {
      modifiedIds.push_back(record->id);
    }
  }

  Sqrat::Array result(vm, modifiedIds.size());
  for (int i = 0; i < modifiedIds.size(); ++i)
  {
    result.SetValue(SQInteger(i), modifiedIds[i]);
  }

  return result;
}

Sqrat::Array EntityObjEditor::getEntities(HSQUIRRELVM vm)
{
  if (orderedTemplatesGroups.empty())
    initDefaultTemplatesGroups();
  const TemplatesGroup *pTemplatesGroup = nullptr;
  const char *groupName = nullptr;
  sq_getstring(vm, SQInteger(2), &groupName);
  if (groupName)
  {
    for (auto &it : orderedTemplatesGroups)
    {
      if (it.name == groupName)
      {
        pTemplatesGroup = &it;
        break;
      }
    }
  }

  bool showAll = !showOnlySceneEntities;
  Tab<ecs::EntityId> entities(framemem_ptr());
  entities.reserve(objects.size());
  forEachEntityObj([&showAll, &entities](EntityObj *o) {
    if (showAll || is_scene_entity(o))
      entities.push_back(o->getEid());
  });

  if (pTemplatesGroup)
  {
    Tab<ecs::EntityId> filtered(framemem_ptr());
    filtered.reserve(entities.size());
    for (auto eid : entities)
    {
      const char *templ_name = g_entity_mgr->getEntityTemplateName(eid);
      const ecs::Template *pTemplate = g_entity_mgr->getTemplateDB().getTemplateByName(templ_name);
      if (pTemplate && testEcsTemplateByTemplatesGroup(*pTemplate, nullptr, *pTemplatesGroup, 0))
        filtered.push_back(eid);
    }
    entities = filtered;
  }

  Sqrat::Array res(vm, entities.size());
  for (int i = 0; i < entities.size(); ++i)
    res.SetValue(SQInteger(i), entities[i]);

  return res;
}

Sqrat::Array EntityObjEditor::getSceneEntities(HSQUIRRELVM vm)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  Tab<ecs::EntityId> entities(framemem_ptr());
  entities.reserve(scene.entitiesCount());
  forEachEntityObj([&scene, &entities](EntityObj *o) {
    if (scene.findEntityRecord(o->getEid()))
      entities.push_back(o->getEid());
  });

  Sqrat::Array res(vm, entities.size());
  for (int i = 0; i < entities.size(); ++i)
    res.SetValue(SQInteger(i), entities[i]);

  return res;
}

static Sqrat::Table sceneRecordToSqTable(HSQUIRRELVM vm, const ecs::Scene::SceneRecord &record)
{
  Sqrat::Table ret(vm);
  ret.SetValue("loadType", (int)record.loadType);
  ret.SetValue("path", record.path);
  ret.SetValue("importDepth", record.importDepth);
  ret.SetValue("parent", record.parent);
  ret.SetValue("hasParent", (bool)(record.parent != ecs::Scene::SceneRecord::NO_PARENT));
  ret.SetValue("id", record.id);
  return ret;
}

Sqrat::Array EntityObjEditor::getSceneImports(HSQUIRRELVM vm)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesList &scenes = scene.getAllScenes();
  Sqrat::Array res(vm, scenes.size());
  for (int i = 0; i < scenes.size(); ++i)
    res.SetValue(SQInteger(i), sceneRecordToSqTable(vm, scenes[i]));
  return res;
}

Sqrat::Table EntityObjEditor::getSceneRecord(HSQUIRRELVM vm)
{
  SQInteger sceneId;
  sq_getinteger(vm, SQInteger(2), &sceneId);
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  if (const ecs::Scene::SceneRecord *record = scene.getSceneRecordById(sceneId))
  {
    return sceneRecordToSqTable(vm, *record);
  }
  Sqrat::Table ret(vm);
  return ret;
}

Sqrat::Table EntityObjEditor::getTargetScene(HSQUIRRELVM vm)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::SceneId sceneId = scene.getTargetSceneId();
  if (const ecs::Scene::SceneRecord *record = scene.getSceneRecordById(sceneId))
  {
    return sceneRecordToSqTable(vm, *record);
  }

  Sqrat::Table ret(vm);
  return ret;
}

void EntityObjEditor::clearTemplatesGroups() { orderedTemplatesGroups.clear(); }

void EntityObjEditor::addTemplatesGroupRequire(const char *group_name, const char *require)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  group.name = group_name;
  group.reqs.push_back(require);
}

void EntityObjEditor::addTemplatesGroupVariant(const char *group_name, const char *variant)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  group.name = group_name;
  group.variants.push_back({variant, ""});
}

void EntityObjEditor::addTemplatesGroupTSuffix(const char *group_name, const char *tsuffix)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  if (group.variants.empty())
    return;
  group.variants.back().second = tsuffix;
}

static bool has_nonparent_component(const ecs::Template &ecs_template, const char *comp_name)
{
  return ecs_template.getComponentsMap().find(ECS_HASH_SLOW(comp_name)) != nullptr;
}

static bool has_prefixed_components(const ecs::Template &ecs_template, const char *prefix, int prefix_len)
{
  const auto &db = g_entity_mgr->getTemplateDB();
  const auto &comps = ecs_template.getComponentsMap();
  for (const auto &c : comps)
  {
    const char *compName = db.getComponentName(c.first);
    if (!compName)
      continue;
    const int compNameLen = strlen(compName);
    if (compNameLen >= prefix_len && strncmp(prefix, compName, prefix_len) == 0)
      return true;
  }
  return false;
}

bool EntityObjEditor::testEcsTemplateCondition(const ecs::Template &ecs_template, const eastl::string &condition,
  const TemplatesGroup &group, int depth)
{
  bool invert = false;
  bool result = false;

  const char *cond = condition.c_str();
  if (cond[0] == '!')
  {
    invert = true;
    ++cond;
  }

  if (cond[0] == '~')
  {
    ++cond;
    if (strcmp(cond, "singleton") == 0)
    {
      result = ecs_template.isSingleton();
    }
    else if (strcmp(cond, "hastags") == 0) // only in this template (not parents)
    {
      const auto &db = g_entity_mgr->getTemplateDB();
      const ecs::component_type_t tagHash = ECS_HASH("ecs::Tag").hash;

      const auto &comps = ecs_template.getComponentsMap();
      for (const auto &c : comps)
      {
        if (db.getComponentType(c.first) == tagHash)
        {
          result = true;
          break;
        }
      }
    }
    else if (strcmp(cond, "fitprev") == 0)
    {
      if (depth > 0)
        return false; // avoid cyclic

      result = false;
      for (const auto &checkgroup : orderedTemplatesGroups)
      {
        if (&checkgroup == &group)
          break;
        if (checkgroup.reqs.empty() && checkgroup.variants.empty())
          continue;
        if (testEcsTemplateByTemplatesGroup(ecs_template, nullptr, checkgroup, 1))
        {
          result = true;
          break;
        }
      }
    }
    else if (strcmp(cond, "noncreatable") == 0)
    {
      result = ecs_template.hasComponent(ECS_HASH("nonCreatableObj"), g_entity_mgr->getTemplateDB().data());
    }
  }
  else if (cond[0] == '\0')
  {
    result = true;
  }
  else if (cond[0] == '@') // only in this template (for lone template picks tags)
  {
    result = has_nonparent_component(ecs_template, cond + 1);
  }
  else if (cond[0] == '?') // has component with specific string value
  {
    ++cond;
    eastl::string cond_str;
    while (*cond != '\0' && *cond != '=')
      cond_str += *cond++;
    if (*cond == '=')
    {
      ++cond;
      const ecs::ChildComponent &comp =
        ecs_template.getComponent(ECS_HASH_SLOW(cond_str.c_str()), g_entity_mgr->getTemplateDB().data());
      if (comp.is<ecs::string>())
      {
        const char *comp_str = comp.get<ecs::string>().c_str();
        result = comp_str && strcmp(comp_str, cond) == 0;
      }
    }
  }
  else if (condition.back() == '*')
  {
    const int condLen = strlen(cond) - 1;
    const auto &dbdata = g_entity_mgr->getTemplateDB().data();
    dbdata.iterate_template_parents(ecs_template, [&](const ecs::Template &tmpl) {
      if (!result && has_prefixed_components(tmpl, cond, condLen))
        result = true;
    });
  }
  else
  {
    result = ecs_template.hasComponent(ECS_HASH_SLOW(cond), g_entity_mgr->getTemplateDB().data());
  }

  return invert ? !result : result;
}

bool EntityObjEditor::testEcsTemplateByTemplatesGroup(const ecs::Template &ecs_template, eastl::vector<eastl::string> *out_variants,
  const TemplatesGroup &group, int depth)
{
  if (out_variants)
    out_variants->clear();

  if (!ecs_template.canInstantiate())
    return false;

  for (auto &condition : group.reqs)
    if (!testEcsTemplateCondition(ecs_template, condition, group, depth))
      return false;

  if (group.variants.empty())
  {
    if (out_variants)
      out_variants->push_back("");
    return true;
  }

  for (auto &variant : group.variants)
  {
    if (testEcsTemplateCondition(ecs_template, variant.first, group, depth))
    {
      if (out_variants)
      {
        if (eastl::find(out_variants->begin(), out_variants->end(), variant.second) == out_variants->end())
          out_variants->push_back(variant.second);
      }
      else
        return true;
    }
  }

  if (out_variants && !out_variants->empty())
    return true;

  return false;
}

Sqrat::Array EntityObjEditor::getEcsTemplates(HSQUIRRELVM vm)
{
  if (orderedTemplatesGroups.empty())
    initDefaultTemplatesGroups();

  const TemplatesGroup *pTemplatesGroup = nullptr;

  const char *groupName;
  sq_getstring(vm, SQInteger(2), &groupName);

  for (auto &it : orderedTemplatesGroups)
  {
    if (!groupName || groupName[0] == '\0' || it.name == groupName)
    {
      pTemplatesGroup = &it;
      break;
    }
  }

  const ecs::TemplateDB &db = g_entity_mgr->getTemplateDB();

  eastl::vector<eastl::string> templates;
  templates.reserve(db.size());

  bool filterNonCreatable = true;
  if (pTemplatesGroup && !pTemplatesGroup->reqs.empty() && pTemplatesGroup->reqs[0] == "~noncreatable")
    filterNonCreatable = false;

  eastl::vector<eastl::string> variants;
  for (auto &it : db)
  {
    if (strchr(it.getName(), '+')) // avoid combined templates
      continue;
    if (filterNonCreatable && it.hasComponent(ECS_HASH("nonCreatableObj"), db.data()))
      continue;
    if (!pTemplatesGroup)
      templates.push_back(it.getName());
    else
    {
      if (!testEcsTemplateByTemplatesGroup(it, &variants, *pTemplatesGroup, 0))
        continue;
      for (const auto &variant : variants)
        if (variant[0] == '+')
          templates.push_back(it.getName() + variant);
        else
          templates.push_back(variant + it.getName());
    }
  }

  eastl::sort(templates.begin(), templates.end());

  int idx = 0;
  Sqrat::Array res(vm, templates.size());
  for (const auto &templateName : templates)
    res.SetValue(SQInteger(idx++), templateName);

  res.Resize(idx);
  return res;
}

Sqrat::Array EntityObjEditor::getEcsTemplatesGroups(HSQUIRRELVM vm)
{
  if (orderedTemplatesGroups.empty())
    initDefaultTemplatesGroups();

  int idx = 0;
  Sqrat::Array res(vm, orderedTemplatesGroups.size());
  for (auto &it : orderedTemplatesGroups)
    res.SetValue(SQInteger(idx++), it.name);

  return res;
}

void daEd4_get_all_ecs_tags(eastl::vector<eastl::string> &out_tags)
{
  out_tags.clear();

  const auto &db = g_entity_mgr->getTemplateDB();
  const ecs::component_type_t tagHash = ECS_HASH("ecs::Tag").hash;
  HashedKeySet<ecs::component_t> added;

  for (int i = 0; i < db.size(); ++i)
  {
    const ecs::Template *pTemplate = db.getTemplateById(i);
    if (!pTemplate)
      continue;
    const auto &comps = pTemplate->getComponentsMap();
    for (const auto &c : comps)
    {
      auto tp = db.getComponentType(c.first);
      if (tp == tagHash && !added.has_key(c.first))
      {
        added.insert(c.first);
        const char *compName = db.getComponentName(c.first);
        if (compName)
          out_tags.push_back(compName);
      }
    }
  }

  eastl::sort(out_tags.begin(), out_tags.end());
}

void EntityObjEditor::initDefaultTemplatesGroups()
{
  clearTemplatesGroups();
  addTemplatesGroupRequire("All", "");
  addTemplatesGroupRequire("Singletons", "~singleton");

  eastl::vector<eastl::string> tags;
  daEd4_get_all_ecs_tags(tags);
  eastl::string groupName;
  for (const auto &tag : tags)
  {
    groupName = "Tag: ";
    groupName += tag;
    addTemplatesGroupRequire(groupName.c_str(), tag.c_str());
  }
}

// TODO: extend Sqrat to allow passing VM handle to member functions
SQInteger EntityObjEditor::get_entites(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getEntities(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_scene_entities(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getSceneEntities(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_scene_imports(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getSceneImports(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_scene_record(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Table res = self.value->getSceneRecord(vm);
  Sqrat::Var<Sqrat::Table>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_target_scene(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Table res = self.value->getTargetScene(vm);
  Sqrat::Var<Sqrat::Table>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_ecs_templates(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getEcsTemplates(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_ecs_templates_groups(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Array res = self.value->getEcsTemplatesGroups(vm);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

SQInteger EntityObjEditor::get_scene_ordered_entries(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityObjEditor *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityObjEditor *> self(vm, 1);
  Sqrat::Var<ecs::Scene::SceneId> sceneId(vm, 2);
  Sqrat::Array res = self.value->getSceneOrderedEntries(vm, sceneId.value);
  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

void EntityObjEditor::saveScenesSq(Sqrat::Array ids)
{
  if (ids.Length())
  {
    eastl::vector<ecs::Scene::SceneId> sceneIds;
    sceneIds.resize(ids.Length());

    ids.GetArray(sceneIds.data(), ids.Length());

    saveScenes(sceneIds);
  }
}

void EntityObjEditor::selectObjectsSq(Sqrat::Array objects_ids)
{
  const eastl::vector<SqObjectData> ids = parseSqObjectDataArray(objects_ids);

  DAEDITOR4.undoSys.begin();
  unselectAll();

  if (!ids.empty())
  {
    for (EditableObject *obj : objects)
    {
      if (auto *eObj = RTTI_cast<EntityObj>(obj))
      {
        if (eastl::find_if(ids.cbegin(), ids.cend(),
              [id = eObj->getEid()](auto &&val) { return val.isEntity == true && val.eid == id; }) != ids.cend())
        {
          eObj->selectObject();
        }
      }
      else if (auto *sObj = RTTI_cast<SceneObj>(obj))
      {
        if (eastl::find_if(ids.cbegin(), ids.cend(),
              [id = sObj->getSceneId()](auto &&val) { return val.isEntity == false && val.sid == id; }) != ids.cend())
        {
          sObj->selectObject();
        }
      }
    }
  }

  updateSelection();
  updateGizmo();
  DAEDITOR4.undoSys.accept("Select");
}

eastl::string EntityObjEditor::getTemplateNameForUI(ecs::EntityId eid)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return "<invalid>";
  const char *tplName = g_entity_mgr->getEntityTemplateName(eid);
  if (!tplName)
    return "<invalid>";
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
  {
    eastl::string uiTplName = tplName;
    removeSelectedTemplateName(uiTplName);
    return uiTplName;
  }
  bool added = false;
  eastl::string uiTplName = erec->templateName;
  removeSelectedTemplateName(uiTplName);
  for_each_sub_template_name(tplName, [&erec, &uiTplName, &added](const char *tpl_name) {
    if (strcmp(tpl_name, SELECTED_TEMPLATE) == 0)
      return;
    if (find_sub_template_name(erec->templateName.c_str(), tpl_name))
      return;
    uiTplName += !added ? " (+" : "+";
    uiTplName += tpl_name;
    added = true;
  });
  if (added)
    uiTplName += ")";
  return uiTplName;
}

bool EntityObjEditor::hasTemplateComponent(const char *template_name, const char *comp_name)
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

bool EntityObjEditor::getSavedComponents(ecs::EntityId eid, eastl::vector<eastl::string> &out_comps)
{
  out_comps.clear();
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return false;
  for (const auto &comp : erec->clist)
    out_comps.push_back(comp.first);
  return true;
}

void EntityObjEditor::resetComponent(ecs::EntityId eid, const char *comp_name)
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

  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  auto it = eastl::find_if(erec->clist.begin(), erec->clist.end(),
    [comp_name](const ecs::ComponentsList::value_type &kv) { return kv.first == comp_name; });
  if (it == erec->clist.end())
    return;
  erec->clist.erase(it);
  scene.setNewChangesApplied(erec->sceneId);
}

void EntityObjEditor::saveComponent(ecs::EntityId eid, const char *comp_name)
{
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
  if (strcmp(comp_name, "transform") == 0)
  {
    scene.notifyEntityTransformWasChanged(eid);
  }
  scene.setNewChangesApplied(erec->sceneId);
}

void EntityObjEditor::getChangedComponentsToDataBlock(ecs::EntityId eid, DataBlock &blk)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::ComponentsList *compList = scene.findComponentsList(eid);
  if (!compList)
    return;

  ecs::components_to_blk(compList->begin(), compList->end(), blk, NULL, false);
}

void EntityObjEditor::mapEidToEditableObject(EntityObj *obj)
{
  if (obj == nullptr || newObj == obj || obj->getEid() == ecs::INVALID_ENTITY_ID)
    return;

  const auto inserted = eidToEntityObj.emplace(obj->getEid(), obj);
  if (!inserted.second)
    logerr("[%s] Entity with eid <%i> already exist in eidToEntityObj.", __FUNCTION__, (ecs::entity_id_t)obj->getEid());
}

void EntityObjEditor::unmapEidFromEditableObject(EntityObj *obj)
{
  if (obj == nullptr || newObj == obj || obj->getEid() == ecs::INVALID_ENTITY_ID)
    return;

  const ska::hash_size_t result = eidToEntityObj.erase(obj->getEid());
  if (result == 0)
    logerr("[%s] Entity with eid <%i> not exist in eidToEntityObj.", __FUNCTION__, (ecs::entity_id_t)obj->getEid());
}

void EntityObjEditor::_addObjects(EditableObject **obj, int num, bool use_undo)
{
  ObjectEditor::_addObjects(obj, num, use_undo);
  for (int i = 0; i < num; ++i)
  {
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(obj[i]))
      mapEidToEditableObject(entityObj);
    else if (SceneObj *sceneObj = RTTI_cast<SceneObj>(obj[i]))
    {
      createdSceneObjs.emplace(sceneObj->getSceneId());
      sceneIdToEObj.emplace(sceneObj->getSceneId(), sceneObj);
    }
  }
}

void EntityObjEditor::_removeObjects(EditableObject **obj, int num, bool use_undo)
{
  for (int i = 0; i < num; ++i)
  {
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(obj[i]))
      unmapEidFromEditableObject(entityObj);
    else if (SceneObj *sceneObj = RTTI_cast<SceneObj>(obj[i]))
    {
      sceneIdToEObj.erase(sceneObj->getSceneId());
      createdSceneObjs.erase(sceneObj->getSceneId());
    }
  }
  ObjectEditor::_removeObjects(obj, num, use_undo);
}

void EntityObjEditor::saveAddTemplate(ecs::EntityId eid, const char *templ_name)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  erec->templateName = add_sub_template_name(erec->templateName.c_str(), templ_name).c_str();
  removeSelectedTemplateName(erec->templateName);
  scene.setNewChangesApplied(erec->sceneId);
}

EditableObject *EntityObjEditor::getObjectByEid(ecs::EntityId eid) const
{
  auto find = eidToEntityObj.find(eid);
  return find != eidToEntityObj.end() ? find->second : nullptr;
}

EditableObject *EntityObjEditor::getSceneObjectById(ecs::Scene::SceneId sid) const
{
  auto find = sceneIdToEObj.find(sid);
  return find != sceneIdToEObj.end() ? find->second : nullptr;
}

uint32_t EntityObjEditor::writeSceneToBlk(DataBlock &blk, const ecs::Scene::SceneRecord &record, EntityObjEditor &editor)
{
  const bool isMain = record.importDepth == 0;
  eastl::vector<SceneItemSaveObj, framemem_allocator> saveItems;
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

    saveItems.push_back(SceneItemSaveObj{.entry = entry,
      .erec = erec,
      .position = i,
      .priority = entry.isEntity && erec ? calc_save_priority(erec->templateName.c_str(), editor.getSaveOrderRules())
                                         : editor.getSaveOrderRules().size()});
  }

  if (isMain)
  {
    editor.forEachEntityObj([&](EntityObj *o) {
      if (auto erec = ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid()); erec)
      {
        if (record.entities.find(o->getEid()) == record.entities.end() && erec->loadType == ecs::Scene::UNKNOWN &&
            erec->sceneId == ecs::Scene::C_INVALID_SCENE_ID)
        {
          saveItems.push_back(SceneItemSaveObj{.entry = ecs::Scene::SceneRecord::OrderedEntry{.eid = o->getEid(), .isEntity = true},
            .erec = erec,
            .position = saveItems.size(),
            .priority = calc_save_priority(erec->templateName.c_str(), editor.getSaveOrderRules())});
        }
      }
    });
  }

  for (const SceneItemSaveObj &item : saveItems)
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
        if (find_sub_template_name(templateName, HIERARCHIAL_FREE_TRANSFORM_TEMPLATE))
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

  for (const SceneItemSaveObj &item : saveItems)
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

        EntityObj::save(*entityBlock, recordCopy);
      }
      else
      {
        EntityObj::save(*entityBlock, *item.erec);
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
            auto finalTemplateName = remove_sub_template_name(templateName, HIERARCHIAL_FREE_TRANSFORM_TEMPLATE);
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

bool EntityObjEditor::isEntityHidden(ecs::EntityId eid) const
{
  if (EditableObject *eo = getObjectByEid(eid))
  {
    return eo->isHidden();
  }

  return false;
}

bool EntityObjEditor::isSceneHidden(ecs::Scene::SceneId sid) const
{
  if (EditableObject *eo = getSceneObjectById(sid))
  {
    return eo->isHidden();
  }

  return false;
}

bool EntityObjEditor::isEntityLocked(ecs::EntityId eid) const
{
  if (EditableObject *eo = getObjectByEid(eid))
  {
    return eo->isLocked();
  }

  return false;
}

bool EntityObjEditor::isSceneLocked(ecs::Scene::SceneId sid) const
{
  if (EditableObject *eo = getSceneObjectById(sid))
  {
    return eo->isLocked();
  }

  return false;
}

bool EntityObjEditor::isSceneInLockedHierarchy(ecs::Scene::SceneId sid) const
{
  ecs::Scene::SceneId currentScene = sid;
  while (currentScene != ecs::Scene::C_INVALID_SCENE_ID)
  {
    if (EditableObject *eo = getSceneObjectById(currentScene); eo && eo->isLocked())
    {
      return true;
    }

    ecs::Scene::SceneRecord *srecord = ecs::g_scenes->getActiveScene().getSceneRecordById(currentScene);
    if (!srecord)
    {
      break;
    }

    currentScene = srecord->parent;
  }

  return false;
}

void EntityObjEditor::saveDelTemplate(ecs::EntityId eid, const char *templ_name, bool use_undo)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  ecs::Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
  if (!erec)
    return;
  const eastl::string newTemplName = remove_sub_template_name(erec->templateName.c_str(), templ_name).c_str();
  if (erec->templateName == newTemplName)
    return;

  UndoDelTemplate *undo = nullptr;
  if (use_undo && DAEDITOR4.undoSys.is_holding())
  {
    EntityObj *entityObj = RTTI_cast<EntityObj>(get_entity_obj_editor()->getObjectByEid(eid));
    if (entityObj)
      undo = new UndoDelTemplate(templ_name, entityObj);
  }
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
  scene.setNewChangesApplied(erec->sceneId);
  if (undo)
    DAEDITOR4.undoSys.put(undo);
}

bool EntityObjEditor::getAxes(Point3 &ax, Point3 &ay, Point3 &az)
{
  IDaEditor4Engine::BasisType basis = DAEDITOR4.getGizmoBasisType();

  if (basis == IDaEditor4Engine::BASIS_parent && selection.size() > 0)
  {
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(selection[0]))
      if (EntityObj *parentObject = entityObj->getParentObject())
      {
        const TMatrix tm = parentObject->getWtm();
        ax = normalize(tm.getcol(0));
        ay = normalize(tm.getcol(1));
        az = normalize(tm.getcol(2));
        return true;
      }

    basis = IDaEditor4Engine::BASIS_local;
  }

  if (basis == IDaEditor4Engine::BASIS_local && selection.size() > 0)
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

void EntityObjEditor::handleKeyPress(int dkey, int modif)
{
  const bool isAlt = DAEDITOR4.isAltKeyPressed();
  const bool isCtrl = DAEDITOR4.isCtrlKeyPressed();
  const bool isShift = DAEDITOR4.isShiftKeyPressed();

  if (!isAlt && !isCtrl && !isShift && dkey == HumanInput::DKEY_X && !isGizmoStarted &&
      (getEditMode() == CM_OBJED_MODE_MOVE || getEditMode() == CM_OBJED_MODE_SURF_MOVE || getEditMode() == CM_OBJED_MODE_ROTATE ||
        getEditMode() == CM_OBJED_MODE_SCALE))
  {
    switch (DAEDITOR4.getGizmoBasisType())
    {
      case IDaEditor4Engine::BASIS_none:
      case IDaEditor4Engine::BASIS_world:
        DAEDITOR4.setGizmoBasisType(IDaEditor4Engine::BASIS_local);
        updateGizmo();
        break;
      case IDaEditor4Engine::BASIS_local:
        DAEDITOR4.setGizmoBasisType(IDaEditor4Engine::BASIS_parent);
        updateGizmo();
        break;
      case IDaEditor4Engine::BASIS_parent:
        DAEDITOR4.setGizmoBasisType(IDaEditor4Engine::BASIS_world);
        updateGizmo();
        break;
    }

    return;
  }

  ObjectEditor::handleKeyPress(dkey, modif);
}

void EntityObjEditor::updateSingleHierarchyTransform(EditableObject &editable_object, bool calculate_from_relative_transform)
{
  EntityObj *object = RTTI_cast<EntityObj>(&editable_object);
  if (!object)
    return;

  const ecs::EntityId eid = object->getEid();
  TMatrix *transform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("transform"));
  if (!transform)
    return;

  TMatrix *hierarchyTransform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("hierarchy_transform"));
  if (!hierarchyTransform)
    return;

  TMatrix *hierarchyParentLastTransform = g_entity_mgr->getNullableRW<TMatrix>(eid, ECS_HASH("hierarchy_parent_last_transform"));
  if (!hierarchyParentLastTransform)
    return;

  const EntityObj *parentObject = object->getParentObject();
  const TMatrix parentTm = parentObject ? parentObject->getWtm() : TMatrix::IDENT;

  if (calculate_from_relative_transform)
  {
    TMatrix newTransform = parentTm * *hierarchyTransform;
    object->setWtm(newTransform);
    *transform = newTransform; // Needed because setWtm() only sets the transform in ECS if the object is selected.
  }
  else
  {
    if (parentObject)
      *hierarchyTransform = inverse(parentTm) * object->getWtm();

    *transform = object->getWtm(); // Needed because setWtm() only sets the transform in ECS if the object is selected.
  }

  if (parentObject)
    *hierarchyParentLastTransform = parentTm;
}

EntityObjEditor::HierarchyItem *EntityObjEditor::makeHierarchyItemParent(EditableObject &object, HierarchyItem &hierarchy_root,
  ska::flat_hash_map<EditableObject *, HierarchyItem *> &object_to_hierarchy_item_map)
{
  auto it = object_to_hierarchy_item_map.find(&object);
  if (it != object_to_hierarchy_item_map.end())
    return it->second;

  HierarchyItem *hierarchyItemParent = &hierarchy_root;
  if (EntityObj *entityObj = RTTI_cast<EntityObj>(&object))
    if (EntityObj *objectParent = entityObj->getParentObject())
      hierarchyItemParent = makeHierarchyItemParent(*objectParent, hierarchy_root, object_to_hierarchy_item_map);

  HierarchyItem *hierarchyItem = hierarchyItemParent->addChild(object);
  auto result = object_to_hierarchy_item_map.insert({&object, hierarchyItem});
  return result.first->second;
}

void EntityObjEditor::fillObjectHierarchy(HierarchyItem &hierarchy_root)
{
  G_ASSERT(hierarchy_root.isRoot());

  ska::flat_hash_map<EditableObject *, HierarchyItem *> objectToHierarchyItemMap;

  for (EditableObject *editableObject : objects)
  {
    if (!editableObject->isLocked())
    {
      G_ASSERT(editableObject);
      makeHierarchyItemParent(*editableObject, hierarchy_root, objectToHierarchyItemMap);
    }
  }
}

void EntityObjEditor::applyChange(EditableObject &object, const Point3 &delta)
{
  switch (getEditMode())
  {
    case CM_OBJED_MODE_MOVE: object.moveObject(delta, DAEDITOR4.getGizmoBasisType()); break;
    case CM_OBJED_MODE_SURF_MOVE: object.moveSurfObject(delta, DAEDITOR4.getGizmoBasisType()); break;
    case CM_OBJED_MODE_ROTATE: object.rotateObject(-delta, gizmoOrigin, DAEDITOR4.getGizmoBasisType()); break;
    case CM_OBJED_MODE_SCALE: object.scaleObject(delta, gizmoOrigin, DAEDITOR4.getGizmoBasisType()); break;
  }
}

void EntityObjEditor::applyChangeHierarchically(const HierarchyItem &parent, const Point3 &delta, bool parent_updated)
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

void EntityObjEditor::makeTransformUndoForHierarchySelection(const HierarchyItem &parent, bool parent_updated)
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

void EntityObjEditor::changed(const Point3 &delta)
{
  // Work from the initial state because delta is total delta since startGizmo().
  G_ASSERT(hierarchicalUndoGroup);
  hierarchicalUndoGroup->undo();
  hierarchicalUndoGroup->clear();
  makeTransformUndoForHierarchySelection(temporaryHierarchyForGizmoChange);

  IDaEditor4Engine::BasisType basis = DAEDITOR4.getGizmoBasisType();

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

void EntityObjEditor::gizmoStarted()
{
  gizmoOrigin = gizmoPt;
  isGizmoStarted = true;
  temporaryHierarchyForGizmoChange.clearChildren();
  delete hierarchicalUndoGroup;
  hierarchicalUndoGroup = new HierarchicalUndoGroup();

  if (DAEDITOR4.isShiftKeyPressed() && getEditMode() == CM_OBJED_MODE_MOVE && canCloneSelection())
  {
    cloneMode = true;
    cloneDelta = getPt();
  }

  DAEDITOR4.undoSys.begin();
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

void EntityObjEditor::gizmoEnded(bool apply)
{
  // Because the non-root objects are moved by their parents we have to save their transforms after the hierarchy
  // ECS events ran.
  if (apply)
    hierarchicalUndoGroup->saveTransformComponentOfAllObjects();

  getUndoSystem()->put(hierarchicalUndoGroup);
  hierarchicalUndoGroup = nullptr;
  temporaryHierarchyForGizmoChange.clearChildren();

  ObjectEditor::gizmoEnded(apply);
}

void EntityObjEditor::undoLastOperationSq()
{
  IDaEditor4Engine::get().setSuppressUndoRedoEvents(true);
  getUndoSystem()->undo();
  IDaEditor4Engine::get().setSuppressUndoRedoEvents(false);
}

void EntityObjEditor::redoLastOperationSq()
{
  IDaEditor4Engine::get().setSuppressUndoRedoEvents(true);
  getUndoSystem()->redo();
  IDaEditor4Engine::get().setSuppressUndoRedoEvents(false);
}

void EntityObjEditor::register_script_class(HSQUIRRELVM vm)
{
  Sqrat::Class<EntityObjEditor, Sqrat::NoConstructor<EntityObjEditor>> sqEntityObjEditor(vm, "EntityObjEditor");
  sqEntityObjEditor //
    .Func("selectEntities", &EntityObjEditor::selectEntities)
    .Func("selectEcsTemplate", &EntityObjEditor::selectNewObjEntity)
    .Func("hasUnsavedChanges", &EntityObjEditor::hasUnsavedChanges)
    .Func("saveMainSceneCopy", &EntityObjEditor::saveMainSceneCopy)
    .Func("saveMainScene", &EntityObjEditor::saveMainScene)
    .Func("saveDirtyScenes", &EntityObjEditor::saveDirtyScenes)
    .Func("saveScenes", &EntityObjEditor::saveScenesSq)
    .Func("isChildScene", &EntityObjEditor::isChildScene)
    .Func("setTargetScene", &EntityObjEditor::setTargetScene)
    .Func("setFocusedEntity", &EntityObjEditor::setFocusedEntity)
    .Func("hideSelectedTemplate", &EntityObjEditor::hideSelectedTemplate)
    .Func("hideUnmarkedEntities", &EntityObjEditor::hideUnmarkedEntities)
    .Func("unhideAll", &EntityObjEditor::unhideAll)
    .Func("dropObjects", &EntityObjEditor::dropObjects)
    .Func("dropObjectsNorm", &EntityObjEditor::dropObjectsNorm)
    .Func("resetScale", &EntityObjEditor::resetScale)
    .Func("resetRotate", &EntityObjEditor::resetRotate)
    .Func("zoomAndCenter", &EntityObjEditor::zoomAndCenter)
    .Func("setParentForSelection", &EntityObjEditor::setParentForSelection)
    .Func("clearParentForSelection", &EntityObjEditor::clearParentForSelection)
    .Func("toggleFreeTransformForSelection", &EntityObjEditor::toggleFreeTransformForSelection)
    .Func("deleteSelectedObjects", &EntityObjEditor::deleteSelectedObjects)
    .SquirrelFunc("getEntities", &EntityObjEditor::get_entites, 2, "xs")
    .SquirrelFunc("getSceneEntities", &EntityObjEditor::get_scene_entities, 1, "x")
    .SquirrelFunc("getSceneImports", &EntityObjEditor::get_scene_imports, 1, "x")
    .SquirrelFunc("getSceneRecord", &EntityObjEditor::get_scene_record, 2, "xi")
    .SquirrelFunc("getTargetScene", &EntityObjEditor::get_target_scene, 1, "x")
    .SquirrelFunc("getEcsTemplates", &EntityObjEditor::get_ecs_templates, 2, "xs")
    .SquirrelFunc("getEcsTemplatesGroups", &EntityObjEditor::get_ecs_templates_groups, 1, "x")
    .SquirrelFunc("getSceneOrderedEntries", &EntityObjEditor::get_scene_ordered_entries, 2, "xi")
    .Func("isSceneEntity", &EntityObjEditor::isSceneEntity)
    .Func("checkSceneEntities", &EntityObjEditor::checkSceneEntities)
    .Func("getEntityRecordLoadType", &EntityObjEditor::getEntityRecordLoadType)
    .Func("getEntityRecordSceneId", &EntityObjEditor::getEntityRecordSceneId)
    .Func("addImportScene", &EntityObjEditor::addImportScene)
    .Func("reCreateEditorEntity", &EntityObjEditor::reCreateEditorEntity)
    .Func("makeSingletonEntity", &EntityObjEditor::makeSingletonEntity)
    .Func("selectEntity", &EntityObjEditor::selectEntity)
    .Func("isEntityTransformable", &EntityObjEditor::isEntityTransformable)
    .Func("isSceneInTransformableHierarchy", &EntityObjEditor::isSceneInTransformableHierarchy)
    .Func("isSceneTransformable", &EntityObjEditor::isSceneTransformable)
    .Func("setSceneTransformable", &EntityObjEditor::setSceneTransformable)
    .Func("getScenePivot", &EntityObjEditor::getScenePivot)
    .Func("setScenePivot", &EntityObjEditor::setScenePivot)
    .Func("setScenePrettyName", &EntityObjEditor::setScenePrettyName)
    .Func("getScenePrettyName", &EntityObjEditor::getScenePrettyName)
    .Func("setSceneNewParent", &EntityObjEditor::setSceneNewParent)
    .Func("setSceneNewParentAndOrder", &EntityObjEditor::setSceneNewParentAndOrder)
    .Func("setSceneOrder", &EntityObjEditor::setSceneOrder)
    .Func("getSceneOrder", &EntityObjEditor::getSceneOrder)
    .Func("getSceneEntityCount", &EntityObjEditor::getSceneEntityCount)
    .Func("removeObjects", &EntityObjEditor::removeObjectsSq)
    .Func("hideObjects", &EntityObjEditor::hideObjectsSq)
    .Func("unhideObjects", &EntityObjEditor::unhideObjectsSq)
    .Func("lockObjects", &EntityObjEditor::lockObjectsSq)
    .Func("unlockObjects", &EntityObjEditor::unlockObjectsSq)
    .Func("isEntityHidden", &EntityObjEditor::isEntityHidden)
    .Func("isSceneHidden", &EntityObjEditor::isSceneHidden)
    .Func("isEntityLocked", &EntityObjEditor::isEntityLocked)
    .Func("isSceneLocked", &EntityObjEditor::isSceneLocked)
    .Func("isSceneInLockedHierarchy", &EntityObjEditor::isSceneInLockedHierarchy)
    .SquirrelFunc("getModifiedSceneIds", &EntityObjEditor::get_modified_scene_ids, 1, "x")
    .Func("selectObjects", &EntityObjEditor::selectObjectsSq)
    .Func("undoLastOperation", &EntityObjEditor::undoLastOperationSq)
    .Func("redoLastOperation", &EntityObjEditor::redoLastOperationSq)
    /**/;
}

void entity_obj_editor_for_each_entity(EntityObjEditor &editor, eastl::fixed_function<sizeof(void *), void(EntityObj *)> cb)
{
  editor.forEachEntityObj(cb);
}
