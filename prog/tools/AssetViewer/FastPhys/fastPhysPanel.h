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
  PID_WIND_VEL,
  PID_WIND_POWER,
  PID_WIND_TURB,
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

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  virtual void onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) override;
  virtual bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

protected:
  FastPhysPlugin &mPlugin;
};

//------------------------------------------------------------------

class FPPanel : public PropPanel::ControlEventHandler
{
public:
  FPPanel(FastPhysPlugin &plugin) : mPlugin(plugin) {}
  virtual ~FPPanel(){};

  void refillPanel();

  // ControlEventHandler

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

protected:
  FastPhysPlugin &mPlugin;
};


//------------------------------------------------------------------
