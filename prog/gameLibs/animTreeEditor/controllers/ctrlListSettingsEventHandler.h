// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animCtrlData.h"
#include "../nodeListEventHandler.h"

class CtrlListSettingsEventHandler : public NodeListEventHandler
{
public:
  CtrlListSettingsEventHandler(dag::Vector<AnimCtrlData> &controllers);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;

private:
  void editSelectedNode() override;

  dag::Vector<AnimCtrlData> &controllers;
};
