// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <math/dag_e3dColor.h>
#include <de3_huid.h>
#include <de3_splineGenSrv.h>
#include <de3_assetService.h>
#include <de3_rendInstGen.h>
#include <propPanel/control/container.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

enum
{
  PID_SPLINE_TYPE_GROUP = 5,
  PID_SPLINE_LEN,
  PID_SPLINE_GEOMTAGS_GROUP,
  PID_SPLINE_GEOMTAGS_0,
  PID_SPLINE_GEOMTAGS_LAST = PID_SPLINE_GEOMTAGS_0 + 256,
};
static int rendEntGeomMask = -1;
static int collisionMask = -1;

class SplineViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  SplineViewPlugin() : splineLen(0.f), assetName(""), splineType(FIG_TYPE__FIRST)
  {
    initScriptPanelEditor("spline.scheme.nut", "spline by scheme");
    splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  }

  ~SplineViewPlugin() override { end(); }

  const char *getInternalName() const override { return "splineViewer"; }

  void registered() override
  {
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
  }
  void unregistered() override {}

  bool begin(DagorAsset *asset) override
  {
    assetName = asset->getName();
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    const splineclass::AssetData *adata = assetSrv->getSplineClassData(assetName);

    splineLen = calculateNeedLen(adata);
    splineType = getTypeFromData(splineLen, adata);
    recreateGenObj();

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
    const BezierSpline3d *spline = nullptr;
    if (ISplineEntity *se = genObj ? genObj->queryInterface<ISplineEntity>() : nullptr)
      spline = &se->getBezierSpline();

    if (!spline)
      return;

    ::begin_draw_cached_debug_lines();

    if (splineType == FIG_TYPE_LINE)
      ::draw_cached_debug_line(spline->get_pt(0.f), spline->get_pt(spline->leng), E3DCOLOR(255, 255, 0));
    else
    {
      Point3 lp, np;
      float step = min(spline->leng / 256.f, 2.f);
      lp = spline->get_pt(0.f);

      for (float i = 0.f; i < spline->leng; i += step) //-V1034
      {
        np = spline->get_pt(i);
        ::draw_cached_debug_line(lp, np, E3DCOLOR(255, 255, 0));
        lp = np;
      }
      ::draw_cached_debug_line(lp, spline->get_pt(spline->leng), E3DCOLOR(255, 255, 0));

      if (spline->isClosed())
        ::draw_cached_debug_line(lp, spline->get_pt(0), E3DCOLOR(255, 255, 0));
    }

    ::end_draw_cached_debug_lines();
  }
  void renderGeometry(Stage stage) override
  {
    if (taggedGeom)
      switch (stage)
      {
        case STG_RENDER_STATIC_OPAQUE:
        case STG_RENDER_SHADOWS: taggedGeom->render(); break;

        case STG_RENDER_STATIC_TRANS: taggedGeom->renderTrans(); break;

        default: break;
      }
  }

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "spline") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override
  {
    panel.setEventHandler(this);

    PropPanel::ContainerPropertyControl *rg = panel.createRadioGroup(PID_SPLINE_TYPE_GROUP, "spline presentation type:");
    rg->createRadio(FIG_TYPE_LINE, "line");
    rg->createRadio(FIG_TYPE_ARC, "arc");
    rg->createRadio(FIG_TYPE_TRIANGLE, "rounded triangle");
    rg->createRadio(FIG_TYPE_CROWN, "crown");

    panel.setInt(PID_SPLINE_TYPE_GROUP, splineType);
    panel.createEditFloat(PID_SPLINE_LEN, "spline length", splineLen);
    panel.setMinMaxStep(PID_SPLINE_LEN, 10.f, 3000.f, 5.f);

    if (genObj)
    {
      panel.createSeparator();
      if (geomTags.nameCount())
      {
        rg = panel.createRadioGroup(PID_SPLINE_GEOMTAGS_GROUP, "Loft tags:");
        iterate_names_in_lexical_order(geomTags, [&](int id, const char *nm) { rg->createRadio(PID_SPLINE_GEOMTAGS_0 + id, nm); });
        rg->createRadio(PID_SPLINE_GEOMTAGS_LAST, "--- don't show tagged geometry ---");
        panel.setInt(PID_SPLINE_GEOMTAGS_GROUP, PID_SPLINE_GEOMTAGS_LAST);
      }
      else
        panel.createStatic(-1, "No loft tags for generated geometry");
    }
  }

  void postFillPropPanel() override {}

  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override
  {
    if (!genObj || assetName.empty())
      return;

    if (pid == PID_SPLINE_TYPE_GROUP && splineType != (FigType)panel->getInt(pid))
      splineType = (FigType)panel->getInt(pid);
    else if (pid == PID_SPLINE_LEN && fabsf(splineLen - panel->getFloat(pid)) > 1.f)
      splineLen = panel->getFloat(pid);
    else if (pid == PID_SPLINE_GEOMTAGS_GROUP && genObj && splSrv)
    {
      int tag = panel->getInt(pid) == PID_SPLINE_GEOMTAGS_LAST ? -1 : panel->getInt(pid) - PID_SPLINE_GEOMTAGS_0;
      if (showReqTagId != tag)
      {
        showReqTagId = tag;
        prepareTaggedGeom();
      }
      return;
    }
    else
      return;

    recreateGenObj();
    if (EDITORCORE->getCurrentViewport())
      EDITORCORE->getCurrentViewport()->zoomAndCenter(bbox);
  }

