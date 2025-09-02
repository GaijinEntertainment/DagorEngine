// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>

class CustomEditorPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  CustomEditorPlugin() {}
  ~CustomEditorPlugin() override {}

  const char *getInternalName() const override { return "restAssetEditor"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override
  {
    if (!get_app().getScriptChangeFlag())
    {
      String fn(64, "%s.scheme.nut", asset->getTypeStr());
      initScriptPanelEditor(fn.str(), NULL);
      if (spEditor && asset)
        spEditor->load(asset);
    }

    return true;
  }
  bool end() override
  {
    if (spEditor)
      spEditor->destroyPanel();

    return true;
  }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override { return false; }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override {}

  bool supportAssetType(const DagorAsset &asset) const override
  {
    return true;
    // return strcmp(asset.getTypeStr(), "composit")==0 ||
    //        strcmp(asset.getTypeStr(), "dynModel")==0;
  }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override { panel.setEventHandler(this); }


  void postFillPropPanel() override {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override {}
};

static InitOnDemand<CustomEditorPlugin> plugin;


void init_plugin_custom()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
