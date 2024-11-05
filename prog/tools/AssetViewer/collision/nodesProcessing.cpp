// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodesProcessing.h"
#include "propPanelPids.h"
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_localConv.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <scene/dag_physMat.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>

NodesProcessing::NodesProcessing()
{
  editMode = false;
  selectedPanelGroup = PID_COLLISION_INFO_GROUP;
}

void NodesProcessing::init(DagorAsset *asset, CollisionResource *collision_res, PropPanel::ITreeControlEventHandler *event_handler)
{
  curAsset = asset;
  collisionRes = collision_res;
  treeEventHandler = event_handler;
  selectionNodesProcessing.init(asset, collision_res);
  if (selectionNodesProcessing.calcCollisionAfterReject)
    calcCollisionsAfterReject();
}

void NodesProcessing::setPropPanel(PropPanel::ContainerPropertyControl *prop_panel)
{
  panel = prop_panel;
  combinedNodesProcessing.init(collisionRes, prop_panel);
  kdopSetupProcessing.init(collisionRes, prop_panel);
  convexComputerProcessing.init(collisionRes, prop_panel);
  convexVhacdProcessing.init(collisionRes, prop_panel);
  selectionNodesProcessing.setPropPanel(prop_panel);
}

void NodesProcessing::renderNodes(int selected_node_id, bool draw_solid)
{
  const bool isFaded = selected_node_id > -1 || editMode;
  combinedNodesProcessing.renderCombinedNodes(isFaded, draw_solid, selectionNodesProcessing.combinedNodesSettings);
  kdopSetupProcessing.renderKdop(isFaded);
  convexComputerProcessing.renderComputedConvex(isFaded);
  convexVhacdProcessing.renderVhacd(isFaded);
}

void NodesProcessing::onClick(int pcb_id)
{
  switch (pcb_id)
  {
    case PID_SHOW_CONVEX_COMPUTER: convexComputerProcessing.showConvexComputed = panel->getBool(pcb_id); break;

    case PID_SHOW_CONVEX_VHACD: convexVhacdProcessing.showConvexVHACD = panel->getBool(pcb_id); break;

    case PID_SHOW_KDOP: kdopSetupProcessing.showKdop = panel->getBool(pcb_id); break;

    case PID_SHOW_KDOP_FACES: kdopSetupProcessing.showKdopFaces = panel->getBool(pcb_id); break;

    case PID_SHOW_KDOP_DIRS: kdopSetupProcessing.showKdopDirs = panel->getBool(pcb_id); break;

    case PID_KDOP_CUT_OFF_PLANES:
      panel->setEnabledById(PID_KDOP_CUT_OFF_THRESHOLD, panel->getBool(pcb_id));
      kdopSetupProcessing.selectedSettings.cutOffThreshold = 0.0f;
      kdopSetupProcessing.calcSelectedKdop();
      break;

    case PID_SAVE_KDOP:
    case PID_SAVE_CONVEX_COMPUTER:
    case PID_SAVE_CONVEX_VHACD: saveNewNode(pcb_id); break;

    case PID_CANCEL_KDOP:
    case PID_CANCEL_CONVEX_COMPUTER:
    case PID_CANCEL_CONVEX_VHACD: cancelSwitchPanel(pcb_id); break;

    case PID_CREATE_NEW_NODE:
    case PID_EDIT_NODE:
    case PID_NEXT_EDIT_NODE:
    case PID_CANCEL_EDIT_NODE: switchPropPanel(pcb_id); break;

    case PID_DELETE_NODE: deleteCollisionNode(); break;
    case PID_SAVE_CHANGES:
      saveCollisionNodes();
      if (!curAsset->isVirtual())
        curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
      break;
  }
}

