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
  ~CustomEditorPlugin() {}

  virtual const char *getInternalName() const { return "restAssetEditor"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
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
  virtual bool end()
  {
    if (spEditor)
      spEditor->destroyPanel();

    return true;
  }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const { return false; }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual bool supportAssetType(const DagorAsset &asset) const
  {
    return true;
    // return strcmp(asset.getTypeStr(), "composit")==0 ||
    //        strcmp(asset.getTypeStr(), "dynModel")==0;
  }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel) { panel.setEventHandler(this); }


  virtual void postFillPropPanel() {}

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) {}
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
