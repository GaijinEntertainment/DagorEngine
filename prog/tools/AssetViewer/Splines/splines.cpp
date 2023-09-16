#include "../av_plugin.h"
#include <EditorCore/ec_interface.h>
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <de3_genObjAlongSpline.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <math/dag_e3dColor.h>
#include <de3_huid.h>
#include <de3_assetService.h>
#include <propPanel2/c_panel_base.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/ObjCreator3d/objCreator3d.h>

enum
{
  PID_SPLINE_TYPE_GROUP = 5,
};
static int rendEntGeomMask = -1;
static int collisionMask = -1;

class SplineViewPlugin : public IGenEditorPlugin, public ControlEventHandler
{
public:
  SplineViewPlugin() : entPool(midmem), spline(NULL), splineLen(0.f), assetName(""), presentationType(FIG_TYPE__FIRST)
  {
    loftGeom = NULL;
    initScriptPanelEditor("spline.scheme.nut", "spline by scheme");
  }

  ~SplineViewPlugin() { del_it(spline); }

  virtual const char *getInternalName() const { return "splineViewer"; }

  virtual void registered()
  {
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
  }
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    del_it(spline);
    spline = new BezierSpline3d();

    assetName = asset->getName();
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    const splineclass::AssetData *adata = assetSrv->getSplineClassData(assetName);

    splineLen = calculateNeedLen(adata);

    presentationType = getTypeFromData(splineLen, adata);

    if (!generateSplineType(spline, presentationType, splineLen))
    {
      del_it(spline);
      return false;
    }