void NodesProcessing::onChange(int pcb_id)
{
  switch (pcb_id)
  {
    case PID_ROT_X:
    case PID_ROT_Y:
    case PID_ROT_Z:
    case PID_SEGMENTS_COUNT_X:
    case PID_SEGMENTS_COUNT_Y:
    case PID_KDOP_CUT_OFF_THRESHOLD:
    case PID_SELECTED_PRESET: kdopSetupProcessing.calcSelectedKdop(); break;

    case PID_CONVEX_COMPUTER_SHRINK: convexComputerProcessing.calcSelectedComputer(); break;

    case PID_CONVEX_DEPTH:
    case PID_CONVEX_MAX_HULLS:
    case PID_CONVEX_MAX_VERTS:
    case PID_CONVEX_RESOLUTION: convexVhacdProcessing.calcSelectedInterface(); break;

    case PID_SELECTED_TYPE:
    case PID_SELECTABLE_NODES_LIST:
      switchSelectedType();
      checkCreatedNode();
      break;

    case PID_NEW_NODE_NAME: checkCreatedNode(); break;

    case PID_COLLISION_NODES_TREE: checkSelectedTreeLeaf(); break;
  }
}

void NodesProcessing::selectNode(const char *node_name, bool ctrl_pressed)
{
  if (node_name || !editMode)
  {
    selectionNodesProcessing.selectNode(node_name, ctrl_pressed);
    checkSelectedTreeLeaf();
  }
  if (editMode)
    switchSelectedType();
}

void NodesProcessing::saveCollisionNodes() { selectionNodesProcessing.saveCollisionNodes(); }

void NodesProcessing::saveExportedCollisionNodes()
{
  selectionNodesProcessing.saveExportedCollisionNodes();
  if (!selectionNodesProcessing.calcCollisionAfterReject)
  {
    combinedNodesProcessing.clearSelectedNode();
    combinedNodesProcessing.nodes.clear();
    kdopSetupProcessing.kdops.clear();
    convexComputerProcessing.computers.clear();
    convexVhacdProcessing.interfaces.clear();
    editMode = false;
    selectionNodesProcessing.clearSettings();
  }
  selectionNodesProcessing.hiddenNodes.clear();
}

void NodesProcessing::fillCollisionInfoPanel()
{
  auto *group = panel->createGroup(PID_COLLISION_INFO_GROUP, "Collision nodes");
  auto *tree = group->createMultiSelectTreeCheckbox(PID_COLLISION_NODES_TREE, "Collision nodes", hdpi::_pxScaled(300));
  PropPanel::ContainerPropertyControl *treeContainer = tree->getContainer();
  treeContainer->setTreeEventHandler(treeEventHandler);

  selectionNodesProcessing.fillInfoTree(tree);

  group->createButton(PID_CREATE_NEW_NODE, "Create new node");
  group->createButton(PID_EDIT_NODE, "Edit node", false);
  group->createButton(PID_DELETE_NODE, "Delete node", false);
  group->createButton(PID_SAVE_CHANGES, "Save changes");
}

static void fill_node_type_names(Tab<String> &type_names)
{
  type_names.push_back() = "mesh";
  type_names.push_back() = "box";
  type_names.push_back() = "sphere";
  type_names.push_back() = "kdop";
  type_names.push_back() = "convex computer";
  type_names.push_back() = "convex v-hacd";
}

void NodesProcessing::fillEditNodeInfoPanel()
{
  Tab<String> nodes;
  selectionNodesProcessing.fillNodeNamesTab(nodes);
  Tab<String> typeNames;
  Tab<String> materialTypes;
  fill_node_type_names(typeNames);
  for (const PhysMat::MaterialData &data : PhysMat::getMaterials())
    materialTypes.emplace_back(data.name);

  PropPanel::ContainerPropertyControl *group = panel->createGroup(PID_EDIT_COLLISION_NODE_GROUP, "Create new node");
  group->createMultiSelectList(PID_SELECTABLE_NODES_LIST, nodes, hdpi::_pxScaled(304));
  group->createEditBox(PID_NEW_NODE_NAME, "New node name");
  group->createCheckBox(PID_REPLACE_NODE, "Replace selected nodes", true);
  group->createCheckBox(PID_PHYS_COLLIDABLE_FLAG, "Phys Collidable", true);
  group->createCheckBox(PID_TRACEABLE_FLAG, "Traceable", true);
  group->createCombo(PID_SELECTED_TYPE, "Collision type", typeNames, 0);
  group->createCombo(PID_MATERIAL_TYPE, "Material type", materialTypes, 0);
  group->createButton(PID_NEXT_EDIT_NODE, "Save", false);
  group->createButton(PID_CANCEL_EDIT_NODE, "Cancel");
}

