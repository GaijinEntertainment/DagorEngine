// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_appwnd.h"
#include "av_plugin.h"
#include "av_cm.h"
#include <de3_interface.h>

extern void init_plugin_prefabs();
extern void init_plugin_textures();
extern void init_plugin_materials();
extern void init_plugin_entities();
extern void init_plugin_grass();
extern void init_plugin_splines();
extern void init_plugin_tif_mask();
extern void init_plugin_land_class();
extern void init_plugin_nodes();
extern void init_plugin_collision();
extern void init_plugin_physobj();
extern void init_plugin_effects();
extern void init_plugin_vehicle();
extern void init_plugin_fastphys();
extern void init_plugin_rndgrass();
extern void init_plugin_csg_entities();
extern void init_plugin_wt_unit();
extern void init_plugin_a2d();
extern void init_plugin_anim_tree();
extern void init_plugin_ecs_templates();
// extern void init_plugin_custom();

void init_all_editor_plugins()
{
#define INIT_SERVICE(TYPE_NAME, EXPR)           \
  if (DAEDITOR3.getAssetTypeId(TYPE_NAME) >= 0) \
    EXPR;                                       \
  else                                          \
    debug("skipped initing plugin for \"%s\", asset manager did not declare such type", TYPE_NAME);

  INIT_SERVICE("prefab", ::init_plugin_prefabs());
  INIT_SERVICE("tex", ::init_plugin_textures());
  INIT_SERVICE("mat", ::init_plugin_materials());
  ::init_plugin_entities();
  INIT_SERVICE("grass", ::init_plugin_grass());
  INIT_SERVICE("spline", ::init_plugin_splines());
  INIT_SERVICE("tifMask", ::init_plugin_tif_mask());
  INIT_SERVICE("land", ::init_plugin_land_class());
  INIT_SERVICE("skeleton", ::init_plugin_nodes());
  INIT_SERVICE("collision", ::init_plugin_collision());
  INIT_SERVICE("physObj", ::init_plugin_physobj());
  INIT_SERVICE("fx", ::init_plugin_effects());
  INIT_SERVICE("vehicle", ::init_plugin_vehicle());
  INIT_SERVICE("fastPhys", ::init_plugin_fastphys());
  INIT_SERVICE("rndGrass", ::init_plugin_rndgrass());
  INIT_SERVICE("csg", ::init_plugin_csg_entities());
  INIT_SERVICE("animTree", ::init_plugin_anim_tree());
#if HAS_PLUGIN_WT_UNIT
  INIT_SERVICE("wtUnit", ::init_plugin_wt_unit());
#endif
  INIT_SERVICE("a2d", ::init_plugin_a2d());
  INIT_SERVICE("ecs_template", ::init_plugin_ecs_templates());
  //::init_plugin_custom();
#undef INIT_SERVICE
}


void IGenEditorPlugin::drawInfo(IGenViewportWnd *wnd) { get_app().drawAssetInformation(wnd); }


void IGenEditorPlugin::repaintView() { get_app().repaint(); }


PropPanel::ContainerPropertyControl *IGenEditorPlugin::getPluginPanel()
{
  PropPanel::ContainerPropertyControl *mainPanel = get_app().getPropPanel();

  if (!mainPanel || !mainPanel->getById(ID_SPEC_GRP))
    return NULL;

  return mainPanel->getById(ID_SPEC_GRP)->getContainer();
}


PropPanel::ContainerPropertyControl *IGenEditorPlugin::getPropPanel() { return get_app().getPropPanel(); }


void IGenEditorPlugin::fillPluginPanel() { get_app().fillPropPanel(); }


CoolConsole &IGenEditorPlugin::getMainConsole() { return get_app().getConsole(); }


IWndManager &IGenEditorPlugin::getWndManager() { return *get_app().getWndManager(); }


void IGenEditorPlugin::onPropPanelClear(PropPanel::ContainerPropertyControl &)
{
  if (spEditor)
    spEditor->destroyPanel();
}


bool IGenEditorPlugin::hasScriptPanel() { return spEditor != NULL; }


void IGenEditorPlugin::fillScriptPanel(PropPanel::ContainerPropertyControl &propPanel)
{
  if (spEditor)
    spEditor->createPanel(propPanel);
}


void IGenEditorPlugin::initScriptPanelEditor(const char *scheme, const char *panel_caption)
{
  del_it(spEditor);
  spEditor = new AVScriptPanelEditor(scheme, panel_caption);
  if (spEditor && !spEditor->scriptExists())
    del_it(spEditor);
}
