// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_rendInstGen.h>
#include <de3_splineGenSrv.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_hierBitMap2d.h>
#include <math/dag_e3dColor.h>
#include <image/dag_texPixel.h>
#include <3d/dag_texMgr.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <propPanel/control/container.h>
#include <de3_assetService.h>
#include <de3_rasterizePoly.h>
#include <de3_genObjByDensGridMask.h>
#include <de3_grassPlanting.h>
#include <de3_genGrassByDensGridMask.h>
#include <debug/dag_debug3d.h>
#include <3d/dag_texPackMgr2.h>
// #include <debug/dag_debug.h>


static float edgeSize = 100.f;
static const float yOffset = 0.02;


enum
{
  PID_POLYGON_SIZE = 5,
  PID_POLYGON_TYPE_GROUP,
  PID_POLYGON_GEOMTAGS_GROUP,
  PID_POLYGON_GEOMTAGS_0,
  PID_POLYGON_GEOMTAGS_LAST = PID_POLYGON_GEOMTAGS_0 + 256,
};

enum FigType
{
  FIG_TYPE__FIRST = 1,

  FIG_TYPE_SQUARE = FIG_TYPE__FIRST,
  FIG_TYPE_TRIANGLE,
  FIG_TYPE_RHOMBUS,

  FIG_TYPE__LAST,
};


class LandClassViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  LandClassViewPlugin() :
    points(midmem),
    texId(BAD_TEXTUREID),
    tile(0.f),
    assetName(""),
    rbuf("editor_opaque_geom_helper"),
    presentationType(FIG_TYPE__FIRST)
  {
    bbox.setempty();
    initScriptPanelEditor("land.scheme.nut", "landClass' by scheme");
    splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  }

  ~LandClassViewPlugin() override { end(); }

  const char *getInternalName() const override { return "landClassViewer"; }

  void registered() override {}

  void unregistered() override {}

  bool begin(DagorAsset *asset) override
  {
    end();

    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    assetName = asset->getName();
    landClassData = !assetName.empty() ? assetSrv->getLandClassData(assetName) : NULL;

    tile = (landClassData) ? generateImage(landClassData) / 2.f : 0.f;
    if (tile < 0.f)
      tile = 256.f;

    setPoints(tile > 0 ? tile * 2.0f : edgeSize);
    recreateGenObj();

    optimizeBbox(edgeSize);

    if (spEditor && asset)
      spEditor->load(asset);

    return true;
  }

  bool end() override
  {
    if (spEditor)
      spEditor->destroyPanel();
    destroy_it(genObj);
    del_it(taggedGeom);
    geomTags.clear();
    showReqTagId = -1;

    clearRenderBuf();
    clearGrass();
    points.clear();
    tile = 0.f;
    assetName = "";
    landClassData = nullptr;

    return true;
  }

  IGenEventHandler *getEventHandler() override { return NULL; }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
  {
    box = bbox;
    return true;
  }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override
  {
    if (points.size() < 3)
      return;
    ::begin_draw_cached_debug_lines();

    Point3 lp, np;
    lp = points.back();

    for (int i = 0; i < points.size(); i++)
    {
      np = points[i];
      ::draw_cached_debug_line(lp, np, E3DCOLOR(255, 255, 0));
      lp = np;
    }

    ::end_draw_cached_debug_lines();
  }
  void renderGeometry(Stage stage) override
  {
    if (!getVisible())
      return;

    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE:
        if (taggedGeom)
          taggedGeom->render();
        if ((texId == BAD_TEXTUREID) || (tile == 0.f) || taggedGeom)
          return;

        rbuf.drawQuad(Point3(-tile, yOffset, -tile), Point3(-tile, yOffset, +tile), Point3(+tile, yOffset, +tile),
          Point3(+tile, yOffset, -tile), E3DCOLOR(255, 255, 255, 0), 1, -1);

        rbuf.addFaces(texId);
        rbuf.flush();
        rbuf.clearBuf();
        break;

      case STG_RENDER_STATIC_TRANS:
        if (taggedGeom)
          taggedGeom->renderTrans();
        break;

      default: break;
    }
  }

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "land") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &propPanel) override
  {
    propPanel.setEventHandler(this);

    propPanel.createEditFloat(PID_POLYGON_SIZE, "polygon edge size", edgeSize);
    propPanel.setMinMaxStep(PID_POLYGON_SIZE, 10.f, 4096.f, 5.f);

    PropPanel::ContainerPropertyControl *rg = propPanel.createRadioGroup(PID_POLYGON_TYPE_GROUP, "presentation type:");

    rg->createRadio(FIG_TYPE_SQUARE, "square");
    rg->createRadio(FIG_TYPE_TRIANGLE, "triangle");
    rg->createRadio(FIG_TYPE_RHOMBUS, "rhombus");

    propPanel.setInt(PID_POLYGON_TYPE_GROUP, presentationType);

    if (genObj)
    {
      propPanel.createSeparator();
      if (geomTags.nameCount())
      {
        rg = propPanel.createRadioGroup(PID_POLYGON_GEOMTAGS_GROUP, "Loft and polygon tags:");
        iterate_names_in_lexical_order(geomTags, [&](int id, const char *nm) { rg->createRadio(PID_POLYGON_GEOMTAGS_0 + id, nm); });
        rg->createRadio(PID_POLYGON_GEOMTAGS_LAST, "--- don't show tagged geometry ---");
        propPanel.setInt(PID_POLYGON_GEOMTAGS_GROUP, PID_POLYGON_GEOMTAGS_LAST);
      }
      else
        propPanel.createStatic(-1, "No loft/polygon tags for generated geometry");
    }
  }

  void postFillPropPanel() override {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (assetName.empty())
      return;

    if (pcb_id == PID_POLYGON_SIZE)
      ; // no-op
    else if (pcb_id == PID_POLYGON_TYPE_GROUP)
    {
      const FigType type = (FigType)panel->getInt(pcb_id);
      if ((type == presentationType) || (type == (FigType)PropPanel::RADIO_SELECT_NONE))
        return;

      presentationType = type;
    }
    else if (pcb_id == PID_POLYGON_GEOMTAGS_GROUP && genObj && splSrv)
    {
      int tag = panel->getInt(pcb_id) == PID_POLYGON_GEOMTAGS_LAST ? -1 : panel->getInt(pcb_id) - PID_POLYGON_GEOMTAGS_0;
      if (showReqTagId != tag)
      {
        showReqTagId = tag;
        prepareTaggedGeom();
      }
      return;
    }
    else
      return;

    const float r = panel->getFloat(PID_POLYGON_SIZE);

    setPoints(r);
    recreateGenObj();

    optimizeBbox(r);
  }