void NodesProcessing::setPanelAfterReject()
{
  if (!selectionNodesProcessing.calcCollisionAfterReject)
    return;

  selectionNodesProcessing.calcCollisionAfterReject = false;
  if (selectedPanelGroup != PID_COLLISION_INFO_GROUP)
    removePanelGroupDelayed(PID_COLLISION_INFO_GROUP);

  switch (selectedPanelGroup)
  {
    case PID_EDIT_NODE: editCollisionNode(combinedNodesProcessing.selectedSettings.nodeName); break;

    case PID_KDOP_GROUP: kdopSetupProcessing.fillKdopPanel(); break;

    case PID_CONVEX_COMPUTER_GROUP:
      convexComputerProcessing.fillConvexComputerPanel();
      convexComputerProcessing.calcSelectedComputer();
      break;

    case PID_CONVEX_VHACD_GROUP:
      convexVhacdProcessing.fillConvexVhacdPanel();
      convexVhacdProcessing.calcSelectedInterface();
      break;

    case PID_EDIT_COLLISION_NODE_GROUP:
      fillEditNodeInfoPanel();
      restoreEditInfoPanel(combinedNodesProcessing.selectedSettings);
      combinedNodesProcessing.selectedSettings.refNodes.clear();
      selectionNodesProcessing.getSelectedNodes(combinedNodesProcessing.selectedSettings.refNodes);
      combinedNodesProcessing.calcSelectedCombinedNode();
      break;
  }
}

bool NodesProcessing::canChangeAsset()
{
  if (selectionNodesProcessing.haveUnsavedChanges())
  {
    const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Collision properties",
      "You have changed collision properties. Do you want to save the changes to the collision?");

    switch (dialogResult)
    {
      case PropPanel::DIALOG_ID_CANCEL: return false;
      case PropPanel::DIALOG_ID_YES:
        saveCollisionNodes();
        if (!curAsset->isVirtual())
          curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
        break;
    }
  }

  return true;
}

void NodesProcessing::calcCollisionsAfterReject()
{
  combinedNodesProcessing.init(collisionRes, nullptr);
  kdopSetupProcessing.init(collisionRes, nullptr);
  convexComputerProcessing.init(collisionRes, nullptr);
  convexVhacdProcessing.init(collisionRes, nullptr);
  for (const auto &settings : selectionNodesProcessing.rejectedCombinedNodesSettings)
  {
    combinedNodesProcessing.calcCombinedNode(settings);
    combinedNodesProcessing.addSelectedCombinedNode();
    selectionNodesProcessing.addCombinedSettings(settings);
  }
  for (const auto &settings : selectionNodesProcessing.rejectedKdopsSettings)
  {
    kdopSetupProcessing.calcKdop(settings);
    kdopSetupProcessing.addSelectedKdop();
    selectionNodesProcessing.addKdopSettings(settings);
  }
  for (const auto &settings : selectionNodesProcessing.rejectedConvexsComputerSettings)
  {
    convexComputerProcessing.calcComputer(settings);
    convexComputerProcessing.addSelectedComputer();
    selectionNodesProcessing.addConvexComputerSettings(settings);
  }
  for (const auto &settings : selectionNodesProcessing.rejectedConvexsVhacdSettings)
  {
    convexVhacdProcessing.calcInterface(settings);
    convexVhacdProcessing.addSelectedInterface();
    selectionNodesProcessing.addConvexVhacdSettings(settings);
  }
  selectionNodesProcessing.clearRejectedSettings();
  selectionNodesProcessing.updateHiddenNodes();
}


