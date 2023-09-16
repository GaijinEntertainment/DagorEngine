#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <de3_genObjAlongSpline.h>
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_rendInstGen.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_hierBitMap2d.h>
#include <math/dag_bezierPrec.h>
#include <math/dag_e3dColor.h>
#include <image/dag_texPixel.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_drv3d.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <propPanel2/c_panel_base.h>
#include <de3_assetService.h>
#include <de3_rasterizePoly.h>
#include <de3_genObjByDensGridMask.h>
#include <de3_genObjInsidePoly.h>
#include <de3_bitMaskMgr.h>
#include <de3_grassPlanting.h>
#include <de3_genGrassByDensGridMask.h>
#include <debug/dag_debug3d.h>
#include <landMesh/lmeshRenderer.h>
#include <3d/dag_texPackMgr2.h>
// #include <debug/dag_debug.h>


const float radius = 100.f;
const float yOffset = 0.02;


enum
{
  PID_POLYGON_SIZE = 5,
  PID_POLYGON_TYPE_GROUP,
};

enum FigType
{
  FIG_TYPE__FIRST = 1,

  FIG_TYPE_SQUARE = FIG_TYPE__FIRST,
  FIG_TYPE_TRIANGLE,
  FIG_TYPE_RHOMBUS,

  FIG_TYPE__LAST,
};

struct Points
{
public:
  Points(float x = 0.f, float y = 0.f, float z = 0.f) : pt(x, y, z) {}

  inline Point3 getPt() const { return pt; }
  inline bool setPos(const Point3 &new_pt)
  {
    pt = new_pt;
    return true;
  }
  inline void setPos(float x, float y, float z)
  {
    pt.x = x;
    pt.y = y;
    pt.z = z;
  }

protected:
  Point3 pt;
};


class LandClassViewPlugin : public IGenEditorPlugin, public ControlEventHandler
{
public:
  LandClassViewPlugin() :
    entPoolPlanted(midmem),
    entPoolTiled(midmem),
    entPoolSpline(midmem),
    points(midmem),
    spline(NULL),
    texId(BAD_TEXTUREID),
    tile(0.f),
    assetName(""),
    rbuf("editor_opaque_geom_helper"),
    presentationType(FIG_TYPE__FIRST)
  {
    bbox.setempty();
    initScriptPanelEditor("land.scheme.nut", "landClass' by scheme");
  }

  ~LandClassViewPlugin() { end(); }

  virtual const char *getInternalName() const { return "landClassViewer"; }

  virtual void registered() {}

  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    end();

    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    assetName = asset->getName();
    const landclass::AssetData *landClassData = !assetName.empty() ? assetSrv->getLandClassData(assetName) : NULL;
    const char *splineAssetName = landClassData->splineClassAssetName;
    const splineclass::AssetData *splineClassData =
      splineAssetName && splineAssetName[0] ? assetSrv->getSplineClassData(splineAssetName) : NULL;

    if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
      rigenSrv->discardRIGenRect(0, 0, 64, 64);

    if (!landClassData && !splineClassData)
      return false;

    tile = (landClassData) ? generateImage(landClassData) / 2.f : 0.f;
    if (tile < 0.f)
      tile = 256.f;

    setPoints(radius);

    generateObjectByLandClass(landClassData);
    generateObjectBySpline(splineClassData);

    optimizeBbox(radius);

    if (spEditor && asset)
      spEditor->load(asset);

