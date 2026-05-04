//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/de_objEditor.h>
#include <daECS/core/template.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>
#include <daECS/scene/scene.h>
#include <util/dag_console.h>
#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <sqrat.h>

ECS_UNICAST_EVENT_TYPE(EditorToDeleteEntitiesEvent, ecs::EidList /*linkedEntitiesToDelete*/)
ECS_UNICAST_EVENT_TYPE(EditorOnInitClonedEntity)

struct EntCreateData;
class EntityObj;
class HierarchicalUndoGroup;
enum
{
  CM_OBJED_MODE_CREATE_ENTITY = CM_OBJED_LAST + 1,
};


class EntityObjEditor : public ObjectEditor, public console::ICommandProcessor
{
public:
  EntityObjEditor();
  ~EntityObjEditor();

public:
  virtual EditableObject *cloneObject(EditableObject *o, bool use_undo);
  virtual bool canCloneSelection() { return true; }

  virtual bool processCommand(const char *argv[], int argc);
  virtual void destroy() {}

  virtual void onObjectFlagsChange(EditableObject *obj, int changed_flags);
  virtual void selectionChanged();

  virtual void setWorkMode(const char *mode);

  virtual void setEditMode(int cm);
  virtual void createObjectBySample(EditableObject *sample);
  virtual bool cancelCreateMode();
  virtual void beforeRender();
  virtual void render(const Frustum &, const Point3 &);

  virtual bool pickObjects(int x, int y, Tab<EditableObject *> &objs);

  virtual void fillDeleteDepsObjects(Tab<EditableObject *> &list) const;

  bool getAxes(Point3 &ax, Point3 &ay, Point3 &az) override;
  void handleKeyPress(int dkey, int modif) override;

  void setupNewScene(const char *fpath);
  const char *getSceneFilePath() { return sceneFilePath; }

  typedef eastl::vector<eastl::string> SaveOrderRules;
  void setSaveOrderRules(const SaveOrderRules &rules) { saveOrderRules = rules; }
  const SaveOrderRules &getSaveOrderRules() const { return saveOrderRules; }

  void setMainScenePath(const char *fpath);
  void saveMainSceneCopy(const char *fpath, const char *reason);
  void saveMainScene();
  void saveDirtyScenes();
  void saveScenes(const eastl::vector<ecs::Scene::SceneId> scenes_to_save);

  void saveEntityToBlk(ecs::EntityId eid, DataBlock &blk) const;
  bool hasUnsavedChanges() const;
  bool isChildScene(ecs::Scene::SceneId id);
  void setTargetScene(ecs::Scene::SceneId id);

  void hideSelectedTemplate();
  void hideUnmarkedEntities(Sqrat::Array eids_arr);
  void unhideAll();

  void zoomAndCenter();

  void clearTemplatesGroups();
  void addTemplatesGroupRequire(const char *group_name, const char *require);
  void addTemplatesGroupVariant(const char *group_name, const char *variant);
  void addTemplatesGroupTSuffix(const char *group_name, const char *tsuffix);
  void initDefaultTemplatesGroups();

  void addEntity(ecs::EntityId eid) { addEntityEx(eid, false); }
  void addEntityEx(ecs::EntityId eid, bool use_undo);
  ecs::EntityId createEntitySample(EntCreateData *data);
  ecs::EntityId createEntityDirect(EntCreateData *data);
  void destroyEntityDirect(ecs::EntityId eid);
  void selectEntity(ecs::EntityId eid, bool selected);
  void setFocusedEntity(ecs::EntityId eid);
  ecs::EntityId getFirstSelectedEntity();

  void initSceneObjects();
  void addScene(ecs::Scene::SceneId sid);
  void removeScene(ecs::Scene::SceneId sid);

  void setParentForSelection();
  void clearParentForSelection();
  void toggleFreeTransformForSelection();

  virtual void update(float dt);
  void scheduleObjRemoval(EditableObject *o) { objsToRemove.push_back(o); }

  static void register_script_class(HSQUIRRELVM vm);

  static eastl::string getTemplateNameForUI(ecs::EntityId eid);
  static bool hasTemplateComponent(const char *template_name, const char *comp_name);
  static bool getSavedComponents(ecs::EntityId eid, eastl::vector<eastl::string> &out_comps);
  static void resetComponent(ecs::EntityId eid, const char *comp_name);
  static void saveComponent(ecs::EntityId eid, const char *comp_name);
  static void getChangedComponentsToDataBlock(ecs::EntityId eid, DataBlock &blk);
  static void saveAddTemplate(ecs::EntityId eid, const char *templ_name);
  static void saveDelTemplate(ecs::EntityId eid, const char *templ_name, bool use_undo = false);

