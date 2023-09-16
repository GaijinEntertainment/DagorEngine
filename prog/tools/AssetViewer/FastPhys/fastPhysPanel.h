#pragma once

#include <propPanel2/c_control_event_handler.h>
#include <propPanel2/comWnd/treeview_panel.h>

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

class ActionsTreeCB : public ITreeViewEventHandler
{
public:
  ActionsTreeCB(FastPhysPlugin &plugin) : mPlugin(plugin) {}

  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel);
  virtual void onTvListSelection(TreeBaseWindow &tree, int index);
  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu);

protected:
  FastPhysPlugin &mPlugin;
};

//------------------------------------------------------------------

class FPPanel : public ControlEventHandler
{
public:
  FPPanel(FastPhysPlugin &plugin) : mPlugin(plugin) {}
  virtual ~FPPanel(){};

  void refillPanel();

  // ControlEventHandler

  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);

protected:
  FastPhysPlugin &mPlugin;
};


//------------------------------------------------------------------
