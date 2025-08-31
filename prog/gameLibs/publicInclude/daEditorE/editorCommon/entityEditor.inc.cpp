// required externals:
//   void select_entity_webui(const ecs::EntityId &eid)

#include "entityEditor.h"
#include "entityObj.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
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
#include <ecs/core/utility/ecsRecreate.h>
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

static void removeSelectedTemplateName(eastl::string &templ_name)
{
  templ_name = remove_sub_template_name(templ_name.c_str(), SELECTED_TEMPLATE);
}

static const ecs::HashedConstString EDITABLE_TEMPLATE_COMP = ECS_HASH("editableTemplate");

static inline bool is_entity_editable(ecs::EntityId eid) { return g_entity_mgr->getOr(eid, ECS_HASH("editableObj"), true); }

static inline bool is_scene_entity(EntityObj *o)
{
  if (!o)
    return false;
  const auto *erec = ecs::g_scenes->getActiveScene().findEntityRecord(o->getEid());
  return erec && erec->toBeSaved;
}

static inline bool can_transform_object_freely(EditableObject *object)
{
  if (EntityObj *entityObj = RTTI_cast<EntityObj>(object))
    return entityObj->canTransformFreely();

  return true;
}

// Check if setting the child's parent to parent would result in a loop.
static bool is_safe_to_set_parent(EntityObj &child, EntityObj &parent)
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

static const char *check_singleton_mutex_exists(const ecs::Template *templ, const ecs::TemplatesData &templates_data)
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
        o->replaceEid(createEntitySample(&cd));
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
    no = o->cloneObject(templName.c_str());
    if (!no)
      return;
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

    objsToRemove.clear();
    DAEDITOR4.undoSys.clear();
  }
}

struct EntityObjSaveRec
{
  const EntityObj *obj;
  const ecs::Scene::EntityRecord *erec;
  int priority;
  bool operator<(const EntityObjSaveRec &rhs) const
  {
    if (priority != rhs.priority)
      return priority < rhs.priority;
    return erec->order < rhs.erec->order;
  }
};

static int calc_save_priority(const char *template_name, const EntityObjEditor::SaveOrderRules &rules)
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

int EntityObjEditor::saveObjectsInternal(const char *fpath, bool save_child_scenes)
{
  if (!fpath || !*fpath)
    return -1;

  const ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();

  const ecs::Scene::ScenesList &importsRecord = saveScene.getImportsRecordList();
  for (uint32_t importsIdx = 0; importsIdx < importsRecord.size(); ++importsIdx)
  {
    if (importsRecord[importsIdx].importDepth == 0)
      return saveSceneObjectsInternal(fpath, importsIdx, ecs::Scene::IMPORT, save_child_scenes);
  }

  return -1;
}