  static bool isPickingRendinst();
  void refreshEids();

  static bool checkSingletonMutexExists(const char *mutex_name);

  template <typename F>
  void forEachEntityObjWithTemplate(F fn);
  template <typename F>
  void forEachEntityObj(F fn);
  template <typename F>
  void forEachSceneObj(F fn);

  EditableObject *getObjectByEid(ecs::EntityId eid) const;
  EditableObject *getSceneObjectById(ecs::Scene::SceneId sid) const;

  static uint32_t writeSceneToBlk(DataBlock &blk, const ecs::Scene::SceneRecord &record, EntityObjEditor &editor);

  bool isEntityHidden(ecs::EntityId eid) const;
  bool isSceneHidden(ecs::Scene::SceneId sid) const;

  bool isEntityLocked(ecs::EntityId eid) const;
  bool isSceneLocked(ecs::Scene::SceneId sid) const;
  bool isSceneInLockedHierarchy(ecs::Scene::SceneId sid) const;

protected:
  virtual void cloneSelection() override;
  virtual void _addObjects(EditableObject **obj, int num, bool use_undo) override;
  virtual void _removeObjects(EditableObject **obj, int num, bool use_undo) override;

private:
  struct HierarchyItem
  {
    HierarchyItem() : object(nullptr), parent(nullptr) {}
    HierarchyItem(EditableObject *in_object, HierarchyItem *in_parent) : object(in_object), parent(in_parent) {}
    ~HierarchyItem() { clearChildren(); }

    void clearChildren() { clear_all_ptr_items(children); }

    HierarchyItem *addChild(EditableObject &child_object)
    {
      HierarchyItem *child = new HierarchyItem(&child_object, this);
      children.push_back(child);
      return child;
    }

    bool isRoot() const { return parent == nullptr; }
    bool isTopLevelObject() const { return parent && parent->isRoot(); }

    Ptr<EditableObject> object;
    HierarchyItem *parent;
    dag::Vector<HierarchyItem *> children;
  };

  bool isSceneEntity(ecs::EntityId eid);
  bool checkSceneEntities(const char *rule);
  int getEntityRecordLoadType(ecs::EntityId eid);
  ecs::Scene::SceneId getEntityRecordSceneId(ecs::EntityId eid) const;

  void addImportScene(ecs::Scene::SceneId parent_id, const char *fpath);

  bool isEntityTransformable(ecs::EntityId eid) const;
  bool isSceneInTransformableHierarchy(ecs::Scene::SceneId scene_id) const;

  // Pivot, transformable, pretty name getters/setters work on Import scenes
  bool isSceneTransformable(ecs::Scene::SceneId scene_id) const;
  void setSceneTransformable(ecs::Scene::SceneId scene_id, bool val);
  Point3 getScenePivot(ecs::Scene::SceneId scene_id) const;
  void setScenePivot(ecs::Scene::SceneId scene_id, const Point3 &pivot);
  const char *getScenePrettyName(ecs::Scene::SceneId scene_id) const;
  void setScenePrettyName(ecs::Scene::SceneId scene_id, const char *name);
  void setSceneNewParent(ecs::Scene::SceneId new_parent_id, Sqrat::Array items);
  void setSceneNewParentAndOrder(ecs::Scene::SceneId new_parent_id, uint32_t order, Sqrat::Array items);
  void setSceneOrder(ecs::Scene::SceneId id, uint32_t order);
  uint32_t getSceneOrder(ecs::Scene::SceneId id) const;
  uint32_t getSceneEntityCount(ecs::Scene::SceneId id) const;

  Sqrat::Array getSceneOrderedEntries(HSQUIRRELVM vm, ecs::Scene::SceneId id);
  void removeObjectsSq(Sqrat::Array objects_ids);
  void hideObjectsSq(Sqrat::Array objects_ids);
  void unhideObjectsSq(Sqrat::Array objects_ids);
  void lockObjectsSq(Sqrat::Array object_ids);
  void unlockObjectsSq(Sqrat::Array objects_ids);

  ecs::EntityId reCreateEditorEntity(ecs::EntityId eid);
  ecs::EntityId makeSingletonEntity(const char *tplName);

  static SQInteger get_modified_scene_ids(HSQUIRRELVM vm);
  Sqrat::Array getModifiedSceneIds(HSQUIRRELVM vm) const;

