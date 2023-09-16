#pragma once

#include "collisionNodesSettings.h"
#include <math/dag_convexHullComputer.h>
#include <propPanel2/c_panel_base.h>
class ConvexHullComputerProcessing
{
public:
  ConvexComputerSettings selectedSettings;
  ConvexHullComputer selectedComputer;
  dag::Vector<ConvexHullComputer> computers;
  bool showConvexComputed = true;

  void init(CollisionResource *collision_res, PropertyContainerControlBase *prop_panel);
  void calcSelectedComputer();
  void calcComputer(const ConvexComputerSettings &settings);
  void setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes);
  void addSelectedComputer();
  void setConvexComputerParams(const ConvexComputerSettings &settings);
  void updatePanelParams();
  void renderComputedConvex(bool is_faded);
  void fillConvexComputerPanel();

private:
  PropertyContainerControlBase *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
};