int EntityObjEditor::saveSceneObjectsInternal(const char *fpath, uint32_t scene_idx, uint32_t load_type, bool save_child_scenes)
{
  if (!fpath || !*fpath)
    return -1;

  const ecs::Scene &saveScene = ecs::g_scenes->getActiveScene();

  const ecs::Scene::ScenesList *sceneRecords = nullptr;
  if (load_type == ecs::Scene::COMMON)
    sceneRecords = &saveScene.getCommonRecordList();
  else if (load_type == ecs::Scene::CLIENT)
    sceneRecords = &saveScene.getClientRecordList();
  else if (load_type == ecs::Scene::IMPORT)
    sceneRecords = &saveScene.getImportsRecordList();
  else
    return -1;

  G_ASSERT(scene_idx < sceneRecords->size());
  const ecs::Scene::SceneRecord *sceneRecord = &sceneRecords->at(scene_idx);
  const bool isMain = sceneRecord->importDepth == 0;
  if (!isMain && !sceneRecord->editable)
  {
    if (save_child_scenes)
      return saveChildSceneObjectsInternal(sceneRecords, scene_idx, load_type);
    else
      return -1;
  }

  eastl::vector<EntityObjSaveRec, framemem_allocator> saveEntities;
  saveEntities.reserve(saveScene.getNumToBeSavedEntities());
  const ecs::TemplateDB &templates = g_entity_mgr->getTemplateDB();
  int hierarchySaveId = 2;
  ska::flat_hash_map<ecs::EntityId, int, ecs::EidHash> hiearchySaveIds;
  forEachEntityObj([&](EntityObj *o) {
    if (o->isValid())
    {
      eastl::vector<eastl::string, framemem_allocator> subTemplsToRemove;
      if (const char *entTempl = g_entity_mgr->getEntityTemplateName(o->getEid()))
        if (const ecs::Template *templ = templates.getTemplateByName(entTempl))
          for (const uint32_t p : templ->getParents())
            if (auto *t = g_entity_mgr->getTemplateDB().getTemplateById(p))
              if (auto *cc = t->getComponentsMap().find(EDITABLE_TEMPLATE_COMP))
                if (!cc->getOr(true))
                  subTemplsToRemove.push_back(t->getName());
      for (const auto &subTempl : subTemplsToRemove)
        remove_sub_template_async(o->getEid(), subTempl.c_str());
    }

    if (auto erec = saveScene.findEntityRecord(o->getEid()); erec && erec->toBeSaved)
    {
      // skip entities not tied to the specific scene (except for the new entities and the main scene)
      if (sceneRecord->entities.find(o->getEid()) == sceneRecord->entities.end())
        if (!isMain || !(erec->loadType == ecs::Scene::UNKNOWN && erec->sceneIdx == 0))
          return;

      saveEntities.push_back(EntityObjSaveRec{o, erec, calc_save_priority(erec->templateName.c_str(), saveOrderRules)});

      const ecs::EntityId *hierarchyEid = g_entity_mgr->getNullable<ecs::EntityId>(o->getEid(), ECS_HASH("hierarchy_eid"));
      if (hierarchyEid && *hierarchyEid)
      {
        int finalHierarchySaveId = hierarchySaveId;
        G_ASSERT((finalHierarchySaveId & 1) == 0);

        const char *templateName = g_entity_mgr->getEntityTemplateName(o->getEid());
        if (find_sub_template_name(templateName, HIERARCHIAL_FREE_TRANSFORM_TEMPLATE))
          finalHierarchySaveId |= 1;

        bool inserted;
        eastl::tie(eastl::ignore, inserted) = hiearchySaveIds.insert({o->getEid(), finalHierarchySaveId});
        G_ASSERT(inserted);
        hierarchySaveId += 2;
      }
    }
  });
  g_entity_mgr->tick();                                  // so we apply all remove sub template
  eastl::sort(saveEntities.begin(), saveEntities.end()); // sort by save priority and scene order, new entites go last

  DataBlock blk;

  const auto importsEnd = sceneRecords->end();
  const int minPriority = saveOrderRules.size();
  auto importsIt = sceneRecords->begin();
  for (auto &sr : saveEntities)
  {
    while (importsIt != importsEnd && (importsIt->order == ecs::Scene::SceneRecord::TOP_IMPORT_ORDER ||
                                        (importsIt->order < sr.erec->order && sr.priority == minPriority)))
    {
      if (importsIt->parent == scene_idx)
        blk.addNewBlock("import")->addStr("scene", importsIt->path.c_str());
      ++importsIt;
    }

    DataBlock *entityBlock = blk.addNewBlock("entity");
    sr.obj->save(*entityBlock, *sr.erec);

    entityBlock->removeParam("hierarchy_unresolved_id");
    entityBlock->removeParam("hierarchy_unresolved_parent_id");

    auto hiearchySaveIdIt = hiearchySaveIds.find(sr.obj->getEid());
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

      const ecs::EntityId *parentId = g_entity_mgr->getNullable<ecs::EntityId>(sr.obj->getEid(), ECS_HASH("hierarchy_parent"));
      if (parentId && *parentId)
      {
        auto hiearchySaveIdParentIt = hiearchySaveIds.find(*parentId);
        if (hiearchySaveIdParentIt != hiearchySaveIds.end())
          entityBlock->addInt("hierarchy_unresolved_parent_id", hiearchySaveIdParentIt->second);
      }
    }
  }

  for (; importsIt != importsEnd; ++importsIt)
    if (importsIt->parent == scene_idx)
      blk.addNewBlock("import")->setStr("scene", importsIt->path.c_str());

  if (dd_file_exist(fpath))
    fpath = df_get_abs_fname(fpath);

  if (!blk.saveToTextFile(fpath))
    return -1;

  int savedEntityCount = 0;
  if (save_child_scenes)
    savedEntityCount = saveChildSceneObjectsInternal(sceneRecords, scene_idx, load_type);

  return savedEntityCount + (int)saveEntities.size();
}

