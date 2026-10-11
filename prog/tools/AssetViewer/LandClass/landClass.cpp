// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_splineGenSrv.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_hierBitMap2d.h>
#include <math/dag_e3dColor.h>
#include <math/dag_adjpow2.h>
#include <math/dag_imageFunctions.h>
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
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_lockTexture.h>
#include <drv/3d/dag_sampler.h>
#include <landMesh/landDetile.h>
#include <3d/dag_textureIDHolder.h>
#include "../av_riGenArea.h"
#include "../av_appwnd.h"
#include <gameRes/dag_gameResources.h>
// #include <debug/dag_debug.h>


static float edgeSize = 100.f;
static const float yOffset = 0.02;
static constexpr float RIGEN_AREA_HALF = -AV_RIGEN_ORIGIN;


enum
{
  PID_POLYGON_SIZE = 5,
  PID_POLYGON_TYPE_GROUP,
  PID_POLYGON_GEOMTAGS_GROUP,
  PID_POLYGON_GEOMTAGS_0,
  PID_POLYGON_GEOMTAGS_LAST = PID_POLYGON_GEOMTAGS_0 + 256,
  PID_VIEW_CENTER,
  PID_VIEW_CENTER_ON_TILE,
  PID_FINAL_LOOK,
};

// the composed ground stays at the source texel density, up to this side
static const int MAX_FINAL_LOOK_TEX = 2048;

enum FigType
{
  FIG_TYPE__FIRST = 1,

  FIG_TYPE_SQUARE = FIG_TYPE__FIRST,
  FIG_TYPE_TRIANGLE,
  FIG_TYPE_RHOMBUS,

  FIG_TYPE__LAST,
};


class LandClassViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler, public IAssetUpdateNotify
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
    assetSrv->subscribeUpdateNotify(this, true, false);

    loadGround(*asset);
    openOnTile();
    buildFinalLook();
    recreateGenObj();

    optimizeBbox(edgeSize);

    if (spEditor)
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
    clearFinalLook();
    tile = 0.f;
    detailOffset = viewCenter = polyCenter = Point2(0, 0);
    shapeOutsideArea = false;
    assetName = "";
    landClassData = nullptr;
    if (IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>())
      assetSrv->unsubscribeUpdateNotify(this, true, false);

    return true;
  }

  // regenerate in place of the restart, keeping the view; an own-asset edit reloads in onLandClassAssetChanged
  bool reloadAsset([[maybe_unused]] DagorAsset *asset) override
  {
    destroy_it(genObj);
    clearGrass();
    free_unused_game_resources();
    recreateGenObj();
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
  void renderTransObjects(BaseTexture *) override
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

        {
          // a size still being edited would stretch the compose; the plain tiling never does
          const bool composed = composeFits(groundHalf(tile * 2.f));
          drawGround(composed);
          rbuf.addFaces(composed ? finalLookTex.getId() : texId);
        }

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
    propPanel.setMinMaxStep(PID_POLYGON_SIZE, 10.f, 2.f * RIGEN_AREA_HALF, 5.f);
    propPanel.createPoint2(PID_VIEW_CENTER, "preview center", polyCenter, 0);
    propPanel.createButton(PID_VIEW_CENTER_ON_TILE, "center on a tile");
    if (canCompose())
    {
      propPanel.createCheckBox(PID_FINAL_LOOK, String(0, "final look (de-tiled ground, %.0f m noise)", land_detile::noise_size),
        finalLook);
      propPanel.createStatic(-1, String(0, "de-tile clusters: %d", landClassData->detile.clusters));
      propPanel.createStatic(-1, String(0, "de-tile dir: %g deg ± spread=%g deg", //
                                   landClassData->detile.angle * RAD_TO_DEG, landClassData->detile.spread * RAD_TO_DEG));
      propPanel.createSeparator();
    }

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

    if (pcb_id == PID_FINAL_LOOK)
    {
      finalLook = panel->getBool(pcb_id);
      buildFinalLook();
      recreateGenObj();
      return;
    }
    else if (pcb_id == PID_POLYGON_SIZE)
      ; // no-op
    else if (pcb_id == PID_VIEW_CENTER)
      viewCenter = panel->getPoint2(pcb_id);
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
    panel->setPoint2(PID_VIEW_CENTER, polyCenter); // may be clamped
    // a drag waits for onChangeFinished
    if (pcb_id != PID_POLYGON_SIZE && pcb_id != PID_VIEW_CENTER)
    {
      buildFinalLook();
      recreateGenObj();
    }

    optimizeBbox(r);
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id != PID_VIEW_CENTER_ON_TILE || assetName.empty())
      return;
    viewCenter = tileCenter();
    setPoints(edgeSize);
    panel->setPoint2(PID_VIEW_CENTER, polyCenter);
    buildFinalLook();
    recreateGenObj();
    optimizeBbox(edgeSize);
  }

  // a drag composes and regenerates once, when it settles
  void onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_POLYGON_SIZE || pcb_id == PID_VIEW_CENTER)
    {
      buildFinalLook();
      recreateGenObj();
      optimizeBbox(edgeSize);
    }
  }

  // IAssetUpdateNotify interface
  void onLandClassAssetChanged(landclass::AssetData *data) override
  {
    if (landClassData == data)
    {
      const float oldTile = tile;
      const Point2 oldOffset = detailOffset;
      if (const DagorAsset *asset = DAEDITOR3.getAssetByName(assetName, DAEDITOR3.getAssetTypeId("land")))
        loadGround(*asset); // also drops the image: its key sees no change of the de-tile keys
      // a new tile lattice: open on its tile, as begin() does
      if (tile != oldTile || detailOffset != oldOffset)
      {
        openOnTile();
        optimizeBbox(edgeSize);
      }
      buildFinalLook();
      recreateGenObj();
      get_app().fillPropPanel();
    }
  }
  void onLandClassAssetTexturesChanged(landclass::AssetData *data) override
  {
    if (landClassData == data)
    {
      clearFinalLook();
      buildFinalLook();
      recreateGenObj();
    }
  }
  void onSplineClassAssetChanged(splineclass::AssetData *data) override {}