    generateObjectBySpline(*spline, adata);
    if (spEditor && asset)
      spEditor->load(asset);
    return true;
  }

  virtual bool end()
  {
    if (spEditor)
      spEditor->destroyPanel();
    clearEntitys();

    del_it(spline);
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

    if (presentationType == FIG_TYPE_LINE)
      ::draw_cached_debug_line(spline->get_pt(0.f), spline->get_pt(spline->leng), E3DCOLOR(255, 255, 0));
    else
    {
      Point3 lp, np;
      lp = spline->get_pt(0.f);

      for (float i = 0.f; i < spline->leng; i += 2.f) //-V1034
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
  virtual void renderGeometry(Stage stage)
  {
    if (!loftGeom)
      return;
    unsigned mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    loftGeom->setTm(TMatrix::IDENT);
    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE:
      case STG_RENDER_SHADOWS:
        if ((mask & rendEntGeomMask) == rendEntGeomMask)
          loftGeom->render();
        break;

      case STG_RENDER_STATIC_TRANS:
        if ((mask & rendEntGeomMask) == rendEntGeomMask)
          loftGeom->renderTrans();
        break;
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "spline") == 0; }

  virtual void fillPropPanel(PropertyContainerControlBase &panel)
  {
    panel.setEventHandler(this);

    PropertyContainerControlBase *rg = panel.createRadioGroup(PID_SPLINE_TYPE_GROUP, "spline presentation type:");
    rg->createRadio(FIG_TYPE_LINE, "line");
    rg->createRadio(FIG_TYPE_TRIANGLE, "rounded triangle");
    rg->createRadio(FIG_TYPE_CROWN, "crown");

    panel.setInt(PID_SPLINE_TYPE_GROUP, presentationType);
  }

  virtual void postFillPropPanel() {}

  virtual void onChange(int pid, PropertyContainerControlBase *panel)
  {
    if (!spline || assetName.empty())
      return;

    if (pid != PID_SPLINE_TYPE_GROUP)
      return;

    const FigType type = (FigType)panel->getInt(pid);

    if (type == presentationType)
      return;

    if (!generateSplineType(spline, type, splineLen))
    {
      del_it(spline);
      return;
    }

    presentationType = type;

    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    const splineclass::AssetData *adata = assetSrv->getSplineClassData(assetName);
    if (!adata)
      return;

    generateObjectBySpline(*spline, adata);

    if (EDITORCORE->getCurrentViewport())
      EDITORCORE->getCurrentViewport()->zoomAndCenter(bbox);
  }

protected:
  enum FigType
  {
    FIG_TYPE__FIRST = 1,

    FIG_TYPE_LINE = FIG_TYPE__FIRST,
    FIG_TYPE_TRIANGLE,
    FIG_TYPE_CROWN,

    FIG_TYPE__COUNT,
  };

  inline float calculateNeedLen(const splineclass::AssetData *adata)
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

  inline FigType getTypeFromData(float len, const splineclass::AssetData *adata)
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

  bool generateSplineType(BezierSpline3d *spline, FigType type, float len)
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

      G_VERIFY(spline->calculateCatmullRom(pt, 2, false));
    }
    else if (type == FIG_TYPE_TRIANGLE)
    {
      double r = 15.f * (len / 75.f);
      if (r < 15.f)
        r = 15.f;

      Point3 pt[3];

      pt[0].y = pt[1].y = pt[2].y = 0.f;

      pt[0].x = r;
      pt[0].z = 0.f;

      pt[1].x = 0.f;
      pt[1].z = r;

      pt[2].x = -r;
      pt[2].z = 0.f;

      G_VERIFY(spline->calculateCatmullRom(pt, 3, true));
    }
    else if (type == FIG_TYPE_CROWN)
    {
      const int semiringCnt = 5;
      const int pPerSemiring = 2;

      const int pCnt = pPerSemiring * semiringCnt + 1 + 2;
      Point3 pt[pCnt];

      double r = (100.f / double(semiringCnt)) * (len / 742.f);

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

        G_VERIFY(spline->calculateCatmullRom(pt, pCnt, true));

        spLen = spline->leng;
      }
    }

    return true;
  }

  inline void generateObjectBySpline(BezierSpline3d &spline, const splineclass::AssetData *adata)
  {
    clearEntitys();
    del_it(loftGeom);

    const int subtype = IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");
    const int rseed = 12345;

    const int segCnt = spline.segs.size();

    const bool closed = spline.isClosed();
    objgenerator::generateBySpline(::toPrecSpline(spline), NULL, 0, segCnt, adata, entPool, nullptr, closed, subtype, 0, rseed, 0);

    bbox.setempty();
    for (int i = entPool.size() - 1; i >= 0; i--)
    {
      splineclass::SingleEntityPool &spool = entPool[i];
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

    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (assetSrv && adata->genGeom)
    {
      loftGeom = new GeomObject;
      for (int j = 0; j < adata->genGeom->loft.size(); j++)
      {
        if (!assetSrv->isLoftCreatable(adata->genGeom, j))
          continue;
        splineclass::LoftGeomGenData::Loft &loft = adata->genGeom->loft[j];
        if (loft.makeDelaunayPtCloud || loft.waterSurface)
          continue;

        for (int materialNo = 0; materialNo < loft.matNames.size(); materialNo++)
        {
          Ptr<MaterialData> material;
          material = assetSrv->getMaterialData(loft.matNames[materialNo]);
          if (!material.get())
          {
            DAEDITOR3.conError("<%s>: invalid material <%s> for loft", assetName, loft.matNames[materialNo].str());
            continue;
          }
          StaticGeometryContainer *g = new StaticGeometryContainer;

          Mesh *mesh = new Mesh;

          Tab<splineclass::Attr> splineScales(midmem);
          splineScales.resize(segCnt + 1);
          mem_set_0(splineScales);
          for (int i = 0; i < splineScales.size(); i++)
          {
            splineScales[i].scale_h = splineScales[i].scale_w = splineScales[i].opacity = 1.0;
            splineScales[i].followOverride = splineScales[i].roadBhvOverride = -1;
          }

          if (!assetSrv->createLoftMesh(*mesh, adata->genGeom, j, spline, 0, segCnt, true, 1.0f, materialNo, splineScales, NULL,
                assetName, 0, 0))
          {
            delete mesh;
            continue;
          }

          MaterialDataList mat;
          mat.addSubMat(material);

          ObjCreator3d::addNode(assetName, mesh, &mat, *g);

          for (int i = 0; i < g->nodes.size(); ++i)
          {
            StaticGeometryNode *node = g->nodes[i];

            node->flags = loft.flags;
            node->normalsDir = loft.normalsDir;

            node->calcBoundBox();
            node->calcBoundSphere();

            if (loft.loftLayerOrder)
              node->script.setInt("layer", loft.loftLayerOrder);
            if (loft.stage)
              node->script.setInt("stage", loft.stage);
            loftGeom->getGeometryContainer()->addNode(new StaticGeometryNode(*node));
          }
          delete g;
        }
      }
      loftGeom->setTm(TMatrix::IDENT);
      loftGeom->recompile();
      bbox += loftGeom->getBoundBox();
    }
  }

  inline void clearEntitys()
  {
    for (int i = entPool.size() - 1; i >= 0; i--)
      entPool[i].clear();
  }

private:
  BezierSpline3d *spline;
  float splineLen;
  String assetName;
  FigType presentationType;
  BBox3 bbox;
  GeomObject *loftGeom;
  Tab<splineclass::SingleEntityPool> entPool;
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
