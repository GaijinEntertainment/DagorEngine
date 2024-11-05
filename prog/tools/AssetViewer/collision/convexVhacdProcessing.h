// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "collisionNodesSettings.h"
#include <math/vhacd/VHACD.h>
#include <propPanel/control/container.h>

class ConvexVhacdProcessing
{
public:
  class VhacdProgress : public VHACD::IVHACD::IUserCallback
  {
  public:
    virtual void Update(const double overallProgress, const double stageProgress, const char *const stage,
      const char *operation) override;
    virtual void NotifyVHACDComplete() override;
    ConvexVhacdProcessing *processing = nullptr;
    bool isComputed = false;
  };

  VhacdProgress progressCallback;
  ConvexVhacdSettings selectedSettings;
  VHACD::IVHACD *selectedInterface = VHACD::CreateVHACD_ASYNC();
  dag::Vector<VHACD::IVHACD *> interfaces;
  bool showConvexVHACD = true;

  void init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel);
  void calcSelectedInterface();
  void calcInterface(const ConvexVhacdSettings &settings);
  void setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes);
  void addSelectedInterface();
  void printCalcProgress(const double progress);
  void setConvexVhacdParams(const ConvexVhacdSettings &settings);
  void updatePanelParams();
  void checkComputedInterface();
  void renderVhacd(bool is_faded);
  void fillConvexVhacdPanel();

private:
  PropPanel::ContainerPropertyControl *panel = nullptr;
  CollisionResource *collisionRes = nullptr;
};