int EntityObjEditor::saveChildSceneObjectsInternal(const ecs::Scene::ScenesList *scene_records, uint32_t scene_idx, uint32_t load_type)
{
  int savedEntityCount = 0;
  for (uint32_t importsIdx = 0; importsIdx < scene_records->size(); ++importsIdx)
  {
    const ecs::Scene::SceneRecord &importRecord = scene_records->at(importsIdx);
    if (importRecord.parent == scene_idx)
    {
      int saved = saveSceneObjectsInternal(importRecord.path.c_str(), importsIdx, load_type, true);
      if (saved != -1)
        savedEntityCount += saved;
    }
  }
  return savedEntityCount;
}

void EntityObjEditor::saveObjectsCopy(const char *fpath, const char *reason, bool save_child_scenes)
{
  if (!reason || !*reason)
    reason = "copy";

  if (!fpath || !*fpath)
  {
    logerr("daEd4: bad scene saving path '%s' (for %s)", fpath, reason);
    return;
  }

  const int numObjects = saveObjectsInternal(fpath, save_child_scenes);
  if (numObjects >= 0)
    visuallog::logmsg(String(0, "daEd4: Scene copy saved successfully (for %s)", reason));
  else
    logerr("daEd4: scene copy saving to '%s' failed (for %s)", fpath, reason);
}
void EntityObjEditor::saveObjects(const char *new_fpath, bool save_child_scenes)
{
  if (new_fpath && *new_fpath)
    sceneFilePath = new_fpath;

  int childScenesSaved = 0;
  if (save_child_scenes)
    childScenesSaved = getEditableScenesCount();

  const int numObjects = saveObjectsInternal(sceneFilePath.c_str(), save_child_scenes);
  if (numObjects >= 0)
  {
    ecs::g_scenes->getActiveScene().setAllChangesWereSaved(save_child_scenes);
    console::print_d("daEd4: %d objects saved to: %s", numObjects, sceneFilePath);
    if (save_child_scenes && childScenesSaved > 0)
    {
      console::print_d("daEd4: %d IMPORT scene(s) saved", childScenesSaved);
      visuallog::logmsg(String(0, "daEd4: MAIN scene and %d IMPORT scene(s) saved successfully", childScenesSaved));
    }
    else
      visuallog::logmsg("daEd4: MAIN scene saved successfully");
  }
  else
    logerr("daEd4: scene saving to '%s' failed", sceneFilePath);
}

bool EntityObjEditor::hasUnsavedChanges() const { return ecs::g_scenes->getActiveScene().hasUnsavedChanges(); }

bool EntityObjEditor::hasUnsavedChildScenes() const { return ecs::g_scenes->getActiveScene().hasUnsavedChildScenes(); }

bool EntityObjEditor::isChildScene(uint32_t load_type, uint32_t scene_idx)
{
  return ecs::g_scenes->getActiveScene().isChildScene(load_type, scene_idx);
}

void EntityObjEditor::setChildSceneEditable(uint32_t load_type, uint32_t scene_idx, bool enable)
{
  ecs::g_scenes->setChildSceneEditable(load_type, scene_idx, enable);
}

int EntityObjEditor::getEditableScenesCount() const
{
  const ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesList &imports = scene.getImportsRecordList();
  int c = 0;
  for (auto &record : imports)
  {
    if (record.editable && record.importDepth != 0)
      c++;
  }
  return c;
}

