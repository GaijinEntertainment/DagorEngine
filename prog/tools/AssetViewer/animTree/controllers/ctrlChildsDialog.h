// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "../blendNodes/blendNodeData.h"
#include <propPanel/commonWindow/dialogWindow.h>

class DagorAsset;
class DataBlock;

class CtrlChildsDialog : public PropPanel::DialogWindow
{
public:
  CtrlChildsDialog(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes, dag::Vector<String> &paths);
  void initPanel();
  void clear();
  void setTreePanels(PropPanel::ContainerPropertyControl *ctrls_tree, PropPanel::ContainerPropertyControl *nodes_tree,
    DagorAsset *cur_asset);
  void fillChildsTree(PropPanel::TLeafHandle leaf);
  void fillChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data);
  const char *getChildNameFromSettings(const DataBlock &settings, CtrlType type, int idx);

private:
  virtual void updateImguiDialog() override;

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 450;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<BlendNodeData> &nodes;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;

  // Used for get caption by handle
  PropPanel::ContainerPropertyControl *ctrlsTree = nullptr;
  PropPanel::ContainerPropertyControl *nodesTree = nullptr;
};