    return true;
  }

  virtual bool end()
  {
    if (spEditor)
      spEditor->destroyPanel();
    del_it(spline);

    clearRenderBuf();
    clearGrass();
    clearEntitys();
    points.clear();

    tile = 0.f;

    assetName = "";

    return true;
  }

  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    box = bbox;
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects()
  {
    if (!spline)
      return;

    ::begin_draw_cached_debug_lines();

    Point3 lp, np;
    lp = spline->get_pt(0.f);

    for (float i = 0.f; i < spline->leng; i += 2.f)
    {
      np = spline->get_pt(i);
      ::draw_cached_debug_line(lp, np, E3DCOLOR(255, 255, 0));
      lp = np;
    }
    ::draw_cached_debug_line(lp, spline->get_pt(spline->leng), E3DCOLOR(255, 255, 0));

    if (spline->isClosed())
      ::draw_cached_debug_line(lp, spline->get_pt(0), E3DCOLOR(255, 255, 0));

    ::end_draw_cached_debug_lines();
  }
  virtual void renderGeometry(Stage stage)
  {
    if (!getVisible())
      return;

    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE:
        if ((texId == BAD_TEXTUREID) || (tile == 0.f))
          return;

        rbuf.drawQuad(Point3(-tile, yOffset, -tile), Point3(-tile, yOffset, +tile), Point3(+tile, yOffset, +tile),
          Point3(+tile, yOffset, -tile), E3DCOLOR(255, 255, 255, 0), 1, -1);

        rbuf.addFaces(texId);
        rbuf.flush();
        rbuf.clearBuf();
        break;
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "land") == 0; }

  virtual void fillPropPanel(PropertyContainerControlBase &propPanel)
  {
    propPanel.setEventHandler(this);

    propPanel.createEditFloat(PID_POLYGON_SIZE, "polygon square size", radius);

    PropertyContainerControlBase *rg = propPanel.createRadioGroup(PID_POLYGON_TYPE_GROUP, "presentation type:");

    rg->createRadio(FIG_TYPE_SQUARE, "square");
    rg->createRadio(FIG_TYPE_TRIANGLE, "triangle");
    rg->createRadio(FIG_TYPE_RHOMBUS, "rhombus");

    propPanel.setInt(PID_POLYGON_TYPE_GROUP, presentationType);
  }

  virtual void postFillPropPanel() {}

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel)
  {
    if (!spline || assetName.empty())
      return;

    if (pcb_id == PID_POLYGON_SIZE)
      ; // no-op
    else if (pcb_id == PID_POLYGON_TYPE_GROUP)
    {
      const FigType type = (FigType)panel->getInt(pcb_id);
      if ((type == presentationType) || (type == (FigType)RADIO_SELECT_NONE))
        return;

      presentationType = type;
    }
    else
      return;

    const float r = panel->getFloat(PID_POLYGON_SIZE);

    del_it(spline);

    clearEntitys();

    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    const landclass::AssetData *landClassData = assetSrv->getLandClassData(assetName);
    const char *splineAssetName = landClassData->splineClassAssetName;
    const splineclass::AssetData *splineClassData =
      splineAssetName && splineAssetName[0] ? assetSrv->getSplineClassData(splineAssetName) : NULL;

    if (!landClassData && !splineClassData)
      return;

    setPoints(r);

    generateObjectByLandClass(landClassData);
    generateObjectBySpline(splineClassData);

    optimizeBbox(r);

    /*if (EDITORCORE->getCurrentViewport())
      EDITORCORE->getCurrentViewport()->zoomAndCenter(bbox);*/
  }

