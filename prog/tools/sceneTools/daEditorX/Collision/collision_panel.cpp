// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision_panel.h"
#include "collisionPlugin.h"

#include <oldEditor/de_collision.h>

#include <libTools/dagFileRW/dagUtil.h>

#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_direct.h>
#include <coolConsole/coolConsole.h>

#include <debug/dag_debug.h>

#include <EditorCore/ec_wndPublic.h>


#define BROSE_CAPTION_TEMP "Browse...(temp)"
#define MATER_PARAMS_NUM   2


enum
{
  CM_PID_EDITOR_GRP,
  CM_PID_EDITOR_LEAF_SIZE,
  CM_PID_EDITOR_LEVELS_COUNT,
  CM_PID_EDITOR_VCM_RAD,
  CM_PID_SHOW_VCM,
  CM_PID_SHOW_ED_FRT,
  CM_PID_SHOW_GAME_FRT,
  CM_PID_SHOW_VCM_WIRE,
  CM_PID_SHOW_DAGS,

  CM_PID_MATER_COLOR_BROSE_BASE,

  CM_PID_MATERIAL_GRP_BASE,
  CM_PID_MATERIAL_COLOR_BASE,
  // CM_PID_MATERIAL_NAME_BASE,
};

enum
{
  PROPBAR_WIDTH = 280,
};


CollisionPropPanelClient::CollisionPropPanelClient(CollisionPlugin *plg, CollisionBuildSettings &_stg) :
  plugin(plg), stg(_stg), mPanelWindow(NULL)
{}


bool CollisionPropPanelClient::showPropPanel(bool show)
{
  if (show != (bool)(mPanelWindow))
  {
    if (show)
      EDITORCORE->addPropPanel(PROPBAR_EDITOR_WTYPE, hdpi::_pxScaled(PROPBAR_WIDTH));
    else
      EDITORCORE->removePropPanel(mPanelWindow);
  }

  return true;
}


void CollisionPropPanelClient::setCollisionParams()
{
  mPanelWindow->createStatic(0, String(32, "Physic engine: %s", DagorPhys::get_collision_name()));

  dag::Span<PhysMat::MaterialData> info_mats = PhysMat::getMaterials();

  PropPanel::ContainerPropertyControl *maxGroup = mPanelWindow->createGroup(CM_PID_EDITOR_GRP, "Collision parameters");

  maxGroup->createEditFloat(CM_PID_EDITOR_VCM_RAD, "Visibility radius:", plugin->vcmRad);
  maxGroup->createStatic(0, "Preview");
  maxGroup->createCheckBox(CM_PID_SHOW_GAME_FRT, "Game", plugin->showGameFrt, true, false);
  maxGroup->createCheckBox(CM_PID_SHOW_ED_FRT, "Editor", !plugin->showGameFrt, true, false);
  maxGroup->createCheckBox(CM_PID_SHOW_VCM, "Show collision", plugin->showVcm);
  maxGroup->createCheckBox(CM_PID_SHOW_VCM_WIRE, "Render as wireframe", plugin->showVcmWire);
  maxGroup->createSeparator(0);
  maxGroup->createCheckBox(CM_PID_SHOW_DAGS, "Show DAGs collision", plugin->showDags);
  maxGroup->createSeparator(0);
  maxGroup->createPoint3(CM_PID_EDITOR_LEAF_SIZE, "Leaf size:", stg.leafSize);
  maxGroup->createEditInt(CM_PID_EDITOR_LEVELS_COUNT, "Levels count:", stg.levels);
  maxGroup->createIndent();

  for (int i = 0; i < info_mats.size(); i++)
  {
    maxGroup = mPanelWindow->createGroupBox(CM_PID_MATERIAL_GRP_BASE + MATER_PARAMS_NUM * i, info_mats[i].name);
    maxGroup->createColorBox(CM_PID_MATERIAL_COLOR_BASE + MATER_PARAMS_NUM * i, "Color:", info_mats[i].vcm_color);
  }

  mPanelWindow->loadState(plugin->mainPanelState);
}


void CollisionPropPanelClient::fillPanel() { setCollisionParams(); }


void CollisionPropPanelClient::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_PID_EDITOR_VCM_RAD:
      plugin->vcmRad = panel->getFloat(pcb_id);
      DAGORED2->repaint();
      break;

    case CM_PID_SHOW_VCM:
      plugin->showVcm = panel->getBool(pcb_id);
      DAGORED2->invalidateViewportCache();
      break;

    case CM_PID_SHOW_VCM_WIRE:
      plugin->showVcmWire = panel->getBool(pcb_id);
      DAGORED2->invalidateViewportCache();
      break;

    case CM_PID_SHOW_DAGS: plugin->showDags = panel->getBool(pcb_id); break;

    case CM_PID_SHOW_GAME_FRT:
      plugin->showGameFrt = panel->getBool(pcb_id);
      panel->setBool(CM_PID_SHOW_ED_FRT, !plugin->showGameFrt);
      break;

    case CM_PID_SHOW_ED_FRT:
      plugin->showGameFrt = !panel->getBool(pcb_id);
      panel->setBool(CM_PID_SHOW_GAME_FRT, plugin->showGameFrt);
      break;

    case CM_PID_EDITOR_LEAF_SIZE: stg.leafSize = panel->getPoint3(pcb_id); break;

    case CM_PID_EDITOR_LEVELS_COUNT: stg.levels = panel->getInt(pcb_id); break;
  }


  dag::Span<PhysMat::MaterialData> info_mats = PhysMat::getMaterials();
  E3DCOLOR c;

  if ((pcb_id > CM_PID_MATERIAL_GRP_BASE) && (pcb_id < CM_PID_MATERIAL_COLOR_BASE + MATER_PARAMS_NUM * info_mats.size()))
  {
    int index = (pcb_id - CM_PID_MATERIAL_COLOR_BASE) / MATER_PARAMS_NUM;
    changeColor(index, panel->getColor(pcb_id));
    DAGORED2->repaint();
  }
}

void CollisionPropPanelClient::changeColor(int ind, E3DCOLOR new_color)
{
  const char *paramsFile = CollisionPlugin::getPhysMatPath(&(DAGORED2->getConsole()));
  if (!paramsFile)
    return;

  DataBlock blk;
  DataBlock *materialsBlk, *defMatBlk;
  if (!CollisionPlugin::isValidPhysMatBlk(paramsFile, blk, NULL, &materialsBlk, &defMatBlk))
    return;

  int m;
  for (m = 0; m < materialsBlk->blockCount(); m++)
  {
    if (m != ind + 1)
      continue;
    DataBlock *matBlk = materialsBlk->getBlock(m);
    // if (matBlk == defMatBlk) continue;
    matBlk->setE3dcolor("color", new_color);
  }
}


void CollisionPropPanelClient::setPanelParams() { fillPanel(); }