protected:
  static const DataBlock &generatePolyGenEntityBlk(DataBlock &polyBlk, const char *asset_name, dag::ConstSpan<Point3> points,
    const Point2 &objOfs, bool objs_follow_detile)
  {
    DataBlock *b = polyBlk.addBlock("polygon");
    b->setStr("name", asset_name);
    b->setStr("blkGenName", asset_name);
    b->setPoint2("polyObjOffs", objOfs);
    b->setBool("tiledObjsWorldAnchor", true);
    b->setBool("plantedObjsWorldAnchor", true);
    b->setBool("objsFollowDetile", objs_follow_detile);
    for (auto &p : points)
      b->addNewBlock("point")->setPoint3("pt", p);
    return *b;
  }
  void recreateGenObj()
  {
    destroy_it(genObj);
    av_discard_rigen_area();

    ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
    DataBlock polyBlk;
    if (IObjEntity *e = splSrv->createVirtualSplineEntity(
          generatePolyGenEntityBlk(polyBlk, assetName, points, Point2(tile, tile), composeFits(groundHalf(tile * 2.f)))))
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

  inline float generateImage(const DataBlock *detTex)
  {
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

  // whole tiles around the polygon from its center; a triangle apex and the rhombus tips reach past half the edge size
  float groundHalf(float tile_size) const { return (ceilf(shapeHalf / tile_size - 0.5f) + 0.5f) * tile_size; }

  // the same rule the objects follow, plus a colormap to compose from
  bool canCompose() const { return landClassData && texId != BAD_TEXTUREID && landClassData->followsDetile(); }

  void clearFinalLook()
  {
    // the drawn material keeps its own ref; while held, re-registering the name returns the old texture
    if (finalLookTex.getId() != BAD_TEXTUREID)
    {
      rbuf.clearBuf();
      rbuf.addFaces(BAD_TEXTUREID);
    }
    finalLookTex.close();
    composedHalf = 0.f;
    composedOrigin = composedCenter = Point2(0, 0);
  }

  // the colormap as the terrain shows it: the shader's clusters, noise and blend,
  // run on the CPU into one image, on demand rather than per frame
  void buildFinalLook()
  {
    if (!finalLook || !canCompose())
    {
      clearFinalLook();
      return;
    }
    const float tileSize = tile * 2.f;
    const float ground = groundHalf(tileSize);
    if (composeFits(ground))
      return;
    clearFinalLook();
    const LandDetile &detile = landClassData->detile;
    // the pattern maps at the key the shader and the object generators read, not the preview's own tile
    const float patternSize = detile.size;
    SmallTab<E3DCOLOR> src;
    int sw = 0, sh = 0;
    if (!readTexRgba(texId, src, sw, sh))
    {
      DAEDITOR3.conError("final look: cannot read <%s> back, the plain tiling stays", assetName.str());
      return;
    }
    const int n = get_bigger_pow2(clamp((int)(sw * 2.f * ground / patternSize), 64, MAX_FINAL_LOOK_TEX));
    // one tap per output texel would fold everything above that density into moire
    const int wantW = (int)ceilf(n * patternSize / (2.f * ground));
    if (wantW < sw)
      boxDownsample(src, sw, sh, wantW);

    // the plain ground's frame, so the picture and the placement read one map
    const float step = 2.f * ground / n;
    const Point2 origin = groundOrigin();
    const float invNoise = 1.f / land_detile::noise_size;
    const int clusters = detile.clusters;

    Point3 mean(0, 0, 0);
    if (clusters > 1) // the blend below is its only reader, and one cluster never blends
    {
      for (const E3DCOLOR &c : src)
        mean += Point3(c.r, c.g, c.b);
      mean *= 1.f / (255.f * max((int)src.size(), 1));
    }

    SmallTab<E3DCOLOR> img;
    clear_and_resize(img, n * n);
    land_detile::Cluster cl[LAND_DETILE_MAX_CLUSTERS];
    for (int k = 0; k < clusters; k++) // nodes() never names one past the last
      cl[k] = land_detile::cluster(detile, k);

    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++)
      {
        const Point2 p(polyCenter.x - ground + (x + 0.5f) * step, polyCenter.y - ground + (y + 0.5f) * step);
        int seed = 0;
        float w = 1.f;
        if (clusters > 1)
          land_detile::nodes(land_detile::noise(p * invNoise) * (clusters - 1), seed, w);
        const Point3 c0 = sampleTile(src, sw, sh, land_detile::to_pattern(cl[seed], p) - origin, patternSize);
        Point3 c = c0;
        if (w < 1.f) // the pair the noise blends, as land_detile_inc.dshl mixes them
        {
          const Point3 c1 = sampleTile(src, sw, sh, land_detile::to_pattern(cl[seed + 1], p) - origin, patternSize);
          c = c1 + (c0 - c1) * w;
          c = mean + (c - mean) * safeinv(sqrtf(w * w + (1.f - w) * (1.f - w)));
        }
        img[y * n + x] = E3DCOLOR(clamp((int)(c.x * 255.f + 0.5f), 0, 255), clamp((int)(c.y * 255.f + 0.5f), 0, 255),
          clamp((int)(c.z * 255.f + 0.5f), 0, 255), 255);
      }

    const int levels = auto_mip_levels_count(n, 1);
    d3d::GpuAutoLock gpuLock;
    Texture *t = d3d::create_tex(nullptr, n, n, TEXFMT_A8R8G8B8 | TEXCF_SYSTEXCOPY | TEXCF_LOADONCE, levels, "land_final_look");
    // the quad is minified as often as it is filled, and the tile it stands in for has mips
    for (int lev = 0, w = n; lev < levels; lev++, w >>= 1)
    {
      if (lev)
      {
        SmallTab<E3DCOLOR> down;
        clear_and_resize(down, w * w);
        imagefunctions::downsample4x_c((unsigned char *)down.data(), (const unsigned char *)img.data(), w, w);
        img = eastl::move(down);
      }
      auto dst = t ? lock_texture<E3DCOLOR>(t, lev, TEXLOCK_WRITE) : LockedImage2DView<E3DCOLOR>{};
      if (!dst)
      {
        del_d3dres(t);
        DAEDITOR3.conError("final look: cannot make the %dx%d ground texture", n, n);
        return;
      }
      dst.writeRows(make_span(img));
    }
    finalLookTex.set(t, ":av_land_final_look");
    composedHalf = ground;
    composedOrigin = origin;
    composedCenter = polyCenter;
    d3d::SamplerInfo smpInfo; // one image over the polygon, not a tile: wrapping it would fold the far edge in
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    set_texture_separate_sampler(finalLookTex.getId(), smpInfo);
  }

  // wrapped and filtered; v runs against z, the way the land shader maps the tile
  static Point3 sampleTile(dag::ConstSpan<E3DCOLOR> img, int w, int h, const Point2 &q, float tile_size)
  {
    const float u = q.x / tile_size * w - 0.5f, v = -q.y / tile_size * h - 0.5f;
    const float fu = floorf(u), fv = floorf(v);
    const float tu = u - fu, tv = v - fv;
    const int x0 = ((int)fu % w + w) % w, y0 = ((int)fv % h + h) % h;
    const int x1 = (x0 + 1) % w, y1 = (y0 + 1) % h;
    Point3 c(0, 0, 0);
    const struct
    {
      int i;
      float k;
    } tap[4] = {
      {y0 * w + x0, (1 - tu) * (1 - tv)}, {y0 * w + x1, tu * (1 - tv)}, {y1 * w + x0, (1 - tu) * tv}, {y1 * w + x1, tu * tv}};
    for (const auto &t : tap)
      c += Point3(img[t.i].r, img[t.i].g, img[t.i].b) * t.k;
    return c * (1.f / 255.f);
  }

  // the driver decodes whatever the colormap was packed in; the scale is not left to the
  // copy, because a vulkan stretch blits mip 0 whatever the ratio and would alias
  static bool readTexRgba(TEXTUREID tid, SmallTab<E3DCOLOR> &out, int &w, int &h)
  {
    BaseTexture *bt = acquire_managed_tex(tid);
    if (!bt)
      return false;
    // streaming hands out a stub until the pixels arrive
    if (!prefetch_and_wait_managed_textures_loaded(make_span_const(&tid, 1), true))
    {
      release_managed_tex(tid);
      return false;
    }
    TextureInfo ti;
    bt->getinfo(ti);
    w = ti.w;
    h = ti.h;
    d3d::GpuAutoLock gpuLock;
    Texture *rt = d3d::create_tex(nullptr, w, h, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "land_final_look_src");
    bool ok = rt && d3d::stretch_rect(bt, rt); // same size: a copy, with no filter to depend on
    if (auto src = ok ? lock_texture<const E3DCOLOR>(rt, 0, TEXLOCK_READ) : LockedImage2DView<const E3DCOLOR>{})
    {
      clear_and_resize(out, w * h);
      src.readRows(make_span(out));
    }
    else
      ok = false;
    del_d3dres(rt);
    release_managed_tex(tid);
    return ok;
  }

  // every source texel of the footprint, so nothing above the wanted density aliases
  static void boxDownsample(SmallTab<E3DCOLOR> &img, int &w, int &h, int want_w)
  {
    const int ow = want_w, oh = max(h * want_w / w, 1);
    SmallTab<E3DCOLOR> out;
    clear_and_resize(out, ow * oh);
    for (int y = 0; y < oh; y++)
    {
      const int y0 = y * h / oh, y1 = max((y + 1) * h / oh, y0 + 1);
      for (int x = 0; x < ow; x++)
      {
        const int x0 = x * w / ow, x1 = max((x + 1) * w / ow, x0 + 1);
        unsigned r = 0, g = 0, b = 0, n = 0;
        for (int sy = y0; sy < y1; sy++)
          for (int sx = x0; sx < x1; sx++, n++)
          {
            const E3DCOLOR &c = img[sy * w + sx];
            r += c.r, g += c.g, b += c.b;
          }
        out[y * ow + x] = E3DCOLOR(r / n, g / n, b / n, 255);
      }
    }
    img = eastl::move(out);
    w = ow;
    h = oh;
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
      bbox += Point3(polyCenter.x + rdq, 0, polyCenter.y + rdq);
      bbox += Point3(polyCenter.x - rdq, rdq, polyCenter.y - rdq);
    }
  }

  // the square of one tile around the tile nearest the origin
  void openOnTile()
  {
    viewCenter = tileCenter();
    setPoints(tile > 0 ? tile * 2.0f : edgeSize);
  }

  // the colormap, its size and its offset from detail{}; drops the image and the old texture first
  void loadGround(const DagorAsset &asset)
  {
    clearRenderBuf();
    clearFinalLook();
    clearGrass(); // clones of the previous load
    tile = landClassData ? generateImage(asset.props.getBlockByName("detail")) / 2.f : 0.f;
    if (tile < 0.f)
      tile = 256.f;
    detailOffset = asset.props.getBlockByNameEx("detail")->getPoint2("offset", Point2(0, 0));
  }

  // the composed image is current for this ground square and colormap origin
  bool composeFits(float ground) const
  {
    return ground == composedHalf && polyCenter == composedCenter && groundOrigin() == composedOrigin;
  }

  // colormap origin: objects' half-tile phase plus detail{} offset, as the land shader maps it
  Point2 groundOrigin() const { return Point2(tile, tile) + detailOffset; }

  // center of the tile nearest the world origin
  Point2 tileCenter() const
  {
    if (tile <= 0.f)
      return Point2(0, 0);
    const float tileSize = tile * 2.f;
    const Point2 c = groundOrigin() + Point2(tile, tile);
    return Point2(c.x - tileSize * floorf((c.x + tile) / tileSize), c.y - tileSize * floorf((c.y + tile) / tileSize));
  }

  // whole tiles around polyCenter: u = (x - origin.x) / size, v = -(z - origin.z) / size; the composed image spans 0..1
  void drawGround(bool composed)
  {
    const float g = groundHalf(tile * 2.f);
    const Point2 origin = composed ? polyCenter - Point2(g, g) : groundOrigin();
    const Point2 invSize = composed ? Point2(0.5f / g, -0.5f / g) : Point2(0.5f / tile, 0.5f / tile);
    // net vertices (0,0) (1,0) (0,1) (1,1): x grows with column, z falls with row (drawQuad winding)
    DynRenderBuffer::Vertex *v = rbuf.drawNetSurface(2, 2);
    for (int i = 0; i < 4; i++)
    {
      const float x = polyCenter.x + ((i & 1) ? g : -g), z = polyCenter.y + ((i & 2) ? -g : g);
      v[i].pos = Point3(x, yOffset, z);
      v[i].color = E3DCOLOR(255, 255, 255, 0);
      v[i].tc = Point2((x - origin.x) * invSize.x, -(z - origin.y) * invSize.y);
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

    // keep the shape inside the RIGen area, objects are not generated outside it
    Point2 ext(0, 0);
    for (const Point3 &p : points)
      ext = Point2(max(ext.x, fabsf(p.x)), max(ext.y, fabsf(p.z)));
    const Point2 lim(max(RIGEN_AREA_HALF - ext.x, 0.f), max(RIGEN_AREA_HALF - ext.y, 0.f));
    polyCenter = Point2(clamp(viewCenter.x, -lim.x, lim.x), clamp(viewCenter.y, -lim.y, lim.y));
    shapeHalf = max(ext.x, ext.y);
    const bool outside = shapeHalf > RIGEN_AREA_HALF;
    if (outside && !shapeOutsideArea)
      DAEDITOR3.conWarning("land preview: the shape reaches past the RI area (+-%.0f m), objects there are not generated",
        RIGEN_AREA_HALF);
    shapeOutsideArea = outside;
    for (Point3 &p : points)
      p += Point3(polyCenter.x, 0, polyCenter.y);
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
  Point2 detailOffset = Point2(0, 0);
  Point2 viewCenter = Point2(0, 0); // requested center
  Point2 polyCenter = Point2(0, 0); // viewCenter clamped to the RIGen area
  float shapeHalf = 0.f;            // taken before the center shift, which would round it
  bool shapeOutsideArea = false;
  String assetName;
  FigType presentationType;
  bool finalLook = false;
  TextureIDHolder finalLookTex; // the composed de-tiled ground, empty while the plain tiling shows
  float composedHalf = 0.f;     // the ground half extent it was composed for; the figure moves it, not only the size
  Point2 composedOrigin, composedCenter;
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
