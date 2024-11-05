// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision_panel.h"
#include "clippingPlugin.h"

#include <oldEditor/de_clipping.h>

#include <libTools/dagFileRW/dagUtil.h>

#include <scene/dag_physMat.h>
#include <perfMon/dag_visClipMesh.h>
#include <osApiWrappers/dag_direct.h>
#include <coolConsole/coolConsole.h>

#include <debug/dag_debug.h>

#include <sepGui/wndPublic.h>


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
  CM_PID_EDITOR_BOX_ACTORS,
  CM_PID_EDITOR_SPH_ACTORS,
  CM_PID_EDITOR_CAP_ACTORS,
  CM_PID_EDITOR_MESH_ACTORS,

  CM_PID_MATER_COLOR_BROSE_BASE,
  CM_PID_EDITOR_RT_GRID_STEP,
  CM_PID_EDITOR_RT_MIN_MUTUAL_OVERLAP,
  CM_PID_EDITOR_RT_MIN_FACE_CNT,
  CM_PID_EDITOR_RT_MIN_SMALL_OVERLAP,

  CM_PID_MATERIAL_GRP_BASE,
  CM_PID_MATERIAL_COLOR_BASE,
  // CM_PID_MATERIAL_NAME_BASE,
};

enum
{
  PROPBAR_WIDTH = 280,
};


CollisionPropPanelClient::CollisionPropPanelClient(ClippingPlugin *plg, CollisionBuildSettings &_stg, int &cur_phys_eng_type) :
  plugin(plg), stg(_stg), curPhysEngType(cur_phys_eng_type), mPanelWindow(NULL)
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

  maxGroup->createEditFloat(CM_PID_EDITOR_VCM_RAD, "Visibility radius:", get_vcm_rad());
  maxGroup->createStatic(0, "Preview");
  maxGroup->createCheckBox(CM_PID_SHOW_GAME_FRT, "Game", plugin->showGameFrt, true, false);
  maxGroup->createCheckBox(CM_PID_SHOW_ED_FRT, "Editor", !plugin->showGameFrt, true, false);
  maxGroup->createCheckBox(CM_PID_SHOW_VCM, "Show collision", plugin->showVcm);
  maxGroup->createCheckBox(CM_PID_SHOW_VCM_WIRE, "Render as wireframe", plugin->showVcmWire);
  maxGroup->createSeparator(0);
  maxGroup->createCheckBox(CM_PID_SHOW_DAGS, "Show DAGs collision", plugin->showDags);
  maxGroup->createSeparator(0);
  maxGroup->createStatic(0, "Game collision preview:");
  maxGroup->createCheckBox(CM_PID_EDITOR_BOX_ACTORS, "Box objects", plugin->showGcBox);
  maxGroup->createCheckBox(CM_PID_EDITOR_SPH_ACTORS, "Sphere objects", plugin->showGcSph);
  maxGroup->createCheckBox(CM_PID_EDITOR_CAP_ACTORS, "Capsule objects", plugin->showGcCap);
  maxGroup->createCheckBox(CM_PID_EDITOR_MESH_ACTORS, "Tri-mesh objects", plugin->showGcMesh);
  maxGroup->createIndent();
  maxGroup->createSeparator(0);
  maxGroup->createPoint3(CM_PID_EDITOR_LEAF_SIZE, "Leaf size:", stg.leafSize());
  maxGroup->createEditInt(CM_PID_EDITOR_LEVELS_COUNT, "Levels count:", stg.levels);
  maxGroup->createIndent();

  if (curPhysEngType == ClippingPlugin::PHYSENG_Bullet)
  {
    maxGroup->createStatic(0, "Bullet settings");
    maxGroup->createEditFloat(CM_PID_EDITOR_RT_GRID_STEP, "Actors grid step", stg.gridStep);
    maxGroup->createEditFloat(CM_PID_EDITOR_RT_MIN_MUTUAL_OVERLAP, "Merge actors thres.(%)", stg.minMutualOverlap * 100);
    maxGroup->createIndent();
    maxGroup->createEditInt(CM_PID_EDITOR_RT_MIN_FACE_CNT, "Small actor max faces", stg.minFaceCnt);
    maxGroup->createEditFloat(CM_PID_EDITOR_RT_MIN_SMALL_OVERLAP, "Merge small actor thres.(%)", stg.minSmallOverlap * 100);
  }

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
  bool changed = false;

  switch (pcb_id)
  {
    case CM_PID_EDITOR_VCM_RAD:
      set_vcm_rad(panel->getFloat(pcb_id));
      changed = true;
      break;

    case CM_PID_SHOW_VCM:
      plugin->showVcm = panel->getBool(pcb_id);
      ::set_vcm_visible(plugin->getVisible() ? plugin->showVcm : false);
      DAGORED2->invalidateViewportCache();
      break;

    case CM_PID_SHOW_VCM_WIRE:
      plugin->showVcmWire = panel->getBool(pcb_id);
      ::set_vcm_draw_type(plugin->showVcmWire);
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

    case CM_PID_EDITOR_BOX_ACTORS: plugin->showGcBox = panel->getBool(pcb_id); break;

    case CM_PID_EDITOR_SPH_ACTORS: plugin->showGcSph = panel->getBool(pcb_id); break;

    case CM_PID_EDITOR_CAP_ACTORS: plugin->showGcCap = panel->getBool(pcb_id); break;

    case CM_PID_EDITOR_MESH_ACTORS: plugin->showGcMesh = panel->getBool(pcb_id); break;

    case CM_PID_EDITOR_LEAF_SIZE:
      stg.leafSize() = panel->getPoint3(pcb_id);
      changed = true;
      break;

    case CM_PID_EDITOR_LEVELS_COUNT:
      stg.levels = panel->getInt(pcb_id);
      changed = true;
      break;

    case CM_PID_EDITOR_RT_GRID_STEP:
      stg.gridStep = panel->getFloat(pcb_id);
      changed = true;
      break;

    case CM_PID_EDITOR_RT_MIN_MUTUAL_OVERLAP:
      stg.minMutualOverlap = panel->getFloat(pcb_id) * 0.01;
      changed = true;
      break;

    case CM_PID_EDITOR_RT_MIN_SMALL_OVERLAP:
      stg.minSmallOverlap = panel->getFloat(pcb_id) * 0.01;
      changed = true;
      break;

    case CM_PID_EDITOR_RT_MIN_FACE_CNT:
      stg.minFaceCnt = panel->getInt(pcb_id);
      changed = true;
      break;
  }


  dag::Span<PhysMat::MaterialData> info_mats = PhysMat::getMaterials();
  E3DCOLOR c;

  if ((pcb_id > CM_PID_MATERIAL_GRP_BASE) && (pcb_id < CM_PID_MATERIAL_COLOR_BASE + MATER_PARAMS_NUM * info_mats.size()))
  {
    int index = (pcb_id - CM_PID_MATERIAL_COLOR_BASE) / MATER_PARAMS_NUM;
    changeColor(index, panel->getColor(pcb_id));
    changed = true;
  }

  if (changed)
  {
    plugin->initClipping(false);
    DAGORED2->repaint();
  }
}

void CollisionPropPanelClient::changeColor(int ind, E3DCOLOR new_color)
{
  const char *paramsFile = ClippingPlugin::getPhysMatPath(&(DAGORED2->getConsole()));
  if (!paramsFile)
    return;

  DataBlock blk;
  DataBlock *materialsBlk, *defMatBlk;
  if (!ClippingPlugin::isValidPhysMatBlk(paramsFile, blk, NULL, &materialsBlk, &defMatBlk))
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
