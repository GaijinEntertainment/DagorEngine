// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animStatesData.h"
#include "../controllers/animCtrlData.h"
#include "../nodeListEventHandler.h"

class DagorAsset;

class StateListSettingsEventHandler : public NodeListEventHandler
{
public:
  StateListSettingsEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<AnimCtrlData> &controllers,
    dag::Vector<String> &paths);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;
  void setEventHandler(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset, AnimTreeEditor *event_handler);

private:
  void editSelectedNode() override;

  dag::Vector<AnimStatesData> &states;
  dag::Vector<AnimCtrlData> &controllers;
  dag::Vector<String> &paths;
  DagorAsset *asset = nullptr;
};
