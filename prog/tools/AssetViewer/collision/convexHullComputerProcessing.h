// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "collisionNodesSettings.h"
#include <math/dag_convexHullComputer.h>
#include <propPanel/control/container.h>
class ConvexHullComputerProcessing
{
public:
  ConvexComputerSettings selectedSettings;
  ConvexHullComputer selectedComputer;
  dag::Vector<ConvexHullComputer> computers;
  bool showConvexComputed = true;

  void init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel);
  void calcSelectedComputer();
  void calcComputer(const ConvexComputerSettings &settings);
  void setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes);
  void addSelectedComputer();
  void setConvexComputerParams(const ConvexComputerSettings &settings);
  void updatePanelParams();
  void renderComputedConvex(bool is_faded);
  void fillConvexComputerPanel();

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
};