  void selectEntities(Sqrat::Array eids);
  Sqrat::Array getEntities(HSQUIRRELVM vm);
  Sqrat::Array getEcsTemplates(HSQUIRRELVM vm);
  Sqrat::Array getEcsTemplatesGroups(HSQUIRRELVM vm);
  static SQInteger get_entites(HSQUIRRELVM vm);
  static SQInteger get_ecs_templates(HSQUIRRELVM vm);
  static SQInteger get_ecs_templates_groups(HSQUIRRELVM vm);
  static SQInteger get_scene_ordered_entries(HSQUIRRELVM vm);

  Sqrat::Array getSceneEntities(HSQUIRRELVM vm);
  Sqrat::Array getSceneImports(HSQUIRRELVM vm);
  Sqrat::Table getSceneRecord(HSQUIRRELVM vm);
  Sqrat::Table getTargetScene(HSQUIRRELVM vm);
  static SQInteger get_scene_entities(HSQUIRRELVM vm);
  static SQInteger get_scene_imports(HSQUIRRELVM vm);
  static SQInteger get_scene_record(HSQUIRRELVM vm);
  static SQInteger get_target_scene(HSQUIRRELVM vm);

  void saveScenesSq(Sqrat::Array ids);
  void selectObjectsSq(Sqrat::Array objects_ids);

  void mapEidToEditableObject(EntityObj *obj);
  void unmapEidFromEditableObject(EntityObj *obj);

  void updateSingleHierarchyTransform(EditableObject &object, bool calculate_from_relative_transform);
  HierarchyItem *makeHierarchyItemParent(EditableObject &object, HierarchyItem &hierarchy_root,
    ska::flat_hash_map<EditableObject *, HierarchyItem *> &object_to_hierarchy_item_map);
  void fillObjectHierarchy(HierarchyItem &hierarchy_root);
  void applyChange(EditableObject &object, const Point3 &delta);
  void applyChangeHierarchically(const HierarchyItem &parent, const Point3 &delta, bool parent_updated = false);
  void makeTransformUndoForHierarchySelection(const EntityObjEditor::HierarchyItem &parent, bool parent_updated = false);
  bool shouldComputeDeltaFromStartPos() override { return true; }
  void changed(const Point3 &delta) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;

  void undoLastOperationSq();
  void redoLastOperationSq();

private:
  static constexpr int DELAY_TO_UPDATE_EIDS = 60;
  int delay_to_update_eids = DELAY_TO_UPDATE_EIDS;
  bool sceneInit = false;
  SimpleString sceneFilePath;
  SaveOrderRules saveOrderRules;
  Tab<EditableObject *> objsToRemove;
  Ptr<EditableObject> newObj;
  bool newObjOk = false;
  bool isCreatingSampleEntity = false;
  eastl::unique_ptr<IObjectCreator> objCreator;
  HierarchicalUndoGroup *hierarchicalUndoGroup = nullptr;
  ska::flat_hash_set<ecs::EntityId, ecs::EidHash> eidsCreated;
  ska::flat_hash_map<ecs::EntityId, EditableObject *, ecs::EidHash> eidToEntityObj;
  eastl::vector<eastl::string> hiddenTemplates;
  HierarchyItem temporaryHierarchyForGizmoChange;
  bool showOnlySceneEntities = false;

  int lastCreateObjFrames = 0;
  Ptr<EditableObject> lastCreatedObj;
  Point3 lastCreatedObjPos;

  ska::flat_hash_set<ecs::Scene::SceneId> createdSceneObjs;
  ska::flat_hash_map<ecs::Scene::SceneId, EditableObject *> sceneIdToEObj;

  struct TemplatesGroup
  {
    eastl::string name;
    eastl::vector<eastl::string> reqs;
    eastl::vector<eastl::pair<eastl::string, eastl::string>> variants;
  };
  eastl::vector<TemplatesGroup> orderedTemplatesGroups;
  bool testEcsTemplateCondition(const ecs::Template &ecs_template, const eastl::string &condition, const TemplatesGroup &group,
    int depth);
  bool testEcsTemplateByTemplatesGroup(const ecs::Template &ecs_template, eastl::vector<eastl::string> *out_variants,
    const TemplatesGroup &group, int depth);

  void selectNewObjEntity(const char *name);
  inline bool is_creating_entity_mode(int m) { return m == CM_OBJED_MODE_CREATE_ENTITY; }

private:
  virtual int parsePointActionPreviewShape(const char *shape) override;
  virtual void renderPointActionPreviewShape(int shape, const Point3 &pos, float param) override;
};
