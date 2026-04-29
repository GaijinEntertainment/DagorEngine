// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/scene/scene.h>
#include <daECS/core/entityId.h>
#include <EASTL/functional.h>
#include <EASTL/memory.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/control/panelWindow.h>
#include <propPanel/control/treeInterface.h>

namespace PropPanel
{
class TreeNode;
} // namespace PropPanel

class ObjectEditor;
class IECSToEditableObjectConverter;
class RenderableEditableObject;
class ECSSceneObject;
class ECSEntityObject;

class ECSSceneOutlinerPanel : public PropPanel::ControlEventHandler, public PropPanel::ITreeRenderEx, public PropPanel::ITreeFilter
{
public:
  enum class LeafType
  {
    SCENE,
    ENTITY
  };

  struct LeafUserData
  {
    LeafType type;
    union
    {
      ecs::Scene::SceneId sid;
      ecs::EntityId eid;
    };
  };

  explicit ECSSceneOutlinerPanel(ObjectEditor &obj_editor, const IECSToEditableObjectConverter &obj_converter, void *hwnd,
    const char *caption) noexcept;
  ~ECSSceneOutlinerPanel() noexcept override;

  void resetPanel(const eastl::function<void(const eastl::function<void(ecs::EntityId)> &)> &cb);
  void fillPanel(const eastl::function<void(const eastl::function<void(ecs::EntityId)> &)> &cb);

  bool isVisible() const { return !!panelWindow; }

  PropPanel::PanelWindowPropertyControl *getPanel() const { return panelWindow; }

  void updateImgui();

  void syncSelection();

  void onEntityAdded(const ecs::EntityId eid);
  void onSceneAdded(const ecs::Scene::SceneId sid);
  void onEntityRemoved(const ecs::EntityId eid);
  void onSceneRemoved(const ecs::Scene::SceneId sid);
  void onEntitySceneDataChanged(const ecs::EntityId eid);

protected:
  long onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key) override;

private:
  enum
  {
    PID_SCENE_TREE = 1,

    PID_CM_DELETE,
    PID_CM_ADD_NEW_IMPORT,
    PID_CM_ADD_EXISTING_IMPORT,
  };

  struct Filters
  {
    bool operator==(const Filters &rhs) const = default;

    String filterString = {};
    bool showEntities = true;
  };

  class DragHandler;
  class DropHandler;
  class EventHandler;
  class SetEntityParentUndoRedo;
  class SetSceneParentUndoRedo;
  class SetEntityOrderUndoRedo;
  class SetSceneOrderUndoRedo;

  struct DragAndDropPayload
  {
    dag::Vector<PropPanel::TLeafHandle> items;
  };

  void addNoSceneLeaf();
  void addSceneLeaf(PropPanel::TLeafHandle parent, const ecs::Scene::SceneRecord &record, bool add_children = true);
  void addEntityLeaf(PropPanel::TLeafHandle parent, const ecs::EntityId eid);
  void reinsertLeafAtOrderedPosition(PropPanel::TLeafHandle parent, PropPanel::TLeafHandle leaf);
  uint32_t getLeafOrder(PropPanel::TLeafHandle leaf) const;

  void deleteSelectedLeafs();
  void deleteLeaf(PropPanel::TLeafHandle handle, bool editor_event = false);

  bool treeNodeStart(const NodeStartData &data) override;
  void treeNodeRender(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow) override;
  void treeNodeEnd(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data) override;
  void treeItemRenderEx(const ItemRenderData &data) override;

  bool filterNode(const PropPanel::TreeNode &node) override;
  bool hasAnyFilter() const override { return Filters{} != filters; }

  RenderableEditableObject *getEditableObjectFromHandle(PropPanel::TLeafHandle handle) const;
  bool canSelectedLeafsBeDeleted();

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  bool drawFilterControls();

  ECSSceneObject *getSceneObject(ecs::Scene::SceneId sid) const;
  ECSEntityObject *getEntityObject(ecs::EntityId eid) const;

  eastl::unique_ptr<DragHandler> dragHandler;
  eastl::unique_ptr<DropHandler> dropHandler;
  eastl::unique_ptr<EventHandler> eventHandler;
  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  PropPanel::ContainerPropertyControl *tree = nullptr;
  eastl::unordered_map<PropPanel::TLeafHandle, eastl::unique_ptr<LeafUserData>> userDataHolder;
  eastl::unordered_map<RenderableEditableObject *, PropPanel::TLeafHandle> edObjToHandle;
  eastl::unordered_set<PropPanel::TLeafHandle> deferredDeletions;

  PropPanel::IconId locateIconId;
  PropPanel::IconId selectionIconId;
  PropPanel::IconId eyeShownIconId;
  PropPanel::IconId eyeHiddenIconId;
  PropPanel::IconId lockOpenIconId;
  PropPanel::IconId lockClosedIconId;
  PropPanel::IconId deleteIconId;
  PropPanel::IconId searchIconId;
  PropPanel::IconId closeIconId;
  PropPanel::IconId filterDefaultIconId;
  PropPanel::IconId filterActiveIconId;
  PropPanel::IconId addToSelectionIconId;
  PropPanel::IconId removeFromSelectionIconId;

  ObjectEditor &objEditor;
  const IECSToEditableObjectConverter &objConverter;

  bool isUpdatingSelection = false;

  bool filterSettingsOpen = false;
  Filters filters;
  const bool searchInputFocusId = false; // Only the address of this member is used.

  ecs::Scene::SceneId importRequested = ecs::Scene::C_INVALID_SCENE_ID;
  bool requestToastOnAdd = false;

  PropPanel::TLeafHandle noSceneLeaf = nullptr;
};