protected:
  static const DataBlock &generatePolyGenEntityBlk(DataBlock &polyBlk, const char *asset_name, dag::ConstSpan<Point3> points,
    const Point2 &objOfs)
  {
    DataBlock *b = polyBlk.addBlock("polygon");
    b->setStr("name", asset_name);
    b->setStr("blkGenName", asset_name);
    b->setPoint2("polyObjOffs", objOfs);
    b->setBool("tiledObjsWorldAnchor", true);
    b->setBool("plantedObjsWorldAnchor", true);
    for (auto &p : points)
      b->addNewBlock("point")->setPoint3("pt", p);
    return *b;
  }
  void recreateGenObj()
  {
    destroy_it(genObj);
    if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
      rigenSrv->discardRIGenRect(0, 0, 64, 64);

    ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
    DataBlock polyBlk;
    if (IObjEntity *e = splSrv->createVirtualSplineEntity(generatePolyGenEntityBlk(polyBlk, assetName, points, Point2(tile, tile))))
    {
      if (ISplineEntity *se = e->queryInterface<ISplineEntity>())
        genObj = se->createSplineEntityInstance();
      destroy_it(e);
    }
    if (genObj)
    {
      genObj->setTm(TMatrix::IDENT);
      if (ISplineEntity *se = genObj->queryInterface<ISplineEntity>())
        bbox = se->calcWABB();
      else
        bbox = genObj->getBbox();
      geomTags.clear();
      if (splSrv)
        splSrv->gatherGeneratedGeomTags(geomTags);
      if (showReqTagId > geomTags.nameCount())
        showReqTagId = -1;
      prepareTaggedGeom();
    }
    else
      bbox.setempty();

    if (landClassData)
      generateGrassByLandClass(landClassData->grass);
  }
  void prepareTaggedGeom()
  {
    del_it(taggedGeom);
    if (showReqTagId < 0)
      return;

    ISplineGenService::LayerIndexList loft_layers(0);
    splSrv->gatherLoftLayers(loft_layers, false);

    GeomObject all_geom;
    loft_layers.iterate_layers([&](unsigned ll) {
      splSrv->gatherStaticGeometry(*all_geom.getGeometryContainer(), StaticGeometryNode::FLG_RENDERABLE, false, ll, 2, -1);
    });

    taggedGeom = new GeomObject;
    for (auto *&node : all_geom.getGeometryContainer()->nodes)
      if (node && showReqTagId == geomTags.getNameId(node->script.getStr("layerTag", "")))
      {
        taggedGeom->getGeometryContainer()->nodes.push_back(node);
        node = nullptr;
      }
    if (taggedGeom->getGeometryContainer()->nodes.size())
    {
      taggedGeom->setTm(TMatrix::IDENT);
      taggedGeom->notChangeVertexColors(true);
      taggedGeom->recompile();
      taggedGeom->notChangeVertexColors(false);
    }
    else
      del_it(taggedGeom);
  }
  void generateGrassByLandClass(landclass::GrassEntities *grassEnt)
  {
    if (grassEnt)
    {
      typedef HierBitMap2d<ConstSizeBitMap2d<5>> HierBitmap32;
      HierBitmap32 bmp;

      float ofsX = 0.f, ofsZ = 0.f;
      const float cell = rasterize_poly(bmp, ofsX, ofsZ, points);

      if (!grass.size())
      {
        clear_and_resize(grass, grassEnt->data.size());
        for (int i = 0; i < grass.size(); i++)
          grass[i] = IDaEditor3Engine::get().cloneEntity(grassEnt->data[i].entity);
      }

      const int grassCnt = grass.size();
      for (int i = 0; i < grassCnt; i++)
      {
        if (!grass[i])
          continue;

        IGrassPlanting *gp = grass[i]->queryInterface<IGrassPlanting>();
        if (!gp)
          continue;

        landclass::GrassDensity &grassDens = grassEnt->data[i];

        const Point2 &densMapSize = grassDens.densMapSize;
        int wx, wz, ofx, ofz;

        if (grassDens.densityMap && ((densMapSize.x > 0) || (densMapSize.y > 0)))
        {
          const Point2 halfDensMap = densMapSize / 2.f;
          wx = halfDensMap.x + ofsX;
          wz = halfDensMap.y + ofsZ;
          ofx = -halfDensMap.x;
          ofz = -halfDensMap.y;
        }
        else
        {
          wx = wz = 0;
          ofx = ofsX;
          ofz = ofsZ;
        }

        const float w = bmp.getW() * cell;
        const float h = bmp.getH() * cell;

        gp->startGrassPlanting(0, 0, 0.71 * (w > h ? w : h));

        objgenerator::generateGrassInMaskedRect(grassDens, gp, bmp, 1.0 / cell, wx, wz, w, h, ofx, ofz);

        if (gp->finishGrassPlanting() == 0)
          clearGrass();
      }
    }
  }

  inline void clearGrass()
  {
    for (int i = grass.size() - 1; i >= 0; i--)
      destroy_it(grass[i]);
    clear_and_shrink(grass);
  }

  inline float generateImage(const landclass::AssetData *landClassData)
  {
    const DataBlock *detTex = landClassData->detTex;

    if (detTex)
    {
      const char *colormap = detTex->getStr("texture", "");
      if (*colormap)
      {
        const char *fname = DAEDITOR3.resolveTexAsset(colormap);
        DAEDITOR3.conNote("'%s' : texFile='%s'", colormap, fname);
        const bool validFname = (fname && fname[0]);
        texId = validFname ? get_managed_texture_id(fname) : BAD_TEXTUREID;
        if (validFname && (texId == BAD_TEXTUREID))
          DAEDITOR3.conError("can't load texFile='%s'", fname);
        if (texId != BAD_TEXTUREID)
        {
          if (!acquire_managed_tex(texId))
            texId = BAD_TEXTUREID;
          if (!is_managed_textures_streaming_load_on_demand())
            ddsx::tex_pack2_perform_delayed_data_loading();
        }
      }
      else
        texId = BAD_TEXTUREID;

      return detTex->getPoint2("size", Point2(10, 10)).x;
    }

    return 0.f;
  }

  inline void clearRenderBuf()
  {
    if (texId != BAD_TEXTUREID)
    {
      release_managed_tex(texId);
      texId = BAD_TEXTUREID;
      ShaderGlobal::reset_textures(true);
    }

    rbuf.addFaces(BAD_TEXTUREID);
    rbuf.clearBuf();
  }

  void optimizeBbox(float r)
  {
    const Point3 width = bbox.width();
    if ((width.x > 200.f) || (width.y > 200.f) || (width.z > 200.f) || (width.x < 0.1) || (width.y < 0.1) || (width.z < 0.1))
    {
      bbox.setempty();
      const float rdq = r / 4;
      bbox += Point3(+rdq, 0, +rdq);
      bbox += Point3(-rdq, rdq, -rdq);
    }
  }

  void setPoints(float sz)
  {
    if (presentationType == FIG_TYPE_SQUARE)
    {
      if (points.size() != 4)
        points.resize(4);

      float r = sz * 0.5;
      points[0].set(-r, yOffset, +r);
      points[1].set(-r, yOffset, -r);
      points[2].set(+r, yOffset, -r);
      points[3].set(+r, yOffset, +r);
    }
    else if (presentationType == FIG_TYPE_TRIANGLE)
    {
      if (points.size() != 3)
        points.resize(3);

      float h = sz * sqrt(3.0f) / 2.0f / 3.0f;
      points[0].set(+h * 2.f, yOffset, 0);
      points[1].set(-h, yOffset, -sz * 0.5f);
      points[2].set(-h, yOffset, +sz * 0.5f);
    }
    else // if (presentationType == FIG_TYPE_RHOMBUS)
    {
      if (points.size() != 4)
        points.resize(4);

      float r = sz / sqrt(2.0f);
      points[0].set(+0, yOffset, +r);
      points[1].set(+r, yOffset, +0);
      points[2].set(-0, yOffset, -r);
      points[3].set(-r, yOffset, -0);
    }
    edgeSize = sz;
  }

private:
  BBox3 bbox;
  Tab<Point3> points;
  const landclass::AssetData *landClassData = nullptr;
  IObjEntity *genObj = nullptr;
  GeomObject *taggedGeom = nullptr;
  OAHashNameMap<true> geomTags;
  int showReqTagId = -1;
  SmallTab<IObjEntity *, MidmemAlloc> grass;

  TEXTUREID texId;
  DynRenderBuffer rbuf;
  float tile;
  String assetName;
  FigType presentationType;
  ISplineGenService *splSrv = nullptr;
};

static InitOnDemand<LandClassViewPlugin> plugin;


void init_plugin_land_class()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
