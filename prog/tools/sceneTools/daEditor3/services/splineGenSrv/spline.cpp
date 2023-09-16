class SplineEntity : public IObjEntity, public IRandomSeedHolder, public ISplineEntity
{
public:
  struct SplineData : DObject
  {
    struct Attr
    {
      DagorAsset *splineClass = nullptr;
      splineclass::Attr attr;
    };

    int rndSeed = 0;
    int perInstSeed = 0;

    bool placeOnCollision = false;
    bool maySelfCross = false;
    bool isPoly = false;

    short cornerType = 2; // -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    bool exportable = false;
    bool useForNavMesh = false;
    float navMeshStripeWidth = 0;

    int layerOrder = 0;
    float scaleTcAlong = 1;

    struct PolyProps
    {
      Point2 objOffs = Point2(0, 0);
      real objRot = 0;
      float curvStrength = 1;
      float minStep = 10, maxStep = 100;
      bool smooth = false;
    } poly;

    Tab<ISplineGenObj::SplinePt> pt;
    Tab<Attr> ptAttr;
    SimpleString splAssetName;

    BBox3 lbb;
    BSphere3 lbs;

  public:
    bool load(const DataBlock &blk)
    {
      static int spline_atype = DAEDITOR3.getAssetTypeId("spline");

      isPoly = strcmp(blk.getBlockName(), "polygon") == 0;

      splAssetName = blk.getStr("blkGenName", "");
      rndSeed = blk.getInt("rseed", 0);
      perInstSeed = blk.getInt("perInstSeed", 0);

      layerOrder = blk.getInt("layerOrder", 0);
      scaleTcAlong = blk.getReal("scaleTcAlong", 1.0f);

      maySelfCross = blk.getBool("maySelfCross", false);
      placeOnCollision = (blk.getInt("modifType", 0) == 1);

      exportable = blk.getBool("exportable", false);
      cornerType = blk.getInt("cornerType", 0);
      useForNavMesh = blk.getBool("useForNavMesh", false);
      navMeshStripeWidth = blk.getReal("navMeshStripeWidth", 20);

      poly.objOffs = blk.getPoint2("polyObjOffs", Point2(0, 0));
      poly.objRot = blk.getReal("polyObjRot", 0);
      poly.smooth = blk.getBool("polySmooth", false);
      poly.curvStrength = blk.getReal("polyCurv", 1);
      poly.minStep = blk.getReal("polyMinStep", 30);
      poly.maxStep = blk.getReal("polyMaxStep", 1000);

      int nid = blk.getNameId("point");
      pt.reserve(blk.blockCount());
      ptAttr.reserve(blk.blockCount());
      for (int i = 0; i < blk.blockCount(); i++)
        if (blk.getBlock(i)->getBlockNameId() == nid)
        {
          const DataBlock &cb = *blk.getBlock(i);
          ISplineGenObj::SplinePt &p = pt.push_back();
          Attr &pa = ptAttr.push_back();
          const char *scname = cb.getBool("useDefSplineGen", true) ? splAssetName.str() : cb.getStr("blkGenName", "");

          p.pt = cb.getPoint3("pt", Point3(0, 0, 0));
          p.relIn = cb.getPoint3("in", Point3(0, 0, 0));
          p.relOut = cb.getPoint3("out", Point3(0, 0, 0));
          pa.attr.scale_h = cb.getReal("scale_h", 1.0f);
          pa.attr.scale_w = cb.getReal("scale_w", 1.0f);
          pa.attr.opacity = cb.getReal("opacity", 1.0f);
          pa.attr.tc3u = cb.getReal("tc3u", 1.0f);
          pa.attr.tc3v = cb.getReal("tc3v", 1.0f);
          pa.attr.followOverride = cb.getInt("followOverride", -1);
          pa.attr.roadBhvOverride = cb.getInt("roadBhvOverride", -1);
          p.cornerType = cb.getInt("cornerType", -2);
          pa.splineClass = *scname ? DAEDITOR3.getAssetByName(scname, spline_atype) : nullptr;

          lbb += p.pt;
          lbs += p.pt;
        }

      if (blk.getBool("closed", false) && pt.size() > 1)
        pt.push_back(ISplineGenObj::SplinePt(pt[0]));

      pt.shrink_to_fit();
      ptAttr.shrink_to_fit();
      return pt.size() > 1;
    }
    bool save(DataBlock &blk, const TMatrix &tm) const
    {
      blk.setStr("blkGenName", splAssetName);
      blk.setInt("rseed", rndSeed);
      blk.setInt("perInstSeed", perInstSeed);

      blk.setInt("layerOrder", layerOrder);
      blk.setReal("scaleTcAlong", scaleTcAlong);

      blk.setBool("maySelfCross", maySelfCross);
      blk.setInt("modifType", placeOnCollision ? 1 : 0);

      blk.setBool("exportable", exportable);
      blk.setInt("cornerType", cornerType);
      blk.setBool("useForNavMesh", useForNavMesh);
      blk.setReal("navMeshStripeWidth", navMeshStripeWidth);

      blk.setPoint2("polyObjOffs", poly.objOffs);
      blk.setReal("polyObjRot", poly.objRot);
      blk.setBool("polySmooth", poly.smooth);
      blk.setReal("polyCurv", poly.curvStrength);
      blk.setReal("polyMinStep", poly.minStep);
      blk.setReal("polyMaxStep", poly.maxStep);

      if (pt.size() > ptAttr.size())
        blk.setBool("closed", true);

      for (int i = 0; i < ptAttr.size(); i++)
      {
        DataBlock &cb = *blk.addNewBlock("point");
        const ISplineGenObj::SplinePt &p = pt[i];
        const Attr &pa = ptAttr[i];

        cb.setPoint3("pt", tm * p.pt);
        cb.setPoint3("in", tm % p.relIn);
        cb.setPoint3("out", tm % p.relOut);
        cb.setReal("scale_h", pa.attr.scale_h);
        cb.setReal("scale_w", pa.attr.scale_w);
        cb.setReal("opacity", pa.attr.opacity);
        cb.setReal("tc3u", pa.attr.tc3u);
        cb.setReal("tc3v", pa.attr.tc3v);
        cb.setInt("followOverride", pa.attr.followOverride);
        cb.setInt("roadBhvOverride", pa.attr.roadBhvOverride);
        cb.setInt("cornerType", p.cornerType);
        if (pa.splineClass)
        {
          if (strcmp(pa.splineClass->getName(), splAssetName) != 0)
          {
            cb.setBool("useDefSplineGen", false);
            cb.setStr("blkGenName", pa.splineClass->getName());
          }
        }
        else if (!splAssetName.empty())
        {
          cb.setBool("useDefSplineGen", false);
          cb.setStr("blkGenName", "");
        }
      }

      return true;
    }
  };
  struct GenPart
  {
    IObjEntity *e = nullptr;
    int startIdx = 0, endIdx = 0;
  };

public:
  SplineEntity() : IObjEntity(-1) { tm.identity(); }
  ~SplineEntity()
  {
    for (auto &gp : ptGen)
      if (gp.e)
        gp.e->destroy();
    clear_and_shrink(ptGen);
    sd = NULL;
  }

  bool load(const DataBlock &blk)
  {
    // debug("SplineEntity.load(%s)", blk.getStr("name", "n/a"));
    sd = new SplineData;
    if (sd->load(blk))
      return true;
    sd = nullptr;
    return false;
  }

  void updateSpline()
  {
    int pts_num = sd->pt.size();

    SmallTab<Point3, TmpmemAlloc> pts;
    clear_and_resize(pts, pts_num * 3);

    for (int pi = 0; pi < pts_num; ++pi)
    {
      int wpi = pi % sd->pt.size();
      pts[pi * 3 + 1] = tm * sd->pt[wpi].pt;
      if (sd->isPoly && !sd->poly.smooth)
        pts[pi * 3 + 2] = pts[pi * 3] = pts[pi * 3 + 1];
      else
      {
        pts[pi * 3] = tm * (sd->pt[wpi].pt + getPtEffRelBezierIn(sd->pt, wpi, sd->cornerType));
        pts[pi * 3 + 2] = tm * (sd->pt[wpi].pt + getPtEffRelBezierOut(sd->pt, wpi, sd->cornerType));
      }
    }

    bezierSpline.calculate(pts.data(), pts.size(), false);
  }

  void generateData()
  {
    static int splineSubTypeId = DAEDITOR3.registerEntitySubTypeId("spline_cls");

    Tab<Attr> splineScales(midmem);
    for (int i = 0; i <= sd->ptAttr.size(); i++)
      splineScales.push_back(sd->ptAttr[i % sd->ptAttr.size()].attr);

    String name(0, "s%p", this);
    for (auto &gp : ptGen)
      if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
        gen->generateLoftSegments(bezierSpline, name, gp.startIdx, gp.endIdx, sd->placeOnCollision,
          make_span_const(splineScales).subspan(gp.startIdx, gp.endIdx - gp.startIdx + 1), sd->scaleTcAlong);

    BezierSpline3d onGndSpline;
    BezierSpline3d &effSpline = sd->placeOnCollision ? onGndSpline : bezierSpline;

    if (sd->placeOnCollision)
    {
      for (auto &gp : ptGen)
        if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
          for (auto &p : gen->entPools)
            p.resetUsedEntities();

      if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
        splSrv->build_ground_spline(onGndSpline, sd->pt, tm, sd->cornerType, sd->isPoly, sd->poly.smooth,
          sd->pt.size() > sd->ptAttr.size());
    }

    for (auto &gp : ptGen)
      if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
        gen->generateObjects(effSpline, gp.startIdx, gp.endIdx, splineSubTypeId, editLayerIdx, rndSeed, instSeed, nullptr);
  }

  void setTm(const TMatrix &_tm) override
  {
    if (gizmoEnabled)
    {
      TMatrix transf;
      transf = _tm * inverse(tm);
      tm = _tm;
      for (auto &gp : ptGen)
        if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
          gen->transform(transf);
      return;
    }
    tm = _tm;
    updateSpline();
    generateData();
  }
  void getTm(TMatrix &_tm) const override { _tm = tm; }

  // void setSubtype(int t) override { subType = t; }
  void setEditLayerIdx(int t) override
  {
    editLayerIdx = t;
    for (auto &gp : ptGen)
      gp.e->setEditLayerIdx(t);
  }
  void setGizmoTranformMode(bool enabled) override
  {
    if (gizmoEnabled == enabled)
      return;
    gizmoEnabled = enabled;
    if (!enabled)
    {
      updateSpline();
      generateData();
    }
  }

  void destroy() override { delete this; }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, ISplineEntity);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  BBox3 getBbox() const override { return sd ? sd->lbb : BBox3(); }
  BSphere3 getBsph() const override { return sd ? sd->lbs : BSphere3(); }

  const char *getObjAssetName() const override { return "n/a"; }

  // IRandomSeedHolder interface
  void setSeed(int new_seed) override { rndSeed = new_seed; }
  int getSeed() override { return rndSeed; }
  void setPerInstanceSeed(int seed) override { instSeed = seed; }
  int getPerInstanceSeed() override { return instSeed; }

  // ISplineEntity
  IObjEntity *createSplineEntityInstance() override
  {
    if (!sd)
      return nullptr;

    SplineEntity *e = new SplineEntity;
    e->sd = sd;
    e->rndSeed = sd->rndSeed;
    e->instSeed = sd->perInstSeed;
    e->ptGen.reserve(sd->ptAttr.size());

    DagorAsset *pa = nullptr;
    for (int i = 0; i < (int)sd->pt.size() - 1; i++)
      if (pa != sd->ptAttr[i].splineClass)
      {
        pa = sd->ptAttr[i].splineClass;
        if (IObjEntity *gen = pa ? DAEDITOR3.createEntity(*pa) : nullptr)
        {
          GenPart &gp = e->ptGen.push_back();
          gp.e = gen;
          gp.e->setEditLayerIdx(editLayerIdx);
          gp.startIdx = i;
          gp.endIdx = i + 1;
        }
      }
      else if (sd->ptAttr[i].splineClass)
        e->ptGen.back().endIdx++;

    // for (auto &gp : e->ptGen)
    //   debug("%p.genPart %p: %d,%d (%s)", e, gp.e, gp.startIdx, gp.endIdx, sd->ptAttr[gp.startIdx].splineClass->getName());
    return e;
  }
  virtual bool saveSplineTo(DataBlock &out_blk) const
  {
    if (!sd)
      return false;
    out_blk.clearData();
    return sd->save(*out_blk.addBlock(sd->isPoly ? "polygon" : "spline"), tm);
  }

public:
  TMatrix tm;
  int rndSeed = 123;
  int instSeed = 0;
  Ptr<SplineData> sd;
  Tab<GenPart> ptGen;
  BezierSpline3d bezierSpline;
  bool gizmoEnabled = false;
};
