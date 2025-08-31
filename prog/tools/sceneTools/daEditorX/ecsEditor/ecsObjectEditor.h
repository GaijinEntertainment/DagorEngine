// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecsPropPanel/ecsBasicObjectEditor.h>
#include <ecsPropPanel/ecsTemplateSelectorClient.h>

class ECSEditableObject;
class ECSHierarchicalUndoGroup;
class ECSTemplateSelectorDialog;
struct ECSEntityCreateData;

ECS_UNICAST_EVENT_TYPE(EditorToDeleteEntitiesEvent, ecs::EidList /*linkedEntitiesToDelete*/)
ECS_UNICAST_EVENT_TYPE(EditorOnInitClonedEntity)

#define ECS_EDITOR_SELECTED_TEMPLATE "daeditor_selected"

class ECSObjectEditor : public ECSBasicObjectEditor, public IECSTemplateSelectorClient
{
public:
  typedef eastl::vector<eastl::string> SaveOrderRules;

  ECSObjectEditor();
  ~ECSObjectEditor() override;

  /// @name Methods inherited from ObjectEditor
  //@{
  void registerViewportAccelerators(IWndManager &wndManager) override;
  void fillToolBar(PropPanel::ContainerPropertyControl *toolbar) override;
  void updateToolbarButtons() override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags) override;
  void selectionChanged() override;
  void setEditMode(int cm) override;
  void createObjectBySample(RenderableEditableObject *sample) override;
  bool isSampleObject(const RenderableEditableObject &object) const;
  void render() override;
  void update(float dt) override;
  //@}

  void reset();
  void load(const char *blk_path);

  void setSaveOrderRules(const SaveOrderRules &rules) { saveOrderRules = rules; }
  int saveObjectsInternal(const char *fpath);
  void saveObjects(const char *new_fpath = nullptr);
  bool hasUnsavedChanges() const;

  void hideSelectedTemplate();
  void unhideAll();

  void addEntity(ecs::EntityId eid) { addEntityEx(eid, false); }
  void addEntityEx(ecs::EntityId eid, bool use_undo);
  ecs::EntityId createEntitySample(ECSEntityCreateData *data);
  ecs::EntityId createEntityDirect(ECSEntityCreateData *data);
  void destroyEntityDirect(ecs::EntityId eid);

  void addEditableEntities();
  void refreshEids();
  RenderableEditableObject *getObjectByEid(ecs::EntityId eid) const;
  ECSEditableObject *getParentSelectionHoveredObject() { return parentSelectionHoveredObject; }

  void onBeforeExport();
  void onAfterExport();

  template <typename F>
  void forEachEditableObjectWithTemplate(F fn);

  template <typename F>
  void forEachEditableObject(F fn);

  static bool checkSingletonMutexExists(const char *mutex_name);

private:
  struct HierarchyItem
  {
    HierarchyItem() : object(nullptr), parent(nullptr) {}
    HierarchyItem(ECSEditableObject *in_object, HierarchyItem *in_parent) : object(in_object), parent(in_parent) {}
    ~HierarchyItem() { clearChildren(); }

    void clearChildren() { clear_all_ptr_items(children); }

    HierarchyItem *addChild(ECSEditableObject &child_object)
    {
      HierarchyItem *child = new HierarchyItem(&child_object, this);
      children.push_back(child);
      return child;
    }

    bool isRoot() const { return parent == nullptr; }
    bool isTopLevelObject() const { return parent && parent->isRoot(); }

    ECSEditableObject *object;
    HierarchyItem *parent;
    dag::Vector<HierarchyItem *> children;
  };

  /// @name Methods inherited from ObjectEditor
  //@{
  void _addObjects(RenderableEditableObject **obj, int num, bool use_undo) override;
  void _removeObjects(RenderableEditableObject **obj, int num, bool use_undo) override;
  void deleteSelectedObjects(bool use_undo = true) override;
  //@}

  /// @name Methods inherited from IGizmoClient
  //@{
  bool getAxes(Point3 &ax, Point3 &ay, Point3 &az) override;
  bool shouldComputeDeltaFromStartPos() override { return true; }
  void changed(const Point3 &delta) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;
  int getAvailableTypes() override;
  //@}

  /// @name Method inherited from IECSTemplateSelectorClient
  //@{
  void onTemplateSelectorTemplateSelected(const char *template_name) override;
  //@}

  void cloneSelection();
  void fillDeleteDepsObjects(Tab<RenderableEditableObject *> &list) const;

  bool checkSceneEntities(const char *rule);
  int getEntityRecordLoadType(ecs::EntityId eid);
  int getEntityRecordIndex(ecs::EntityId eid);
  ecs::EntityId reCreateEditorEntity(ecs::EntityId eid);
  ecs::EntityId makeSingletonEntity(const char *tplName);

  void mapEidToEditableObject(ECSEditableObject *obj);
  void unmapEidFromEditableObject(ECSEditableObject *obj);

  void updateSingleHierarchyTransform(ECSEditableObject &object, bool calculate_from_relative_transform);
  HierarchyItem *makeHierarchyItemParent(ECSEditableObject &object, HierarchyItem &hierarchy_root,
    ska::flat_hash_map<EditableObject *, HierarchyItem *> &object_to_hierarchy_item_map);
  void fillObjectHierarchy(HierarchyItem &hierarchy_root);
  void applyChange(ECSEditableObject &object, const Point3 &delta);
  void applyChangeHierarchically(const HierarchyItem &parent, const Point3 &delta, bool parent_updated = false);
  void makeTransformUndoForHierarchySelection(const HierarchyItem &parent, bool parent_updated = false);

  bool cancelCreateMode();
  void zoomAndCenter();
  void selectNewObjEntity(const char *name);
  bool setParentForObjects(dag::ConstSpan<ECSEditableObject *> children, ECSEditableObject &parent);
  void clearParentForSelection();
  void toggleFreeTransformForSelection();

  SaveOrderRules saveOrderRules;
  Tab<RenderableEditableObject *> objsToRemove;
  Ptr<ECSEditableObject> newObj;
  String newObjTemplate;
  bool newObjOk = false;
  eastl::unique_ptr<ECSTemplateSelectorDialog> templateSelectorDialog;
  eastl::unique_ptr<ECSHierarchicalUndoGroup> hierarchicalUndoGroup;
  ska::flat_hash_set<ecs::EntityId, ecs::EidHash> eidsCreated;
  ska::flat_hash_map<ecs::EntityId, RenderableEditableObject *, ecs::EidHash> eidToEditableObject;
  eastl::vector<eastl::string> hiddenTemplates;
  ecs::EidList objectsForParenting;
  HierarchyItem temporaryHierarchyForGizmoChange;
  bool showOnlySceneEntities = false;

  int lastCreateObjFrames = 0;
  Ptr<EditableObject> lastCreatedObj;
  Point3 lastCreatedObjPos;

  bool cloneMode = false;
  Point3 cloneStartPosition = ZERO<Point3>();
  PtrTab<ECSEditableObject> cloneObjs;

  Ptr<ECSEditableObject> parentSelectionHoveredObject;
};
