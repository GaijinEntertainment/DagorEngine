// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "selectionNodesProcessing.h"

namespace PropPanel
{
class ITreeControlEventHandler;
}

class NodesProcessing
{
public:
  NodesProcessing();

  CombinedNodesProcessing combinedNodesProcessing;
  KdopProcessing kdopSetupProcessing;
  ConvexHullComputerProcessing convexComputerProcessing;
  ConvexVhacdProcessing convexVhacdProcessing;
  SelectionNodesProcessing selectionNodesProcessing;

  bool editMode = false;
  int selectedPanelGroup;

  void init(DagorAsset *curAsset, CollisionResource *collision_res, PropPanel::ITreeControlEventHandler *event_handler);
  void setPropPanel(PropPanel::ContainerPropertyControl *prop_panel);
  void renderNodes(int selected_node_id, bool draw_solid);
  void onClick(int pcb_id);
  void onChange(int pcb_id);
  void selectNode(const char *node_name, bool ctrl_pressed);
  void saveCollisionNodes();
  void saveExportedCollisionNodes();
  void fillCollisionInfoPanel();
  void fillEditNodeInfoPanel();
  void setPanelAfterReject();
  bool canChangeAsset();

  static void delete_flags_prefix(String &node_name);

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
  DagorAsset *curAsset = nullptr;
  PropPanel::ITreeControlEventHandler *treeEventHandler = nullptr;

  void calcCollisionsAfterReject();
  void switchPropPanel(int pcb_id);
  void switchPanelByType(int pcb_id, SelectedNodesSettings &&new_node_settings);
  void saveNewNode(int pcb_id);
  void cancelSwitchPanel(int pcb_id);
  void removePanelGroupDelayed(int pcb_id);
  void restoreEditInfoPanel(const SelectedNodesSettings &settings);
  void setButtonNameByType(const ExportCollisionNodeType &type);
  void switchSelectedType();
  void fillEditNodes(Tab<String> &nodes);
  void editCollisionNode();
  void editCollisionNode(const String &node_name);
  void deleteCollisionNode();
  void deleteNodeFromProcessing(const String &node_name, const ExportCollisionNodeType &type, const int idx);
  void checkCreatedNode();
  void checkSelectedTreeLeaf();
};