void EntityObjEditor::setTargetScene(uint32_t load_type, uint32_t scene_idx)
{
  ecs::g_scenes->getActiveScene().setTargetSceneId(load_type, scene_idx);
}

bool EntityObjEditor::processCommand(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("daEd4", "saveObjects", 1, 2) { saveObjects(argc > 1 ? argv[1] : NULL); } //-V547
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
  if (!erec || !erec->toBeSaved)
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

int EntityObjEditor::getEntityRecordIndex(ecs::EntityId eid)
{
  auto *record = ecs::g_scenes->getActiveScene().findEntityRecord(eid);
  if (record)
    return record->sceneIdx;

  return -1;
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


Sqrat::Array EntityObjEditor::getEntities(HSQUIRRELVM vm)
{
  if (orderedTemplatesGroups.empty())
    initDefaultTemplatesGroups();
  const TemplatesGroup *pTemplatesGroup = nullptr;
  const SQChar *groupName = nullptr;
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

static Sqrat::Table sceneRecordToSqTable(HSQUIRRELVM vm, const ecs::Scene::ScenesList &records, int index)
{
  G_ASSERT(0 <= index && index < records.size());
  Sqrat::Table ret(vm);
  const ecs::Scene::SceneRecord &record = records[index];
  ret.SetValue("loadType", (int)record.loadType);
  ret.SetValue("index", index);
  ret.SetValue("entityCount", (int)record.entities.size());
  ret.SetValue("order", (int)record.order);
  ret.SetValue("path", record.path);
  ret.SetValue("importDepth", record.importDepth);
  ret.SetValue("parent", record.parent);
  ret.SetValue("hasParent", (bool)(record.parent != ecs::Scene::SceneRecord::NO_PARENT));
  ret.SetValue("imports", record.imports);
  ret.SetValue("editable", (bool)record.editable);
  return ret;
}

Sqrat::Array EntityObjEditor::getSceneImports(HSQUIRRELVM vm)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesList &common = scene.getCommonRecordList();
  const ecs::Scene::ScenesList &client = scene.getClientRecordList();
  const ecs::Scene::ScenesList &imports = scene.getImportsRecordList();
  Sqrat::Array res(vm, common.size() + client.size() + imports.size());
  int c = 0;
  for (int i = 0; i < common.size(); ++i)
    res.SetValue(SQInteger(c++), sceneRecordToSqTable(vm, common, i));
  for (int i = 0; i < client.size(); ++i)
    res.SetValue(SQInteger(c++), sceneRecordToSqTable(vm, client, i));
  for (int i = 0; i < imports.size(); ++i)
    res.SetValue(SQInteger(c++), sceneRecordToSqTable(vm, imports, i));
  return res;
}

Sqrat::Table EntityObjEditor::getSceneRecord(HSQUIRRELVM vm)
{
  SQInteger loadType;
  SQInteger index;
  sq_getinteger(vm, SQInteger(2), &loadType);
  sq_getinteger(vm, SQInteger(3), &index);
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  if (loadType == ecs::Scene::COMMON)
    return sceneRecordToSqTable(vm, scene.getCommonRecordList(), index);
  if (loadType == ecs::Scene::CLIENT)
    return sceneRecordToSqTable(vm, scene.getClientRecordList(), index);
  if (loadType == ecs::Scene::IMPORT)
    return sceneRecordToSqTable(vm, scene.getImportsRecordList(), index);

  Sqrat::Table ret(vm);
  return ret;
}

Sqrat::Table EntityObjEditor::getTargetScene(HSQUIRRELVM vm)
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  uint32_t loadType, index;
  scene.getTargetSceneId(loadType, index);
  if (loadType == ecs::Scene::COMMON)
    return sceneRecordToSqTable(vm, scene.getCommonRecordList(), index);
  if (loadType == ecs::Scene::CLIENT)
    return sceneRecordToSqTable(vm, scene.getClientRecordList(), index);
  if (loadType == ecs::Scene::IMPORT)
    return sceneRecordToSqTable(vm, scene.getImportsRecordList(), index);

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

  const SQChar *groupName;
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
  if (!erec || !erec->toBeSaved)
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
  const bool child_scene = scene.isChildScene(erec->loadType, erec->sceneIdx);
  if (erec->toBeSaved || child_scene)
    scene.setNewChangesApplied(child_scene);
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
  const bool child_scene = scene.isChildScene(erec->loadType, erec->sceneIdx);
  if (erec->toBeSaved || child_scene)
    scene.setNewChangesApplied(child_scene);
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
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(obj[i]))
      mapEidToEditableObject(entityObj);
}