protected:
  void generateObjectByLandClass(const landclass::AssetData *land_class_data)
  {
    // clearEntitys();
    bbox.setempty();
    resetUsedPoolsEntities(make_span(entPoolTiled));

    static const int tiledByPolygonSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");

    Tab<Points *> pt(tmpmem);
    const int pCnt = points.size();
    pt.resize(pCnt);
    for (int i = pCnt - 1; i >= 0; i--)
      pt[i] = &points[i];

    if (land_class_data->tiled)
    {
      entPoolTiled.resize(land_class_data->tiled->data.size());

      objgenerator::generateTiledEntitiesInsidePoly(*land_class_data->tiled, tiledByPolygonSubTypeId, 0, NULL, make_span(entPoolTiled),
        make_span_const(pt), 0, Point2(tile, tile));
      bbox = calculateBbox(make_span_const(entPoolTiled));
    }

    deleteUnusedPoolsEntities(make_span(entPoolTiled));

    resetUsedPoolsEntities(make_span(entPoolPlanted));


    landclass::GrassEntities *grassEnt = land_class_data->grass;
    landclass::PlantedEntities *plantedEnt = land_class_data->planted;

    typedef HierBitMap2d<ConstSizeBitMap2d<5>> HierBitmap32;
    HierBitmap32 bmp;

    float ofsX = 0.f, ofsZ = 0.f;
    const bool needRasterize = (grassEnt || plantedEnt);
    const float cell = needRasterize ? rasterize_poly(bmp, ofsX, ofsZ, make_span_const(pt)) : 0;

    if (plantedEnt)
    {
      entPoolPlanted.resize(plantedEnt->ent.size());

      objgenerator::generatePlantedEntitiesInMaskedRect(*plantedEnt, tiledByPolygonSubTypeId, 0, NULL, make_span(entPoolPlanted), bmp,
        1.0 / cell, tile + ofsX, tile + ofsZ, bmp.getW() * cell, bmp.getH() * cell, -tile, -tile, yOffset);
      bbox += calculateBbox(make_span_const(entPoolPlanted));
    }

    deleteUnusedPoolsEntities(make_span(entPoolPlanted));

    if (grassEnt)
    {
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

  void generateObjectBySpline(const splineclass::AssetData *spline_class_data)
  {
    static const int splineSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");

    const int rseed = 12345;

    if (points.size() > 1)
    {
      del_it(spline);
      spline = new BezierSplinePrec3d();

      SmallTab<Point3, TmpmemAlloc> pt;
      const int pCnt = points.size();
      clear_and_resize(pt, 3 * pCnt);

      for (int pi = pt.size() - 1, i = pCnt - 1; i >= 0; i--)
      {
        const Point3 p = points[i].getPt();
        pt[pi--] = p;
        pt[pi--] = p;
        pt[pi--] = p;
      }

      G_VERIFY(spline->calculate(pt.data(), pt.size(), true));

      Points *p0 = &points[0];

      resetUsedPoolsEntities(make_span(entPoolSpline));
      objgenerator::generateBySpline(*spline, NULL, 0, pCnt - 1, spline_class_data, entPoolSpline, nullptr, true, splineSubTypeId, 0,
        rseed, 0);
      deleteUnusedPoolsEntities(make_span(entPoolSpline));

      bbox += calculateBbox(make_span_const(entPoolSpline));
    }
  }

  inline void clearGrass()
  {
    for (int i = grass.size() - 1; i >= 0; i--)
      destroy_it(grass[i]);
    clear_and_shrink(grass);
  }

  template <class T>
  inline BBox3 calculateBbox(dag::ConstSpan<T> ent_pool)
  {
    BBox3 bbox;
    for (int i = ent_pool.size() - 1; i >= 0; i--)
    {
      const T &spool = ent_pool[i];
      for (int j = spool.entPool.size() - 1; j >= 0; j--)
      {
        if (spool.entPool[j])
        {
          TMatrix tm;
          spool.entPool[j]->getTm(tm);
          bbox += tm * spool.entPool[j]->getBbox();
        }
      }
    }

    return bbox;
  }

  template <class T>
  static inline void resetUsedPoolsEntities(dag::Span<T> ent_pool)
  {
    const int cnt = ent_pool.size();
    for (int i = 0; i < cnt; i++)
      ent_pool[i].resetUsedEntities();
  }

  template <class T>
  static inline void deleteUnusedPoolsEntities(dag::Span<T> ent_pool)
  {
    const int cnt = ent_pool.size();
    for (int i = 0; i < cnt; i++)
      ent_pool[i].deleteUnusedEntities();
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
          ddsx::tex_pack2_perform_delayed_data_loading();
        }
      }
      else
        texId = BAD_TEXTUREID;

      return detTex->getPoint2("size", Point2(10, 10)).x;
    }

    return 0.f;
  }

  template <class T>
  static inline void clearPool(dag::Span<T> ent_pool)
  {
    for (int i = ent_pool.size() - 1; i >= 0; i--)
      ent_pool[i].clear();
  }

  inline void clearEntitys()
  {
    bbox.setempty();
    clearPool(make_span(entPoolPlanted));
    clearPool(make_span(entPoolTiled));
    clearPool(make_span(entPoolSpline));
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
      const float rdq = radius / 4;
      bbox += Point3(+rdq, 0, +rdq);
      bbox += Point3(-rdq, rdq, -rdq);
    }
  }

  void setPoints(float r)
  {
    if (presentationType == FIG_TYPE_SQUARE)
    {
      if (points.size() != 4)
        points.resize(4);

      points[0].setPos(-r, yOffset, +r);
      points[1].setPos(-r, yOffset, -r);
      points[2].setPos(+r, yOffset, -r);
      points[3].setPos(+r, yOffset, +r);
    }
    else if (presentationType == FIG_TYPE_TRIANGLE)
    {
      if (points.size() != 3)
        points.resize(3);

      points[0].setPos(+0, yOffset, +r);
      points[1].setPos(-r, yOffset, -r);
      points[2].setPos(+r, yOffset, -r);
    }
    else // if (presentationType == FIG_TYPE_RHOMBUS)
    {
      if (points.size() != 4)
        points.resize(4);

      points[0].setPos(+0, yOffset, +r);
      points[1].setPos(+r, yOffset, +0);
      points[2].setPos(-0, yOffset, -r);
      points[3].setPos(-r, yOffset, -0);
    }
  }

private:
  BBox3 bbox;
  Tab<Points> points;
  Tab<landclass::SingleEntityPool> entPoolPlanted, entPoolTiled;
  Tab<splineclass::SingleEntityPool> entPoolSpline;
  SmallTab<IObjEntity *, MidmemAlloc> grass;
  BezierSplinePrec3d *spline;

  TEXTUREID texId;
  DynRenderBuffer rbuf;
  float tile;
  String assetName;
  FigType presentationType;
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
