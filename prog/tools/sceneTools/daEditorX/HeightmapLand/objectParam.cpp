// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "objectParam.h"
#include "hmlPlugin.h"
#include "hmlPanel.h"
#include "hmlEntity.h"

#include <de3_objEntity.h>
#include <de3_interface.h>
#include <debug/dag_debug.h>

#include <propPanel/control/container.h>


static HmapLandPlugin *&plugin = HmapLandPlugin::self;

enum GroupEnum
{
  PID_OBJECT_PARAM_GRP,

  PID_OBJECT_PARAM_FX_GRP,
  PID_OBJECT_PARAM_PHYS_OBJ_GRP,
};

enum ParamEnum
{
  CM_OBJECT_PARAM_ID = 77,

  // FX
  ID_FX_MAX_RADIUS,
  ID_FX_UPDATE_WHEN_INVISIBLE,

  // PhysObj
  ID_PHYS_OBJ_SCRIPT_CLASS,
  ID_PHYS_OBJ_ACTIVE,
};


static void fillFxObj(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> selection, bool expanded)
{
  PropPanel::ContainerPropertyControl &grp = *panel.createGroup(PID_OBJECT_PARAM_FX_GRP, "fx");

  real radius = -1.f;
  int update = -1;

  int cnt = selection.size();
  for (int i = 0; i < cnt; i++)
  {
    LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
    IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
    if (!obj)
      continue;

    if (radius == -1.f)
      radius = leObj->fxProps.maxRadius;
    else if (radius != leObj->fxProps.maxRadius)
      radius = -2.f;

    if (update == -1)
      update = leObj->fxProps.updateWhenInvisible ? 1 : 0;
    else if (update != (leObj->fxProps.updateWhenInvisible ? 1 : 0))
      update = 2;
  }

  grp.createEditFloat(ID_FX_MAX_RADIUS, "Max radius", radius);
  grp.setMinMaxStep(ID_FX_MAX_RADIUS, 0.f, 100000.f, 0.1f);
  grp.createCheckBox(ID_FX_UPDATE_WHEN_INVISIBLE, "Update when invisible", update);

  panel.setBool(PID_OBJECT_PARAM_FX_GRP, !expanded);
}

static void fillPhysObj(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> selection,
  bool expanded)
{
  PropPanel::ContainerPropertyControl &grp = *panel.createGroup(PID_OBJECT_PARAM_PHYS_OBJ_GRP, "physObj");

  String script;
  int str = -1;
  int active = -1;

  int cnt = selection.size();
  for (int i = 0; i < cnt; i++)
  {
    LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
    IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
    if (!obj)
      continue;

    if (str == -1)
    {
      str = 0;
      script = leObj->physObjProps.scriptClass;
    }
    else if (stricmp(leObj->physObjProps.scriptClass, script) != 0)
      str = -2;

    if (active == -1)
      active = leObj->physObjProps.active ? 1 : 0;
    else if (active != (leObj->physObjProps.active ? 1 : 0))
      active = 2;
  }

  grp.createEditBox(ID_PHYS_OBJ_SCRIPT_CLASS, "Script class", str < 0 ? "" : script);
  grp.createCheckBox(ID_FX_UPDATE_WHEN_INVISIBLE, "Update when invisible", active);

  panel.setBool(PID_OBJECT_PARAM_PHYS_OBJ_GRP, !expanded);
}


void ObjectParam::fillParams(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> selection)
{
  if (selection.size() <= 0)
    return;

  int id = -1;

  int cnt = selection.size();
  for (int i = 0; i < cnt; i++)
  {
    LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
    IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
    if (!obj)
    {
      id = -1;
      break;
    }

    id = obj->getAssetTypeId();
    if (id < 0)
      break;
  }

  PropPanel::ContainerPropertyControl &grp = *panel.createGroup(PID_OBJECT_PARAM_GRP, "Object Property");

  static int fxId = IDaEditor3Engine::get().getAssetTypeId("fx");
  static int physObjId = IDaEditor3Engine::get().getAssetTypeId("physObj");

  bool fxobj = (id == fxId);
  bool physobj = (id == physObjId);
  fillFxObj(grp, selection, fxobj);
  fillPhysObj(grp, selection, physobj);
}

bool ObjectParam::onPPChange(PropPanel::ContainerPropertyControl &panel, int pid, dag::ConstSpan<RenderableEditableObject *> selection)
{
  // fx
  if (pid == ID_FX_MAX_RADIUS)
  {
    float val = panel.getFloat(pid);

    int cnt = selection.size();
    for (int i = 0; i < cnt; i++)
    {
      LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
      IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
      if (!obj)
        continue;

      leObj->fxProps.maxRadius = val;
      leObj->objectPropsChanged();
    }
  }
  else if (pid == ID_FX_UPDATE_WHEN_INVISIBLE)
  {
    bool check = panel.getBool(pid);

    int cnt = selection.size();
    for (int i = 0; i < cnt; i++)
    {
      LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
      IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
      if (!obj)
        continue;

      leObj->fxProps.updateWhenInvisible = check;
      leObj->objectPropsChanged();
    }
  }
  // physObj
  else if (pid == ID_PHYS_OBJ_SCRIPT_CLASS)
  {
    int cnt = selection.size();
    for (int i = 0; i < cnt; i++)
    {
      LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
      IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
      if (!obj)
        continue;

      leObj->physObjProps.scriptClass = panel.getText(pid);
      leObj->objectPropsChanged();
    }
  }
  else if (pid == ID_PHYS_OBJ_ACTIVE)
  {
    bool check = panel.getBool(pid);

    int cnt = selection.size();
    for (int i = 0; i < cnt; i++)
    {
      LandscapeEntityObject *leObj = RTTI_cast<LandscapeEntityObject>(selection[i]);
      IObjEntity *obj = leObj ? leObj->getEntity() : NULL;
      if (!obj)
        continue;

      leObj->physObjProps.active = check;
      leObj->objectPropsChanged();
    }
  }
  else
    return false;

  return true;
}