void NodesProcessing::switchPropPanel(int pcb_id)
{
  switch (pcb_id)
  {
    case PID_CREATE_NEW_NODE:
      removePanelGroupDelayed(PID_COLLISION_INFO_GROUP);
      fillEditNodeInfoPanel();
      editMode = true;
      break;

    case PID_EDIT_NODE: editCollisionNode(); break;

    case PID_NEXT_EDIT_NODE:
    {
      SelectedNodesSettings newNodeSettings;
      newNodeSettings.nodeName = panel->getText(PID_NEW_NODE_NAME);
      newNodeSettings.nodeName.toLower();
      if (!selectionNodesProcessing.editSettings || selectionNodesProcessing.editSettings->nodeName != newNodeSettings.nodeName)
      {
        if (newNodeSettings.nodeName.empty() || !selectionNodesProcessing.checkNodeName(newNodeSettings.nodeName))
          return;
      }

      selectionNodesProcessing.getSelectedNodes(newNodeSettings.refNodes);
      newNodeSettings.isTraceable = panel->getBool(PID_TRACEABLE_FLAG);
      newNodeSettings.isPhysCollidable = panel->getBool(PID_PHYS_COLLIDABLE_FLAG);
      newNodeSettings.replaceNodes = panel->getBool(PID_REPLACE_NODE);
      newNodeSettings.type = static_cast<ExportCollisionNodeType>(panel->getInt(PID_SELECTED_TYPE));
      newNodeSettings.physMat = panel->getText(PID_MATERIAL_TYPE);

      if (newNodeSettings.refNodes.empty() ||
          selectionNodesProcessing.rejectExportedCollisions(newNodeSettings.refNodes, newNodeSettings.replaceNodes))
      {
        if (selectionNodesProcessing.calcCollisionAfterReject)
        {
          if (newNodeSettings.type == ExportCollisionNodeType::KDOP)
          {
            selectedPanelGroup = PID_KDOP_GROUP;
            kdopSetupProcessing.setSelectedNodesSettings(eastl::move(newNodeSettings));
          }
          else if (newNodeSettings.type == ExportCollisionNodeType::CONVEX_COMPUTER)
          {
            selectedPanelGroup = PID_CONVEX_COMPUTER_GROUP;
            convexComputerProcessing.setSelectedNodesSettings(eastl::move(newNodeSettings));
          }
          else if (newNodeSettings.type == ExportCollisionNodeType::CONVEX_VHACD)
          {
            selectedPanelGroup = PID_CONVEX_VHACD_GROUP;
            convexVhacdProcessing.setSelectedNodesSettings(eastl::move(newNodeSettings));
          }
          curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
        }
        return;
      }

      switchPanelByType(pcb_id, eastl::move(newNodeSettings));
      removePanelGroupDelayed(PID_EDIT_COLLISION_NODE_GROUP);
      break;
    }

    case PID_CANCEL_EDIT_NODE:
      removePanelGroupDelayed(PID_EDIT_COLLISION_NODE_GROUP);
      selectionNodesProcessing.editSettings = nullptr;
      combinedNodesProcessing.clearSelectedNode();
      editMode = false;
      fillCollisionInfoPanel();
      break;
  }
}