void EntityObjEditor::_removeObjects(EditableObject **obj, int num, bool use_undo)
{
  for (int i = 0; i < num; ++i)
    if (EntityObj *entityObj = RTTI_cast<EntityObj>(obj[i]))
      unmapEidFromEditableObject(entityObj);
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
  const bool child_scene = scene.isChildScene(erec->loadType, erec->sceneIdx);
  if (erec->toBeSaved || child_scene)
    scene.setNewChangesApplied(child_scene);
}

EditableObject *EntityObjEditor::getObjectByEid(ecs::EntityId eid) const
{
  auto find = eidToEntityObj.find(eid);
  return find != eidToEntityObj.end() ? find->second : nullptr;
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
  const bool child_scene = scene.isChildScene(erec->loadType, erec->sceneIdx);
  if (erec->toBeSaved || child_scene)
    scene.setNewChangesApplied(child_scene);
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
    G_ASSERT(editableObject);
    makeHierarchyItemParent(*editableObject, hierarchy_root, objectToHierarchyItemMap);
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

void EntityObjEditor::register_script_class(HSQUIRRELVM vm)
{
  Sqrat::Class<EntityObjEditor, Sqrat::NoConstructor<EntityObjEditor>> sqEntityObjEditor(vm, "EntityObjEditor");
  sqEntityObjEditor //
    .Func("selectEntities", &EntityObjEditor::selectEntities)
    .Func("selectEcsTemplate", &EntityObjEditor::selectNewObjEntity)
    .Func("hasUnsavedChanges", &EntityObjEditor::hasUnsavedChanges)
    .Func("hasUnsavedChildScenes", &EntityObjEditor::hasUnsavedChildScenes)
    .Func("isChildScene", &EntityObjEditor::isChildScene)
    .Func("setChildSceneEditable", &EntityObjEditor::setChildSceneEditable)
    .Func("getEditableScenesCount", &EntityObjEditor::getEditableScenesCount)
    .Func("setTargetScene", &EntityObjEditor::setTargetScene)
    .Func("saveObjects", &EntityObjEditor::saveObjects)
    .Func("saveObjectsCopy", &EntityObjEditor::saveObjectsCopy)
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
    .SquirrelFunc("getSceneRecord", &EntityObjEditor::get_scene_record, 3, "xii")
    .SquirrelFunc("getTargetScene", &EntityObjEditor::get_target_scene, 1, "x")
    .SquirrelFunc("getEcsTemplates", &EntityObjEditor::get_ecs_templates, 2, "xs")
    .SquirrelFunc("getEcsTemplatesGroups", &EntityObjEditor::get_ecs_templates_groups, 1, "x")
    .Func("isSceneEntity", &EntityObjEditor::isSceneEntity)
    .Func("checkSceneEntities", &EntityObjEditor::checkSceneEntities)
    .Func("getEntityRecordLoadType", &EntityObjEditor::getEntityRecordLoadType)
    .Func("getEntityRecordIndex", &EntityObjEditor::getEntityRecordIndex)
    .Func("reCreateEditorEntity", &EntityObjEditor::reCreateEditorEntity)
    .Func("makeSingletonEntity", &EntityObjEditor::makeSingletonEntity)
    .Func("selectEntity", &EntityObjEditor::selectEntity)
    /**/;
}

void entity_obj_editor_for_each_entity(EntityObjEditor &editor, eastl::fixed_function<sizeof(void *), void(EntityObj *)> cb)
{
  editor.forEachEntityObj(cb);
}
