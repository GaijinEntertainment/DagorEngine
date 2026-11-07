// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../controllers/animCtrlData.h"
#include "../nodeListEventHandler.h"
#include <propPanel/c_common.h>

class ResetRandomSwitchListEventHandler : public NodeListEventHandler
{
public:
  ResetRandomSwitchListEventHandler(dag::Vector<AnimCtrlData> &controllers);
  bool onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface) override;

private:
  void editSelectedNode() override;

  dag::Vector<AnimCtrlData> &controllers;
};