void NodesProcessing::switchPanelByType(int pcb_id, SelectedNodesSettings &&new_node_settings)
{
  switch (new_node_settings.type)
  {
    case ExportCollisionNodeType::MESH:
    case ExportCollisionNodeType::BOX:
    case ExportCollisionNodeType::SPHERE: saveNewNode(pcb_id); break;

    case ExportCollisionNodeType::KDOP:
      kdopSetupProcessing.fillKdopPanel();
      kdopSetupProcessing.setSelectedNodesSettings(eastl::move(new_node_settings));
      if (KdopSettings settings; //
          selectionNodesProcessing.editSettings &&
          selectionNodesProcessing.findSettingsByName(selectionNodesProcessing.editSettings->nodeName, settings))
      {
        kdopSetupProcessing.setKdopParams(settings);
        kdopSetupProcessing.updatePanelParams();
        kdopSetupProcessing.calcSelectedKdop();
      }
      break;

    case ExportCollisionNodeType::CONVEX_COMPUTER:
      convexComputerProcessing.fillConvexComputerPanel();
      convexComputerProcessing.setSelectedNodesSettings(eastl::move(new_node_settings));
      if (ConvexComputerSettings settings;
          selectionNodesProcessing.editSettings &&
          selectionNodesProcessing.findSettingsByName(selectionNodesProcessing.editSettings->nodeName, settings))
      {
        convexComputerProcessing.setConvexComputerParams(settings);
        convexComputerProcessing.updatePanelParams();
      }
      convexComputerProcessing.calcSelectedComputer();
      break;

    case ExportCollisionNodeType::CONVEX_VHACD:
      convexVhacdProcessing.fillConvexVhacdPanel();
      convexVhacdProcessing.setSelectedNodesSettings(eastl::move(new_node_settings));
      if (ConvexVhacdSettings settings;
          selectionNodesProcessing.editSettings &&
          selectionNodesProcessing.findSettingsByName(selectionNodesProcessing.editSettings->nodeName, settings))
      {
        convexVhacdProcessing.setConvexVhacdParams(settings);
        convexVhacdProcessing.updatePanelParams();
      }
      convexVhacdProcessing.calcSelectedInterface();
      break;
  }
  combinedNodesProcessing.clearSelectedNode();
}

void NodesProcessing::saveNewNode(int pcb_id)
{
  if (selectionNodesProcessing.editSettings)
  {
    ExportCollisionNodeType type;
    String nodeName = selectionNodesProcessing.editSettings->nodeName;
    deleteNodeFromProcessing(nodeName, type, selectionNodesProcessing.deleteSelectedNode(nodeName, type));
    selectionNodesProcessing.editSettings = nullptr;
  }
  switch (pcb_id)
  {
    case PID_NEXT_EDIT_NODE:
      combinedNodesProcessing.addSelectedCombinedNode();
      combinedNodesProcessing.updateSeletectedSettings();
      selectionNodesProcessing.addCombinedSettings(combinedNodesProcessing.selectedSettings);
      combinedNodesProcessing.clearSelectedNode();
      break;

    case PID_SAVE_KDOP:
      removePanelGroupDelayed(PID_KDOP_GROUP);
      kdopSetupProcessing.addSelectedKdop();
      selectionNodesProcessing.addKdopSettings(kdopSetupProcessing.selectedSettings);
      break;

    case PID_SAVE_CONVEX_COMPUTER:
      removePanelGroupDelayed(PID_CONVEX_COMPUTER_GROUP);
      convexComputerProcessing.addSelectedComputer();
      selectionNodesProcessing.addConvexComputerSettings(convexComputerProcessing.selectedSettings);
      break;

    case PID_SAVE_CONVEX_VHACD:
      removePanelGroupDelayed(PID_CONVEX_VHACD_GROUP);
      convexVhacdProcessing.addSelectedInterface();
      selectionNodesProcessing.addConvexVhacdSettings(convexVhacdProcessing.selectedSettings);
      break;
  }
  for (const auto &nodeName : selectionNodesProcessing.deleteNodesCandidats)
  {
    ExportCollisionNodeType type;
    deleteNodeFromProcessing(nodeName, type, selectionNodesProcessing.deleteSelectedNode(nodeName, type));
  }
  selectionNodesProcessing.deleteNodesCandidats.clear();
  editMode = false;
  selectionNodesProcessing.updateHiddenNodes();
  fillCollisionInfoPanel();
}

