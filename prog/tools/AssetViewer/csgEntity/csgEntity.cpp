// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_randomSeed.h>
#include <de3_lodController.h>
#include <de3_csgEntity.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>

class CsgEntityViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  enum
  {
    PID_LOD_AUTO_CHOOSE = 1,
    PID_LOD_FIRST,

    PID_TYPE_START,
    PID_TYPE_END = PID_TYPE_START + 40,

    PID_LODS_GROUP = 100,
    PID_GENERATE_SEED,
    PID_SEED,
    PID_SHOW_OCCL_BOX,
    PID_ROTATE_Y,
    PID_POLYGON_SIZE,
    PID_POLYGON_TYPE_GROUP,
  };

  CsgEntityViewPlugin() : entity(NULL)
  {
    occluderBox.setempty();
    showOcclBox = true;
    rotY = 0;
    pathType = PID_TYPE_START;
    pathScale = 1.0;

    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    viewShapes = *appBlk.getBlockByNameEx("csgEntityView");
    if (!viewShapes.blockCount())
    {
      DataBlock *b = viewShapes.addBlock("rectangle");
      b->addPoint2("p", Point2(-20, -12));
      b->addPoint2("p", Point2(-20, 12));
      b->addPoint2("p", Point2(20, 12));
      b->addPoint2("p", Point2(20, -12));
    }
  }
  ~CsgEntityViewPlugin() { destroy_it(entity); }

  virtual const char *getInternalName() const { return "csgEntityViewer"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    occluderBox.setempty();
    occluderQuad.clear();
    entity = DAEDITOR3.createEntity(*asset);
    if (entity)
    {
      entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
      entity->setTm(TMatrix::IDENT);

      ILodController *iLodCtrl = entity->queryInterface<ILodController>();
      if (iLodCtrl)
        iLodCtrl->setCurLod(-1);
      setFoundationPath();
    }
    fillPluginPanel();
    return true;
  }
  virtual bool end()
  {
    if (spEditor)
      spEditor->destroyPanel();
    destroy_it(entity);
    return true;
  }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    if (!entity)
      return false;
    box = entity->getBbox();
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects()
  {
    if (showOcclBox && (!occluderBox.isempty() || occluderQuad.size()))
    {
      TMatrix tm;
      tm.rotyTM(rotY * DEG_TO_RAD);
      begin_draw_cached_debug_lines(false, false);
      set_cached_debug_lines_wtm(tm);
      if (occluderQuad.size())
      {
        draw_cached_debug_line(occluderQuad.data(), 4, 0x80808000);
        draw_cached_debug_line(occluderQuad[3], occluderQuad[0], 0x80808000);
      }
      else
        draw_cached_debug_box(occluderBox, 0x80808080);
      end_draw_cached_debug_lines();

      begin_draw_cached_debug_lines();
      set_cached_debug_lines_wtm(tm);
      if (occluderQuad.size())
      {
        draw_cached_debug_line(occluderQuad.data(), 4, 0xFFFFFF00);
        draw_cached_debug_line(occluderQuad[3], occluderQuad[0], 0xFFFFFF00);
      }
      else
        draw_cached_debug_box(occluderBox, 0xFFFFFFFF);
      end_draw_cached_debug_lines();
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "csg") == 0; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel)
  {
    panel.setEventHandler(this);

    panel.createTrackFloat(PID_ROTATE_Y, "Rotate around Y, deg", rotY, -180, 180, 1);
    panel.createIndent();

    if (!entity)
      return;

    ILodController *iLodCtrl = entity->queryInterface<ILodController>();
    if (iLodCtrl && iLodCtrl->getLodCount())
    {
      const int lodsCount = iLodCtrl->getLodCount();
      const bool moreThanOne = lodsCount > 1;
      PropPanel::ContainerPropertyControl *rg = panel.createRadioGroup(PID_LODS_GROUP, "Presentation lods:");

      if (moreThanOne)
        rg->createRadio(PID_LOD_AUTO_CHOOSE, "auto choose");

      String buf;
      for (int i = 0; i < lodsCount; i++)
      {
        buf.printf(64, "lod_%d [%.2f m]", i, iLodCtrl->getLodRange(i));
        rg->createRadio(PID_LOD_FIRST + i, buf);
      }

      panel.setInt(PID_LODS_GROUP, (moreThanOne ? PID_LOD_AUTO_CHOOSE : PID_LOD_FIRST));
      panel.createIndent();
    }

    if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
    {
      panel.createButton(PID_GENERATE_SEED, "Generate random seed");
      panel.createTrackInt(PID_SEED, "Random seed", irsh->getSeed(), 0, 32767, 1);
      panel.createIndent();
    }

    panel.createEditFloat(PID_POLYGON_SIZE, "polygon size", pathScale * 40);
    PropPanel::ContainerPropertyControl *rg = panel.createRadioGroup(PID_POLYGON_TYPE_GROUP, "polygon type:");
    for (int i = 0; i < viewShapes.blockCount(); i++)
      if (PID_TYPE_START + i <= PID_TYPE_END)
        rg->createRadio(PID_TYPE_START + i, viewShapes.getBlock(i)->getBlockName());
    panel.setInt(PID_POLYGON_TYPE_GROUP, pathType);
  }


  virtual void postFillPropPanel() {}

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_LODS_GROUP)
    {
      ILodController *iLodCtrl = entity->queryInterface<ILodController>();
      if (!iLodCtrl)
        return;

      const int n = panel->getInt(PID_LODS_GROUP);
      if (PropPanel::RADIO_SELECT_NONE == n)
        return;

      iLodCtrl->setCurLod(n <= PID_LOD_AUTO_CHOOSE ? -1 : n - PID_LOD_FIRST);
    }
    else if (pcb_id == PID_SEED)
    {
      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
        irsh->setSeed(panel->getInt(pcb_id));
    }
    else if (pcb_id == PID_ROTATE_Y)
    {
      rotY = panel->getFloat(pcb_id);
      setFoundationPath();
    }
    else if (pcb_id == PID_SHOW_OCCL_BOX)
      showOcclBox = panel->getBool(pcb_id);
    else if (pcb_id == PID_POLYGON_SIZE)
    {
      pathScale = panel->getFloat(PID_POLYGON_SIZE) / 40;
      setFoundationPath();
    }
    else if (pcb_id == PID_POLYGON_TYPE_GROUP)
    {
      pathType = panel->getInt(pcb_id);
      setFoundationPath();
    }
  }
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_GENERATE_SEED && entity)
      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
      {
        panel->setInt(PID_SEED, grnd() & 0x7FFF);
        irsh->setSeed(panel->getInt(PID_SEED));
      }
  }

  void setFoundationPath()
  {
    if (ICsgEntity *c = entity ? entity->queryInterface<ICsgEntity>() : NULL)
    {
      Tab<Point3> p;
      int nid = viewShapes.getNameId("p");
      const DataBlock &b = *viewShapes.getBlock(pathType - PID_TYPE_START);
      p.reserve(b.paramCount());
      for (int i = 0; i < b.paramCount(); i++)
        if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_POINT2)
          p.push_back().set_x0y(b.getPoint2(i));
        else if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_POINT3)
          p.push_back(b.getPoint3(i));

      TMatrix tm = rotyTM(rotY * DEG_TO_RAD);
      float s = pathScale * b.getReal("scale", 1.0f);
      for (int i = 0; i < p.size(); i++)
        p[i] = tm * (p[i] * s);
      c->setFoundationPath(make_span(p), true);
    }
  }

private:
  IObjEntity *entity;
  BBox3 occluderBox;
  Tab<Point3> occluderQuad;
  bool showOcclBox;
  float rotY;
  int pathType;
  float pathScale;
  DataBlock viewShapes;
};

static InitOnDemand<CsgEntityViewPlugin> plugin;


void init_plugin_csg_entities()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
