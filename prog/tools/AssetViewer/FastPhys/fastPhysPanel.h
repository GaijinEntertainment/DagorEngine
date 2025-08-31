// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/commonWindow/treeviewPanel.h>

#include "fastPhys.h"

#include "../av_cm.h"

//------------------------------------------------------------------

enum
{
  PID_OBJ_NAME = ID_SPEC_GRP + 1,
  PID_FAST_PHYS_PARAMS_GROUP,
  PID_WIND_ENABLED,
  PID_WIND_VEL,
  PID_WIND_POWER,
  PID_WIND_TURB,
  PID_WIND_RESET,
  PID_OBJ_FIRST,
};


class IGenEditorPlugin;
class FastPhysEditor;
class FpdObject;
class IFPObject;


//------------------------------------------------------------------

class FPPanel;

class ActionsTreeCB : public PropPanel::ITreeViewEventHandler
{
public:
  ActionsTreeCB(FastPhysPlugin &plugin) : mPlugin(plugin) {}

  void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  void onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) override;
  bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

protected:
  FastPhysPlugin &mPlugin;
};

//------------------------------------------------------------------

class FPPanel : public PropPanel::ControlEventHandler
{
public:
  FPPanel(FastPhysPlugin &plugin) : mPlugin(plugin) {}
  ~FPPanel() override {}

  void refillPanel();

  // ControlEventHandler

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

protected:
  FastPhysPlugin &mPlugin;
};


//------------------------------------------------------------------