void NodesProcessing::cancelSwitchPanel(int pcb_id)
{
  SelectedNodesSettings lastSettings;
  switch (pcb_id)
  {
    case PID_CANCEL_KDOP:
      removePanelGroupDelayed(PID_KDOP_GROUP);
      kdopSetupProcessing.selectedKdop.reset();
      lastSettings = kdopSetupProcessing.selectedSettings.selectedNodes;
      break;
    case PID_CANCEL_CONVEX_COMPUTER:
      removePanelGroupDelayed(PID_CONVEX_COMPUTER_GROUP);
      convexComputerProcessing.selectedComputer.vertices.clear();
      convexComputerProcessing.selectedComputer.faces.clear();
      convexComputerProcessing.selectedComputer.edges.clear();
      lastSettings = convexComputerProcessing.selectedSettings.selectedNodes;
      break;
    case PID_CANCEL_CONVEX_VHACD:
      removePanelGroupDelayed(PID_CONVEX_VHACD_GROUP);
      convexVhacdProcessing.selectedInterface->Clean();
      lastSettings = convexVhacdProcessing.selectedSettings.selectedNodes;
      break;
  }
  selectionNodesProcessing.deleteNodesCandidats.clear();
  fillEditNodeInfoPanel();
  restoreEditInfoPanel(lastSettings);
  combinedNodesProcessing.selectedSettings.refNodes = lastSettings.refNodes;
  combinedNodesProcessing.calcSelectedCombinedNode();
}

// remove group with delayed action
// for escape case where winapi throw execption
// when after click button the group with this button is deleted
void NodesProcessing::removePanelGroupDelayed(int pcb_id)
{
  add_delayed_action(make_delayed_action([&, pcb_id]() { panel->removeById(pcb_id); }));
}

static void find_ref_nodes_idxs(const SelectedNodesSettings &settings, const Tab<String> &nodes, Tab<int> &sels)
{
  for (const auto &refNode : settings.refNodes)
  {
    for (int i = 0; i < nodes.size(); ++i)
    {
      if (nodes[i] == refNode)
      {
        sels.push_back(i);
        break;
      }
    }
  }
}

static void set_settings_on_panel(const SelectedNodesSettings &settings, Tab<String> &nodes, Tab<int> &sels,
  PropPanel::ContainerPropertyControl *panel)
{
  panel->setStrings(PID_SELECTABLE_NODES_LIST, nodes);
  panel->setSelection(PID_SELECTABLE_NODES_LIST, sels);
  panel->setText(PID_NEW_NODE_NAME, settings.nodeName);
  panel->setBool(PID_REPLACE_NODE, settings.replaceNodes);
  panel->setBool(PID_PHYS_COLLIDABLE_FLAG, settings.isPhysCollidable);
  panel->setBool(PID_TRACEABLE_FLAG, settings.isTraceable);
  panel->setInt(PID_SELECTED_TYPE, settings.type);
  panel->setText(PID_MATERIAL_TYPE, settings.physMat.str());
}

void NodesProcessing::restoreEditInfoPanel(const SelectedNodesSettings &settings)
{
  Tab<String> nodes;
  Tab<int> sels;
  panel->getStrings(PID_SELECTABLE_NODES_LIST, nodes);
  if (selectionNodesProcessing.editSettings)
  {
    fillEditNodes(nodes);
  }
  find_ref_nodes_idxs(settings, nodes, sels);
  set_settings_on_panel(settings, nodes, sels, panel);
  checkCreatedNode();
  setButtonNameByType(static_cast<ExportCollisionNodeType>(panel->getInt(PID_SELECTED_TYPE)));
}

void NodesProcessing::setButtonNameByType(const ExportCollisionNodeType &type)
{
  switch (type)
  {
    case ExportCollisionNodeType::MESH:
    case ExportCollisionNodeType::BOX:
    case ExportCollisionNodeType::SPHERE: panel->setText(PID_NEXT_EDIT_NODE, "Save"); break;

    case ExportCollisionNodeType::KDOP:
    case ExportCollisionNodeType::CONVEX_COMPUTER:
    case ExportCollisionNodeType::CONVEX_VHACD: panel->setText(PID_NEXT_EDIT_NODE, "Next"); break;
  }
}