protected:
  enum FigType
  {
    FIG_TYPE__FIRST = 1,

    FIG_TYPE_LINE = FIG_TYPE__FIRST,
    FIG_TYPE_ARC,
    FIG_TYPE_TRIANGLE,
    FIG_TYPE_CROWN,

    FIG_TYPE__COUNT,
  };

  static inline float calculateNeedLen(const splineclass::AssetData *adata)
  {
    float len = 10.f;
    if (adata->gen)
    {
      const int dCnt = adata->gen->data.size();
      for (int i = 0; i < dCnt; i++)
      {
        const splineclass::SingleGenEntityGroup &gItem = adata->gen->data[i];

        const float maxL = gItem.step.x + gItem.step.y;
        float maxN = 10.f;
        for (int j = gItem.genEntRecs.size() - 1; j >= 0; j--)
        {
          const float weight = gItem.genEntRecs[j].weight / gItem.sumWeight;
          if (weight < 1e-6)
            continue;

          const float n = (2.f / weight);
          if (maxN < n)
            maxN = n;
        }

        const float locLen = maxN * maxL;
        if (locLen > len)
          len = locLen;
      }
    }

    if (len > 10000)
      len = 10000;

    return len;
  }

  static inline FigType getTypeFromData(float len, const splineclass::AssetData *adata)
  {
    if (!adata->gen)
      return FIG_TYPE_LINE;

    bool atStart = false;
    bool atEnd = false;

    float maxOffset = 0.f;
    for (int i = adata->gen->data.size() - 1; i >= 0; i--)
    {
      const splineclass::SingleGenEntityGroup &gItem = adata->gen->data[i];
      if (gItem.offset.y > maxOffset)
        maxOffset = gItem.offset.y;

      atStart |= gItem.placeAtStart;
      atEnd |= gItem.placeAtEnd;
    }

    if (atStart && atEnd || (maxOffset >= 1.f))
      return FIG_TYPE_LINE;

    if (len < 200.f)
      return FIG_TYPE_TRIANGLE;

    return FIG_TYPE_CROWN;
  }

  static bool generateSplinePoints(DataBlock &blk, FigType type, float len)
  {
    if (type == FIG_TYPE_LINE)
    {
      const float halfLen = len / 2.f;
      Point3 pt[2];

      if (len > 200.f)
      {
        const float a = halfLen / sqrt(2.f);

        pt[0].x = pt[0].z = -a;
        pt[0].y = 0;

        pt[1].x = pt[1].z = +a;
        pt[1].y = 0;
      }
      else
      {
        pt[0].x = -halfLen;
        pt[0].y = pt[0].z = 0;

        pt[1].x = +halfLen;
        pt[1].y = pt[1].z = 0;
      }
      for (auto &p : pt)
        blk.addNewBlock("point")->setPoint3("pt", p);
      blk.setBool("forceCatmullRom", true);
    }
    else if (type == FIG_TYPE_ARC)
    {
      double r = len / 2.5f;
      Point3 pt[3];

      pt[0].y = pt[1].y = pt[2].y = 0.f;

      pt[0].x = r;
      pt[0].z = 0.f;

      pt[1].x = 0.f;
      pt[1].z = r * 0.15f;

      pt[2].x = -r;
      pt[2].z = 0.f;

      for (auto &p : pt)
        blk.addNewBlock("point")->setPoint3("pt", p);
      blk.setBool("forceCatmullRom", true);
      blk.setInt("cornerType", 1);
    }
    else if (type == FIG_TYPE_TRIANGLE)
    {
      double r = 15.f * (len / 75.f);
      if (r < 5.f)
        r = 5.f;

      Point3 pt[3];

      pt[0].y = pt[1].y = pt[2].y = 0.f;

      pt[0].x = r;
      pt[0].z = 0.f;

      pt[1].x = 0.f;
      pt[1].z = r;

      pt[2].x = -r;
      pt[2].z = 0.f;

      for (auto &p : pt)
        blk.addNewBlock("point")->setPoint3("pt", p);
      blk.setBool("closed", true);
      blk.setBool("forceCatmullRom", true);
      blk.setInt("cornerType", 1);
    }
    else if (type == FIG_TYPE_CROWN)
    {
      const int semiringCnt = 5;
      const int pPerSemiring = 2;

      const int pCnt = pPerSemiring * semiringCnt + 1 + 2;
      Point3 pt[pCnt];

      double r = (100.f / double(semiringCnt)) * (len / 742.f);
      BezierSpline3d spline;

      if (r < (50.f / double(semiringCnt)))
        r = 50.f / double(semiringCnt);
      else if (r > (200.f / double(semiringCnt)))
        r = 200.f / double(semiringCnt);

      double amp = (r <= 30.f) ? 2.f : 3.f;
      for (float spLen = 0.f; len > spLen; amp += 0.5) //-V1034
      {
        const double d = r * 2.f;
        const double rm2 = r / 2.f;
        const double deltaRad = PI / 2.f;

        double offset = 0.f;
        double rad = 0.f;
        int ptInd = 1;
        for (int semiring = 0; semiring < semiringCnt; semiring++, offset -= d)
        {
          const bool fullRing = (semiring % 2);
          for (int i = 0; i < pPerSemiring; i++, ptInd++)
          {
            pt[ptInd].x = cos(rad) * (fullRing ? -r : r) + offset;
            pt[ptInd].z = sin(rad) * r * amp;
            pt[ptInd].y = 0.f;

            rad += deltaRad;
          }
        }

        pt[ptInd].x = cos(rad) * ((semiringCnt % 2) ? -r : r) + offset;
        pt[ptInd].z = sin(rad) * r * amp;
        pt[ptInd].y = 0.f;

        offset += d;

        pt[0].x = -rm2;
        const double delta = (double(semiringCnt) * r - 2.f * r * amp);
        pt[0].z = -r * amp - (delta > 0 ? delta : rm2);
        pt[0].y = 0.f;

        const int eInd = pCnt - 1;
        pt[eInd].x = pt[eInd - 1].x + r + rm2;
        pt[eInd].z = pt[0].z;
        pt[eInd].y = 0.f;

        G_VERIFY(spline.calculateCatmullRom(pt, pCnt, true));

        spLen = spline.leng;
      }
      for (auto &p : pt)
        blk.addNewBlock("point")->setPoint3("pt", p);
      blk.setBool("closed", true);
      blk.setBool("forceCatmullRom", true);
      blk.setInt("cornerType", 0);
    }

    return true;
  }

  static const DataBlock &generateSplineGenEntityBlk(DataBlock &polyBlk, const char *asset_name, FigType ftype, float splineLen)
  {
    DataBlock *b = polyBlk.addBlock("spline");
    b->setStr("name", asset_name);
    b->setStr("blkGenName", asset_name);
    generateSplinePoints(*b, ftype, splineLen);
    return *b;
  }
  void recreateGenObj()
  {
    destroy_it(genObj);
    if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
      rigenSrv->discardRIGenRect(0, 0, 64, 64);

    ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
    DataBlock splineBlk;
    if (IObjEntity *e = splSrv->createVirtualSplineEntity(generateSplineGenEntityBlk(splineBlk, assetName, splineType, splineLen)))
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

private:
  IObjEntity *genObj = nullptr;
  GeomObject *taggedGeom = nullptr;
  OAHashNameMap<true> geomTags;
  int showReqTagId = -1;
  float splineLen;
  String assetName;
  FigType splineType;
  BBox3 bbox;
  ISplineGenService *splSrv = nullptr;
};

static InitOnDemand<SplineViewPlugin> plugin;


void init_plugin_splines()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
