// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "controllers/animCtrlData.h"
#include "blendNodes/blendNodeData.h"
#include "animStates/animStatesData.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>

class DagorAsset;
class DataBlock;
struct CachedInclude;

class ChildsDialog : public PropPanel::DialogWindow, public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  ChildsDialog(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes, dag::Vector<AnimStatesData> &states,
    dag::Vector<String> &paths);
  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;
  void initPanel();
  void clear();
  void setTreePanels(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset, PropPanel::ControlEventHandler *event_handler);
  void fillChildsTree(PropPanel::TLeafHandle leaf);
  void fillStateChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf, const DataBlock &enum_root_props);
  void fillCtrlChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data,
    const String &prefix_name, dag::Vector<CachedInclude> &includes, const DataBlock &enum_root_props);
  String getChildNameFromSettings(const DataBlock &settings, CtrlType type, int idx);
  String getImportantParamPrefixName(const DataBlock &settings, int child_idx, CtrlType type, const DataBlock &enum_root_props);

private:
  void editSelectedNode();
  void editInParentSelectedNode();
  void setAllExpanded(bool expanded);
  CachedInclude *findIncludeProps(dag::Vector<CachedInclude> &includes, PropPanel::TLeafHandle include_leaf);

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 450;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;
  dag::Vector<AnimStatesData> &states;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;

  // Used for get caption by handle
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  PropPanel::ContainerPropertyControl *nodesTree = nullptr;
  PropPanel::ContainerPropertyControl *statesTree = nullptr;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};