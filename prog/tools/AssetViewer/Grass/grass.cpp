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
  ~GrassViewPlugin() override { destroy_it(grass); }

  const char *getInternalName() const override { return "grassViewer"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override
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

  bool end() override
  {
    if (spEditor)
      spEditor->destroyPanel();
    destroy_it(grass);
    return true;
  }
  IGenEventHandler *getEventHandler() override { return NULL; }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
  {
    box.lim[0] = Point3(-10, 0, -10);
    box.lim[1] = Point3(10, 4, 10);
    return true;
  }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override {}

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "grass") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override {}
  void postFillPropPanel() override {}

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
