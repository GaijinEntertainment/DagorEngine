// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "collisionNodesSettings.h"
#include <propPanel/control/container.h>

class KdopProcessing
{
public:
  KdopSettings selectedSettings;
  Kdop selectedKdop;
  dag::Vector<Kdop> kdops;

  bool showKdop = true;
  bool showKdopFaces = false;
  bool showKdopDirs = false;

  void init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel);
  void calcSelectedKdop();
  void calcKdop(const KdopSettings &settings);
  void setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes);
  void addSelectedKdop();
  void setKdopParams(const KdopSettings &settings);
  void updatePanelParams();
  void renderKdop(bool is_faded);
  void fillKdopPanel();

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
  void switchSlidersByPreset();
};
