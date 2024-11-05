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
    PropPanel::ContainerPropertyControl &fpGrp = *panel->createGroup(PID_FAST_PHYS_PARAMS_GROUP, "Fast phys params");
    Point3 windVel;
    float windPower, windTurb;
    mPlugin.getEditor()->getWind(windVel, windPower, windTurb);
    fpGrp.createPoint3(PID_WIND_VEL, "Wind vel", windVel);
    fpGrp.createEditFloat(PID_WIND_POWER, "Wind power", windPower);
    fpGrp.createEditFloat(PID_WIND_TURB, "Wind turbulence", windTurb);
  }
}


void FPPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_WIND_VEL || pcb_id == PID_WIND_POWER || pcb_id == PID_WIND_TURB)
    mPlugin.getEditor()->setWind(panel->getPoint3(PID_WIND_VEL), panel->getFloat(PID_WIND_POWER), panel->getFloat(PID_WIND_TURB));

  IFPObject *wobj = mPlugin.getEditor()->getSelObject();

  if (wobj)
    wobj->onChange(pcb_id, panel);
}


void FPPanel::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  IFPObject *wobj = mPlugin.getEditor()->getSelObject();

  if (wobj)
    wobj->onClick(pcb_id, panel);
}


//------------------------------------------------------------------