void NodesProcessing::switchSelectedType()
{
  SelectedNodesSettings newNodeSettings;
  selectionNodesProcessing.getSelectedNodes(newNodeSettings.refNodes);
  newNodeSettings.nodeName = panel->getText(PID_NEW_NODE_NAME);
  newNodeSettings.isTraceable = panel->getBool(PID_TRACEABLE_FLAG);
  newNodeSettings.isPhysCollidable = panel->getBool(PID_PHYS_COLLIDABLE_FLAG);
  newNodeSettings.replaceNodes = panel->getBool(PID_REPLACE_NODE);
  newNodeSettings.type = static_cast<ExportCollisionNodeType>(panel->getInt(PID_SELECTED_TYPE));
  newNodeSettings.physMat = panel->getText(PID_MATERIAL_TYPE);
  setButtonNameByType(newNodeSettings.type);
  if (newNodeSettings.type == ExportCollisionNodeType::KDOP || newNodeSettings.type == ExportCollisionNodeType::CONVEX_COMPUTER ||
      newNodeSettings.type == ExportCollisionNodeType::CONVEX_VHACD)
  {
    newNodeSettings.type = ExportCollisionNodeType::MESH;
  }
  if (selectionNodesProcessing.rejectExportedCollisions(newNodeSettings.refNodes, newNodeSettings.replaceNodes))
  {
    selectedPanelGroup = PID_EDIT_COLLISION_NODE_GROUP;
    newNodeSettings.type = static_cast<ExportCollisionNodeType>(panel->getInt(PID_SELECTED_TYPE));
    combinedNodesProcessing.setSelectedNodesSettings(eastl::move(newNodeSettings));
    add_delayed_action(make_delayed_action(
      [this]() { curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType()); }));
    return;
  }
  combinedNodesProcessing.setSelectedNodesSettings(eastl::move(newNodeSettings));
  combinedNodesProcessing.calcSelectedCombinedNode();
}

void NodesProcessing::fillEditNodes(Tab<String> &nodes)
{
  const SelectedNodesSettings &settings = *selectionNodesProcessing.editSettings;
  for (int i = 0; i < nodes.size(); ++i)
  {
    if (nodes[i] == settings.nodeName)
    {
      nodes.erase(nodes.begin() + i);
      break;
    }
  }
  for (const auto &refNode : settings.refNodes)
  {
    if (settings.replaceNodes || eastl::find(nodes.begin(), nodes.end(), refNode) == nodes.end())
    {
      nodes.push_back() = refNode;
    }
  }
}

void NodesProcessing::delete_flags_prefix(String &node_name)
{
  const int prefixTraceLength = strlen("[trace]");
  if (!dd_strnicmp(node_name, "[trace]", prefixTraceLength))
    node_name.setStr(node_name.str() + prefixTraceLength);
  const int prefixPhysLength = strlen("[phys]");
  if (!dd_strnicmp(node_name, "[phys]", prefixPhysLength))
    node_name.setStr(node_name.str() + prefixPhysLength);
}

void NodesProcessing::editCollisionNode()
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_COLLISION_NODES_TREE)->getContainer();
  PropPanel::TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || !tree->getChildCount(leaf))
    return;

  String nodeName = tree->getCaption(leaf);
  delete_flags_prefix(nodeName);
  if (dag::Vector<String> nodes{nodeName}; selectionNodesProcessing.rejectExportedCollisions(nodes, true))
  {
    selectedPanelGroup = PID_EDIT_NODE;
    combinedNodesProcessing.selectedSettings.nodeName = nodeName;
    curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
    return;
  }

  removePanelGroupDelayed(PID_COLLISION_INFO_GROUP);
  editCollisionNode(nodeName);
}

void NodesProcessing::editCollisionNode(const String &node_name)
{
  fillEditNodeInfoPanel();
  editMode = true;
  selectionNodesProcessing.findSelectedNodeSettings(node_name);
  if (!selectionNodesProcessing.editSettings)
    return;
  const SelectedNodesSettings &settings = *selectionNodesProcessing.editSettings;
  Tab<String> nodes;
  Tab<int> sels;
  panel->getStrings(PID_SELECTABLE_NODES_LIST, nodes);
  fillEditNodes(nodes);
  if (settings.replaceNodes)
  {
    const int startIdx = nodes.size() - settings.refNodes.size();
    for (int i = startIdx; i < nodes.size(); ++i)
    {
      sels.push_back(i);
    }
  }
  else
  {
    find_ref_nodes_idxs(settings, nodes, sels);
  }
  set_settings_on_panel(settings, nodes, sels, panel);
  switchSelectedType();
  checkCreatedNode();
}

