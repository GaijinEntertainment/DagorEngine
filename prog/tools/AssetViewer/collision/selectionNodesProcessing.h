// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "combinedNodesProcessing.h"
#include "convexHullComputerProcessing.h"
#include "convexVhacdProcessing.h"
#include "kdopProcessing.h"

class SelectionNodesProcessing
{
public:
  bool calcCollisionAfterReject = false;

  SelectedNodesSettings *editSettings = nullptr;

  dag::Vector<SelectedNodesSettings> combinedNodesSettings;
  dag::Vector<KdopSettings> kdopsSettings;
  dag::Vector<ConvexComputerSettings> convexsComputerSettings;
  dag::Vector<ConvexVhacdSettings> convexsVhacdSettings;

  dag::Vector<SelectedNodesSettings> rejectedCombinedNodesSettings;
  dag::Vector<KdopSettings> rejectedKdopsSettings;
  dag::Vector<ConvexComputerSettings> rejectedConvexsComputerSettings;
  dag::Vector<ConvexVhacdSettings> rejectedConvexsVhacdSettings;

  dag::Vector<String> deleteNodesCandidats;
  dag::Vector<bool> hiddenNodes;

  void init(DagorAsset *asset, CollisionResource *collision_res);
  void setPropPanel(PropPanel::ContainerPropertyControl *prop_panel);
  void fillInfoTree(PropPanel::ContainerPropertyControl *tree);
  void fillNodeNamesTab(Tab<String> &node_names);
  void fillConvexVhacdNamesBlk(Tab<String> &node_names, dag::Vector<String> &skip_nodes);
  void getSelectedNodes(dag::Vector<String> &selected_nodes);
  bool rejectExportedCollisions(dag::Vector<String> &selected_nodes, bool is_replace);
  void checkReplacedNodes(const dag::Vector<String> &ref_nodes, DataBlock *nodes);
  void rejectExportedNode(DataBlock *props, dag::Vector<String> &selected_nodes, dag::Vector<String> &add_ref_nodes, bool is_replace);
  bool checkNodeName(const char *node_name);
  void addCombinedSettings(const SelectedNodesSettings &settings);
  void addKdopSettings(const KdopSettings &settings);
  void addConvexComputerSettings(const ConvexComputerSettings &settings);
  void addConvexVhacdSettings(const ConvexVhacdSettings &settings);
  int deleteSelectedNode(const String &name, ExportCollisionNodeType &deleted_type);
  bool deleteNodeFromBlk(const String &name);
  void findSelectedNodeSettings(const String &name);
  void selectNode(const char *node_name, bool ctrl_pressed);
  void updateHiddenNodes();
  void saveCollisionNodes();
  void saveExportedCollisionNodes();
  bool findSettingsByName(const String &name, KdopSettings &settings);
  bool findSettingsByName(const String &name, ConvexComputerSettings &settings);
  bool findSettingsByName(const String &name, ConvexVhacdSettings &settings);
  void clearRejectedSettings();
  void clearSettings();
  bool haveUnsavedChanges();

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
  DagorAsset *curAsset = nullptr;

  dag::Vector<SelectedNodesSettings> exportedCombinedNodesSettings;
  dag::Vector<KdopSettings> exportedKdopsSettings;
  dag::Vector<ConvexComputerSettings> exportedConvexsComputerSettings;
  dag::Vector<ConvexVhacdSettings> exportedConvexsVhacdSettings;
};
