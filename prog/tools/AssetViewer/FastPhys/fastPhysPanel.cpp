// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/fastPhysData/fp_data.h>
#include <libTools/fastPhysData/fp_edbone.h>
#include <libTools/fastPhysData/fp_edclipper.h>
#include <libTools/fastPhysData/fp_edpoint.h>

#include "../av_plugin.h"

#include "fastPhysObject.h"
#include "fastPhysEd.h"
#include "fastPhysPanel.h"

//------------------------------------------------------------------

void ActionsTreeCB::onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel)
{
  // objEd->getUndoSystem()->begin();

  if (mPlugin.getEditor()->selProcess)
    return;

  mPlugin.getEditor()->selProcess = true;
  mPlugin.getEditor()->unselectAll();

  FpdAction *action = (FpdAction *)tree.getItemData(new_sel);
  FpdObject *obj = (action) ? action->getObject() : NULL;

  if (obj)
  {
    IFPObject *wobj = mPlugin.getEditor()->getWrapObjectByName(obj->getName());

    if (wobj)
    {
      wobj->selectObject();
    }
  }

  mPlugin.getEditor()->selProcess = false;

  // objEd->getUndoSystem()->accept("Select");
}


void ActionsTreeCB::onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) {}


bool ActionsTreeCB::onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) { return false; }


//------------------------------------------------------------------

void FPPanel::refillPanel()
{
  PropPanel::ContainerPropertyControl *panel = mPlugin.getPluginPanel();
  if (panel)
  {
    panel->clear();
    panel->setEventHandler(this);

    IFPObject *wobj = mPlugin.getEditor()->getSelObject();
    int _cnt = mPlugin.getEditor()->selectedCount();

    if (wobj && _cnt == 1)
    {
      if (wobj->getObject())
        panel->createEditBox(PID_OBJ_NAME, "Action:", wobj->getObject()->getName(), false);

      wobj->refillPanel(panel);
    }
    else
    {
      String s(256, "Selected %d objects", _cnt);
      panel->createStatic(PID_OBJ_NAME, s.str());
    }
    PropPanel::ContainerPropertyControl &fpGrp = *panel->createGroup(PID_FAST_PHYS_PARAMS_GROUP, "Fast phys wind test");
    if (!mPlugin.mRespondsToWind)
      fpGrp.createStatic(PID_WIND_ENABLED, "Wind is disabled.\n(Missing integrate inside fast phys)");
    fpGrp.createPoint3(PID_WIND_VEL, "Wind vel", mPlugin.mWindVel, 2, mPlugin.mRespondsToWind);
    fpGrp.createEditFloat(PID_WIND_POWER, "Wind power", mPlugin.mWindPower, 2, mPlugin.mRespondsToWind);
    fpGrp.createEditFloat(PID_WIND_TURB, "Wind turbulence", mPlugin.mWindTurb, 2, mPlugin.mRespondsToWind);
    fpGrp.createButton(PID_WIND_RESET, "Reset", mPlugin.mRespondsToWind);
  }
}


void FPPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_WIND_VEL || pcb_id == PID_WIND_POWER || pcb_id == PID_WIND_TURB)
  {
    mPlugin.mWindVel = panel->getPoint3(PID_WIND_VEL);
    mPlugin.mWindPower = panel->getFloat(PID_WIND_POWER);
    mPlugin.mWindTurb = panel->getFloat(PID_WIND_TURB);
    mPlugin.getEditor()->setWind(panel->getPoint3(PID_WIND_VEL), panel->getFloat(PID_WIND_POWER), panel->getFloat(PID_WIND_TURB));
  }

  IFPObject *wobj = mPlugin.getEditor()->getSelObject();

  if (wobj)
    wobj->onChange(pcb_id, panel);
}


void FPPanel::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_WIND_RESET)
  {
    mPlugin.getEditor()->setWind(Point3(0, 0, 0), 0.f, 0.f);
    mPlugin.mWindVel = Point3(0, 0, 0);
    mPlugin.mWindPower = 0.f;
    mPlugin.mWindTurb = 0.f;
    panel->setPoint3(PID_WIND_VEL, Point3(0, 0, 0));
    panel->setFloat(PID_WIND_POWER, 0.f);
    panel->setFloat(PID_WIND_TURB, 0.f);
  }

  IFPObject *wobj = mPlugin.getEditor()->getSelObject();

  if (wobj)
    wobj->onClick(pcb_id, panel);
}


//------------------------------------------------------------------