void NodesProcessing::deleteCollisionNode()
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_COLLISION_NODES_TREE)->getContainer();
  PropPanel::TLeafHandle leaf = tree->getSelLeaf();
  String nodeName = tree->getCaption(leaf);
  if (nodeName.empty() || !tree->getChildCount(leaf))
    return;
  tree->removeLeaf(leaf);
  delete_flags_prefix(nodeName);
  if (!selectionNodesProcessing.deleteNodeFromBlk(nodeName))
  {
    ExportCollisionNodeType type;
    deleteNodeFromProcessing(nodeName, type, selectionNodesProcessing.deleteSelectedNode(nodeName, type));
    tree->clear();
    selectionNodesProcessing.fillInfoTree(tree);
  }
  else
  {
    selectedPanelGroup = PID_COLLISION_INFO_GROUP;
    if (curAsset->isVirtual())
    {
      curAsset->props.setStr("name", curAsset->getSrcFileName());
      curAsset->props.saveToTextFile(String(0, "%s/%s.collision.blk", curAsset->getFolderPath(), curAsset->getName()));
    }
    curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
  }
  checkSelectedTreeLeaf();
  selectionNodesProcessing.updateHiddenNodes();
}

void NodesProcessing::deleteNodeFromProcessing(const String &node_name, const ExportCollisionNodeType &type, const int idx)
{
  if (idx > -1)
  {
    switch (type)
    {
      case ExportCollisionNodeType::MESH:
      case ExportCollisionNodeType::BOX:
      case ExportCollisionNodeType::SPHERE: combinedNodesProcessing.nodes.erase(combinedNodesProcessing.nodes.begin() + idx); break;

      case ExportCollisionNodeType::KDOP: kdopSetupProcessing.kdops.erase(kdopSetupProcessing.kdops.begin() + idx); break;

      case ExportCollisionNodeType::CONVEX_COMPUTER:
        convexComputerProcessing.computers.erase(convexComputerProcessing.computers.begin() + idx);
        break;

      case ExportCollisionNodeType::CONVEX_VHACD:
        convexVhacdProcessing.interfaces.erase(convexVhacdProcessing.interfaces.begin() + idx);
        break;
    }
  }
}

void NodesProcessing::checkCreatedNode()
{
  Tab<int> sels;
  panel->getSelection(PID_SELECTABLE_NODES_LIST, sels);
  bool isButtonEnabled = sels.size() > 0;
  String nodeName(panel->getText(PID_NEW_NODE_NAME));
  nodeName.toLower();

  if (!selectionNodesProcessing.editSettings || selectionNodesProcessing.editSettings->nodeName != nodeName)
  {
    if (nodeName.empty() || !selectionNodesProcessing.checkNodeName(nodeName))
      isButtonEnabled = false;
  }
  panel->setEnabledById(PID_NEXT_EDIT_NODE, isButtonEnabled);
}

void NodesProcessing::checkSelectedTreeLeaf()
{
  PropPanel::PropertyControlBase *treeControl = panel->getById(PID_COLLISION_NODES_TREE);
  if (!treeControl)
    return;

  PropPanel::ContainerPropertyControl *tree = treeControl->getContainer();
  PropPanel::TLeafHandle leaf = tree->getSelLeaf();
  const bool isButtonEnabled = leaf && tree->getChildCount(leaf) && !tree->getCaption(leaf).empty();
  panel->setEnabledById(PID_EDIT_NODE, isButtonEnabled);
  panel->setEnabledById(PID_DELETE_NODE, isButtonEnabled);
}
