// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "../blendNodes/blendNodeData.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/treeInterface.h>
#include <sepGui/wndMenuInterface.h>

class DagorAsset;
class DataBlock;

class CtrlChildsDialog : public PropPanel::DialogWindow, public PropPanel::ITreeControlEventHandler, public IMenuEventHandler
{
public:
  CtrlChildsDialog(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes, dag::Vector<String> &paths);
  virtual bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
    PropPanel::ITreeInterface &tree_interface) override;
  virtual int onMenuItemClick(unsigned id) override;
  void initPanel();
  void clear();
  void setTreePanels(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset, PropPanel::ControlEventHandler *event_handler);
  void fillChildsTree(PropPanel::TLeafHandle leaf);
  void fillChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data,
    const String &prefix_name);
  const char *getChildNameFromSettings(const DataBlock &settings, CtrlType type, int idx);
  String getImportantParamPrefixName(const DataBlock &settings, int child_idx, CtrlType type);

private:
  void editSelectedNode();
  void editInParentSelectedNode();

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 450;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;

  // Used for get caption by handle
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  PropPanel::ContainerPropertyControl *nodesTree = nullptr;

  PropPanel::ContainerPropertyControl *pluginPanel = nullptr;
  PropPanel::ControlEventHandler *pluginEventHandler = nullptr;
};