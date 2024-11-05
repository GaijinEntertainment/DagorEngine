// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <de3_grassPlanting.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>

class GrassViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  GrassViewPlugin() : grass(NULL) { initScriptPanelEditor("grass.scheme.nut", "grass by scheme"); }
  ~GrassViewPlugin() { destroy_it(grass); }

  virtual const char *getInternalName() const { return "grassViewer"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    grass = asset ? DAEDITOR3.createEntity(*asset) : NULL;
    G_ASSERT(grass);

    if (spEditor && asset)
      spEditor->load(asset);

    if (grass)
    {
      grass->setTm(TMatrix::IDENT);

      IGrassPlanting *gp = grass->queryInterface<IGrassPlanting>();
      if (gp)
      {
        gp->startGrassPlanting(0, 0, 0.71 * float(20));

        for (int i = -10; i < 10; i++)
          for (int j = -10; j < 10; j++)
            gp->addGrassPoint(i, j);

        gp->finishGrassPlanting();
      }
    }
    return true;
  }

  virtual bool end()
  {
    if (spEditor)
      spEditor->destroyPanel();
    destroy_it(grass);
    return true;
  }
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    box.lim[0] = Point3(-10, 0, -10);
    box.lim[1] = Point3(10, 4, 10);
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "grass") == 0; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel) {}
  virtual void postFillPropPanel() {}

private:
  IObjEntity *grass;
};

static InitOnDemand<GrassViewPlugin> plugin;


void init_plugin_grass()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
