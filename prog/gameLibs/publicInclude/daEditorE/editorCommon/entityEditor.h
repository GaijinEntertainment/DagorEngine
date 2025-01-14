//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/de_objEditor.h>
#include <daECS/core/template.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>
#include <util/dag_console.h>
#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <sqrat.h>

ECS_UNICAST_EVENT_TYPE(EditorToDeleteEntitiesEvent, ecs::EidList /*linkedEntitiesToDelete*/)
ECS_UNICAST_EVENT_TYPE(EditorOnInitClonedEntity)

struct EntCreateData;
class EntityObj;
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

  void setupNewScene(const char *fpath);
  const char *getSceneFilePath() { return sceneFilePath; }

  typedef eastl::vector<eastl::string> SaveOrderRules;
  void setSaveOrderRules(const SaveOrderRules &rules) { saveOrderRules = rules; }
  int saveObjectsInternal(const char *fpath);
  void saveObjectsCopy(const char *fpath, const char *reason);
  void saveObjects(const char *new_fpath = NULL);
  void saveEntityToBlk(ecs::EntityId eid, DataBlock &blk) const;
  bool hasUnsavedChanges() const;

  void hideSelectedTemplate();
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

  virtual void update(float dt);
  void scheduleObjRemoval(EditableObject *o) { objsToRemove.push_back(o); }

  static void register_script_class(HSQUIRRELVM vm);

  static eastl::string getTemplateNameForUI(ecs::EntityId eid);
  static bool hasTemplateComponent(const char *template_name, const char *comp_name);
  static bool getSavedComponents(ecs::EntityId eid, eastl::vector<eastl::string> &out_comps);
  static void resetComponent(ecs::EntityId eid, const char *comp_name);
  static void saveComponent(ecs::EntityId eid, const char *comp_name);
  static void saveAddTemplate(ecs::EntityId eid, const char *templ_name);
  static void saveDelTemplate(ecs::EntityId eid, const char *templ_name, bool use_undo = false);

  static bool isPickingRendinst();
  void refreshEids();

  static bool checkSingletonMutexExists(const char *mutex_name);

  template <typename F>
  void forEachEntityObjWithTemplate(F fn);
  template <typename F>
  void forEachEntityObj(F fn);

  EditableObject *getObjectByEid(ecs::EntityId eid) const;

protected:
  virtual void cloneSelection() override;
  virtual void _addObjects(EditableObject **obj, int num, bool use_undo) override;
  virtual void _removeObjects(EditableObject **obj, int num, bool use_undo) override;

private:
  bool checkSceneEntities(const char *rule);
  ecs::EntityId reCreateEditorEntity(ecs::EntityId eid);
  ecs::EntityId makeSingletonEntity(const char *tplName);

  void selectEntities(Sqrat::Array eids);
  Sqrat::Array getEntities(HSQUIRRELVM vm);
  Sqrat::Array getEcsTemplates(HSQUIRRELVM vm);
  Sqrat::Array getEcsTemplatesGroups(HSQUIRRELVM vm);
  static SQInteger get_entites(HSQUIRRELVM vm);
  static SQInteger get_ecs_templates(HSQUIRRELVM vm);
  static SQInteger get_ecs_templates_groups(HSQUIRRELVM vm);

  void mapEidToEditableObject(EntityObj *obj);
  void unmapEidFromEditableObject(EntityObj *obj);

private:
  static constexpr int DELAY_TO_UPDATE_EIDS = 60;
  int delay_to_update_eids = DELAY_TO_UPDATE_EIDS;
  bool sceneInit = false;
  SimpleString sceneFilePath;
  SaveOrderRules saveOrderRules;
  Tab<EditableObject *> objsToRemove;
  Ptr<EditableObject> newObj;
  bool newObjOk = false;
  eastl::unique_ptr<IObjectCreator> objCreator;
  ska::flat_hash_set<ecs::EntityId, ecs::EidHash> eidsCreated;
  ska::flat_hash_map<ecs::EntityId, EditableObject *, ecs::EidHash> eidToEntityObj;
  eastl::vector<eastl::string> hiddenTemplates;
  bool showOnlySceneEntities = false;

  int lastCreateObjFrames = 0;
  Ptr<EditableObject> lastCreatedObj;
  Point3 lastCreatedObjPos;

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
