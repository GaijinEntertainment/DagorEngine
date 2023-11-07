// #define _DEBUG_TAB_
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_entityUserData.h>
#include <de3_editorEvents.h>
#include <de3_entityCollision.h>
#include <de3_assetService.h>
#include <de3_csgEntity.h>
#include <de3_collisionPreview.h>
#include <de3_composit.h>
#include <de3_entityCollision.h>
#include <de3_randomSeed.h>
#include <de3_splineClassData.h>
#include <de3_genObjAlongSpline.h>
#include <assets/assetMgr.h>
#include <assets/assetChangeNotify.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_clipping.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/ObjCreator3d/objCreator3d.h>
#include <coolConsole/coolConsole.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <math/poly2tri/poly2tri.h>
#include <math/dag_math2d.h>
#include <math/random/dag_random.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <math/dag_TMatrix.h>
#include <generic/dag_carray.h>
#include <debug/dag_debug.h>
#include "csgApi.h"
#include "prefabCache.h"

template <class T>
struct DeletePtrOnScopeLeave
{
  T *obj;
  DeletePtrOnScopeLeave(T *&o) : obj(o) { o = NULL; }
  ~DeletePtrOnScopeLeave() { del_it(obj); }
};

static int csgEntityClassId = -1;
static int rendinstClassId = -1, prefabClassId = -1, compositClassId = -1;
static int rendEntGeomMask = -1;
static int collisionSubtypeMask = -1;
static int polyTileMask = -1;
static bool registeredNotifier = false;

class CsgEntity;
typedef SingleEntityPool<CsgEntity> CsgEntityPool;

class VirtualCsgEntity : public IObjEntity, public IObjEntityUserDataHolder
{
public:
  VirtualCsgEntity(int cls) : IObjEntity(cls), pool(NULL), userDataBlk(NULL), aMgr(NULL), assetNameId(-1) {}
  virtual ~VirtualCsgEntity() { clear(); }

  void clear() { del_it(userDataBlk); }
  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    return NULL;
  }

  virtual BSphere3 getBsph() const { return BSphere3(Point3(0, 0, 0), 1.0); }
  virtual BBox3 getBbox() const { return BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)); }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:fx", assetName());
    return buf;
  }
  const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }

  void setup(const DagorAsset &asset)
  {
    assetNameId = asset.getNameId();
    aMgr = &asset.getMgr();
  }

  bool isNonVirtual() const { return pool; }

  // IObjEntityUserDataHolder
  virtual DataBlock *getUserDataBlock(bool create_if_not_exist)
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  virtual void resetUserDataBlock() { del_it(userDataBlk); }

public:
  CsgEntityPool *pool;
  DataBlock *userDataBlk;
  DagorAssetMgr *aMgr;
  int assetNameId;
};

class CsgEntity : public VirtualCsgEntity, public IEntityCollisionState, public ICsgEntity, public IRandomSeedHolder
{
public:
  CsgEntity(int cls) : VirtualCsgEntity(cls), idx(MAX_ENTITIES), geom(NULL)
  {
    flags = 0;
    foundationPathClosed = false;
    tm.identity();
    rndSeed = 1;
  }
  ~CsgEntity()
  {
    clearEntites();
    del_it(geom);
  }


  virtual void setTm(const TMatrix &_tm)
  {
    tm = _tm;
    // if (geom)
    //   geom->setTm(tm);
  }
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
    clearEntites();
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IEntityCollisionState);
    RETURN_INTERFACE(huid, ICsgEntity);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return VirtualCsgEntity::queryInterfacePtr(huid);
  }
  virtual BSphere3 getBsph() const { return bsph; }
  virtual BBox3 getBbox() const { return bbox; }

  // IEntityCollisionState
  virtual void setCollisionFlag(bool f)
  {
    if (f)
      flags |= FLG_CLIP_EDITOR | FLG_CLIP_GAME;
    else
      flags &= ~(FLG_CLIP_EDITOR | FLG_CLIP_GAME);
  }
  virtual bool hasCollision() { return flags & FLG_CLIP_GAME; }
  virtual int getAssetNameId() { return assetNameId; }
  virtual DagorAsset *getAsset() { return aMgr->findAsset(assetName(), csgEntityClassId); }

  // IRandomSeedHolder interface
  virtual void setSeed(int new_seed)
  {
    rndSeed = new_seed;
    setFoundationPath(make_span(foundationPath), foundationPathClosed);
  }
  virtual int getSeed() { return rndSeed; }
  virtual void setPerInstanceSeed(int seed) {}
  virtual int getPerInstanceSeed() { return 0; }

  // ICsgEntity
  virtual void setFoundationPath(dag::Span<Point3> p, bool closed)
  {
    DagorAsset *a = aMgr->findAsset(assetName(), csgEntityClassId);
    DeletePtrOnScopeLeave<GeomObject> del_it_geom(geom);
    clearEntites();

    visRad = 0;
    bbox = bsph = BSphere3(Point3(0, 0, 0), 0.3);
    if (!a)
    {
      DAEDITOR3.conWarning("cannot find asset <%s> to use in CSG generation", assetName());
      return;
    }
    makeCountourCW(p);
    if (foundationPath.data() != p.data())
    {
      foundationPath = p;
      foundationPathClosed = closed;
    }

    if (!p.size())
    {
      DAEDITOR3.conWarning("bad path of %d points", p.size());
      return;
    }

    // prepare materials
    MaterialDataList *mdl = new MaterialDataList;
    carray<MaterialData *, 6> sm;
    mem_set_0(sm);
    if (IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>())
    {
      // sm[0] = NULL material
      sm[1] = assetSrv->getMaterialData(a->props.getStr("walls_mat", ""));
      sm[2] = assetSrv->getMaterialData(a->props.getStr("walls_inside_mat", a->props.getStr("walls_mat", "")));
      sm[3] = assetSrv->getMaterialData(a->props.getStr("floor_mat", ""));
      sm[4] = assetSrv->getMaterialData(a->props.getStr("ceiling_mat", ""));
      sm[5] = assetSrv->getMaterialData(a->props.getStr("roof_mat", ""));
    }
    for (int i = 0; i < sm.size(); i++)
      mdl->addSubMat(sm[i] ? sm[i] : new (tmpmem) MaterialData);

    // build CSG
    Mesh *m = NULL;
    float y0 = 0, h = 10;
    for (int i = 0; i < p.size(); i++)
      y0 += p[i].y;
    y0 /= p.size();

    StaticGeometryContainer genGeom;

    {
      int floor_cnt = a->props.getInt("floors", 1);
      float floor_ht_def = a->props.getReal("floorHeight", 2.5);
      float floor_thickness = a->props.getReal("floorThickness", 0.1);
      float foundation_ofs = a->props.getReal("foundationOffset", 0);
      float wall_thick = a->props.getReal("wallThickness", 0.25);
      float jointless_wall_m_out = a->props.getReal("jointlessWallMapping", 0.0f);
      float jointless_wall_m_in = a->props.getReal("jointlessWallMappingInside", jointless_wall_m_out);
      bool no_csg = a->props.getBool("skipCsg", false);
      Point2 wall_tc_mul = a->props.getPoint2("wall_tile_sz", Point2(1, 1));
      Point2 wall_inside_tc_mul = a->props.getPoint2("wall_inside_tile_sz", wall_tc_mul);
      Point2 floor_tc_mul = a->props.getPoint2("floor_tile_sz", Point2(2, 2));
      Point2 ceil_tc_mul = a->props.getPoint2("ceiling_tile_sz", Point2(4, 4));
#define TILE_SZ_TO_MUL(X) X = (fabsf(X) > 1e-6) ? 1.0f / (X) : 1.0f
      TILE_SZ_TO_MUL(wall_tc_mul.x);
      TILE_SZ_TO_MUL(wall_tc_mul.y);
      TILE_SZ_TO_MUL(wall_inside_tc_mul.x);
      TILE_SZ_TO_MUL(wall_inside_tc_mul.y);
      TILE_SZ_TO_MUL(floor_tc_mul.x);
      TILE_SZ_TO_MUL(floor_tc_mul.y);
      TILE_SZ_TO_MUL(ceil_tc_mul.x);
      TILE_SZ_TO_MUL(ceil_tc_mul.y);
#undef TILE_SZ_TO_MUL
      Point2 wall_tc_scale = a->props.getPoint2("wall_tc_scale", Point2(1, 1));
      Point2 wall_inside_tc_scale = a->props.getPoint2("wall_inside_tc_scale", wall_tc_scale);

      jointless_wall_m_out *= wall_tc_scale.x;
      jointless_wall_m_in *= wall_inside_tc_scale.x;
      wall_tc_mul = mul(wall_tc_mul, wall_tc_scale);
      wall_inside_tc_mul = mul(wall_inside_tc_mul, wall_inside_tc_scale);

      Tab<Point3> p_inner;
      shiftContour(p_inner, p, -wall_thick);
      if (p_inner.size() < 3)
      {
        DAEDITOR3.conWarning("bad inner path of %d points", p_inner.size());
        return;
      }
      Tab<Point2> inner_ofs;
      inner_ofs.resize(p.size());
      for (int i = 0; i < p.size(); i++)
      {
        Point3 dir = normalize(p[(i + 1) % p.size()] - p[i]);
        inner_ofs[i].set((p_inner[i] - p[i]) * dir, (p[(i + 1) % p.size()] - p_inner[(i + 1) % p.size()]) * dir);
      }

      Tab<float> floor_ht;
      float floor_ht_full = foundation_ofs + floor_thickness;
      floor_ht.resize(floor_cnt);
      for (int i = 0; i < floor_cnt; i++)
      {
        floor_ht[i] = a->props.getReal(String(0, "floorHeight_%d", i + 1), floor_ht_def);
        floor_ht_full += floor_ht[i];
      }

      Tab<int> p_out_tc_idx, sp_out_tc_idx, seg_out_tc_idx;
      Tab<int> p_in_tc_idx, sp_in_tc_idx, seg_in_tc_idx;
      readTcIdx(p_out_tc_idx, sp_out_tc_idx, seg_out_tc_idx, a->props, "wall");
      readTcIdx(p_in_tc_idx, sp_in_tc_idx, seg_in_tc_idx, a->props, "wall_inside");

      if (!p_out_tc_idx.size() && !sp_out_tc_idx.size() && !seg_out_tc_idx.size())
      {
        DAEDITOR3.conNote("wall_tc* not set, assume perimeter in tc0");
        p_out_tc_idx.push_back(0);
      }
      if (!p_in_tc_idx.size() && !sp_in_tc_idx.size() && !seg_in_tc_idx.size())
      {
        // DAEDITOR3.conNote("wallInside_tc* not set, assume the same as wall_tc");
        p_in_tc_idx = p_out_tc_idx;
        sp_in_tc_idx = sp_out_tc_idx;
        seg_in_tc_idx = seg_out_tc_idx;
      }
      int tc_count = 1;
#define GET_MAX_TC(X)                \
  for (int i = 0; i < X.size(); i++) \
    if (tc_count <= X[i])            \
      tc_count = X[i] + 1;
      GET_MAX_TC(p_out_tc_idx);
      GET_MAX_TC(sp_out_tc_idx);
      GET_MAX_TC(seg_out_tc_idx);
      GET_MAX_TC(p_in_tc_idx);
      GET_MAX_TC(sp_in_tc_idx);
      GET_MAX_TC(seg_in_tc_idx)
#undef GET_MAX_TC

      BasicCSG *csg = make_new_csg(tc_count);

      Mesh m_outer;
      genPrism(m_outer, p, y0, floor_ht_full, 1, 0, 5, wall_tc_mul, floor_tc_mul, ceil_tc_mul, jointless_wall_m_out, p_out_tc_idx,
        sp_out_tc_idx, seg_out_tc_idx);

      void *poly_A = csg->create_poly(NULL, m_outer, 0, true);
      if (!poly_A)
      {
        DAEDITOR3.conWarning("cannot make outer CSG object");
        del_it(csg);
        return;
      }

      Mesh m_inner;
      float inner_y_ofs = y0 + foundation_ofs + floor_thickness;
      genPrism(m_inner, p_inner, inner_y_ofs, floor_ht[0] - floor_thickness, 2, 3, 4, wall_inside_tc_mul, floor_tc_mul, ceil_tc_mul,
        jointless_wall_m_in, p_in_tc_idx, sp_in_tc_idx, seg_in_tc_idx);

      float main_ang = safe_atan2(-tm.getcol(2).z, -tm.getcol(2).x); //== to us in AV (south dir)
      for (int i = 0; i < floor_cnt; i++)
      {
        void *poly_B = csg->create_poly(NULL, m_inner, 0, true);
        if (!poly_B)
        {
          DAEDITOR3.conWarning("cannot make inner CSG object");
          csg->delete_poly(poly_A);
          del_it(csg);
          return;
        }

        void *result = csg->op(BasicCSG::A_MINUS_B, poly_A, NULL, poly_B, NULL);
        csg->delete_poly(poly_A);
        csg->delete_poly(poly_B);
        if (!result)
        {
          DAEDITOR3.conWarning("cannot subtract inner CSG object from outer one");
          del_it(csg);
          return;
        }
        poly_A = result;
        if (i + 1 < floor_cnt)
        {
          inner_y_ofs += floor_ht[i];
          if (floor_ht[i + 1] == floor_ht[i])
            for (int j = 0; j < m_inner.vert.size(); j++)
              m_inner.vert[j].y += floor_ht[i];
          else
            genPrism(m_inner, p_inner, inner_y_ofs, floor_ht[i + 1] - floor_thickness, 2, 3, 4, wall_inside_tc_mul, floor_tc_mul,
              ceil_tc_mul, jointless_wall_m_in, p_in_tc_idx, sp_in_tc_idx, seg_in_tc_idx);
        }
      }

      int nid = a->props.getNameId("objectRow");
      int seed = rndSeed;
      for (int i = 0; i < a->props.blockCount(); i++)
        if (a->props.getBlock(i)->getBlockNameId() == nid)
          generateObjects(no_csg ? NULL : csg, poly_A, *a->props.getBlock(i), p, inner_ofs, y0 + foundation_ofs + floor_thickness,
            floor_ht, wall_thick, entList, *mdl, seed, main_ang);

      // copy result to mesh
      m = new Mesh;
      csg->convert(poly_A, *m);
      csg->delete_poly(poly_A);
      delete_csg(csg);

      // remove invisible faces and null materials
      {
        Tab<bool> mat_null;
        mat_null.resize(mdl->list.size());
        mem_set_0(mat_null);
        int good_mat = -1;
        for (int mi = 0; mi < mat_null.size(); mi++)
          if (mdl->list[mi]->className.empty())
            mat_null[mi] = true;
          else if (good_mat < 0)
            good_mat = mi;
        if (good_mat >= 0)
          for (int mi = 0; mi < mat_null.size(); mi++)
            if (mat_null[mi])
              mdl->list[mi] = mdl->list[good_mat];

        for (int fi = m->face.size() - 1; fi >= 0; fi--)
          if (mat_null[m->face[fi].mat])
            m->removeFacesFast(fi, 1);
      }

      nid = a->props.getNameId("vertEdgeGen");
      for (int i = 0; i < a->props.blockCount(); i++)
        if (a->props.getBlock(i)->getBlockNameId() == nid)
          generateVertEdges(*a->props.getBlock(i), p, y0 + foundation_ofs + floor_thickness, floor_ht, genGeom, entList,
            getPerInstanceSeed(), seed, main_ang, editLayerIdx);

      nid = a->props.getNameId("horEdgeGen");
      for (int i = 0; i < a->props.blockCount(); i++)
        if (a->props.getBlock(i)->getBlockNameId() == nid)
          generateHorEdges(*a->props.getBlock(i), p, y0 + foundation_ofs + floor_thickness, floor_ht, genGeom, entList,
            getPerInstanceSeed(), seed, main_ang, editLayerIdx);

      nid = a->props.getNameId("perimeterGen");
      for (int i = 0; i < a->props.blockCount(); i++)
        if (a->props.getBlock(i)->getBlockNameId() == nid)
          generatePerimeterGeom(*a->props.getBlock(i), p, y0 + foundation_ofs + floor_thickness, floor_ht, genGeom, entList,
            getPerInstanceSeed(), seed, main_ang, editLayerIdx);
    }

    geom = new (midmem) GeomObject;

    m->calc_ngr();
    m->calc_vertnorms();
    for (int i = 0; i < m->vertnorm.size(); ++i)
      m->vertnorm[i] = -m->vertnorm[i];
    for (int i = 0; i < m->facenorm.size(); ++i)
      m->facenorm[i] = -m->facenorm[i];

    ObjCreator3d::addNode(String(128, "csg_%s", assetName()), m, mdl, *geom->getGeometryContainer());
    G_ASSERT(geom->getGeometryContainer()->nodes.size() == 1);
    if (StaticGeometryNode *node = geom->getGeometryContainer()->nodes[0])
    {
      node->flags |= StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS | StaticGeometryNode::FLG_CASTSHADOWS;
      node->calcBoundBox();
      node->calcBoundSphere();
      node->script.setBool("collidable", a->props.getBool("collidable", true));
      node->script.setStr("collision", a->props.getStr("collType", "mesh"));
    }
    for (int i = 0; i < genGeom.nodes.size(); ++i)
      geom->getGeometryContainer()->addNode(new StaticGeometryNode(*genGeom.nodes[i]));

    geom->setTm(TMatrix::IDENT);

    geom->notChangeVertexColors(true);
    geom->recompile();
    geom->notChangeVertexColors(false);

    bbox = geom->getBoundBox(true);
    bsph = bbox;
    visRad = bsph.r + length(bsph.c);

    collision.clear();
    collisionpreview::assembleCollisionFromNodes(collision, geom->getGeometryContainer()->nodes);
  }

  static bool isCountourCW(dag::ConstSpan<Point3> p)
  {
    if (p.size() < 3)
      return true;
    float sum = (p[0].x - p.back().x) * (p[0].z + p.back().z);
    for (int i = 1; i < p.size(); i++)
      sum += (p[i].x - p[i - 1].x) * (p[i].z + p[i - 1].z);
    return sum > 0;
  }

  static void makeCountourCW(dag::Span<Point3> p)
  {
    if (isCountourCW(p))
      return;
    for (int i = 0; i < p.size() / 2; i++)
    {
      Point3 tmp = p[i];
      p[i] = p[p.size() - 1 - i];
      p[p.size() - 1 - i] = tmp;
    }
  }

  static void shiftContour(Tab<Point3> &out_c, dag::ConstSpan<Point3> p, float shift)
  {
    Tab<Point2> c;
    c.resize(p.size() * 2);
    for (int i = 0; i < p.size(); i++)
    {
      Point2 dir(p[(i + 1) % p.size()].x - p[i].x, p[(i + 1) % p.size()].z - p[i].z);
      Point2 n(-dir.y, dir.x);
      n.normalize();
      c[i * 2 + 0] = Point2::xz(p[i]) + n * shift;
      c[i * 2 + 1] = Point2::xz(p[(i + 1) % p.size()]) + n * shift;
    }

    Point2 pt;
    static const float ML = 10;
    static const float MLp1 = ML + 1;
    for (int i = 0; i < c.size(); i += 2)
      if (get_lines_intersection(c[i + 0], MLp1 * c[i + 1] - ML * c[i + 0], MLp1 * c[(i + 2) % c.size()] - ML * c[(i + 3) % c.size()],
            c[(i + 3) % c.size()], &pt))
        c[i + 1] = c[(i + 2) % c.size()] = pt;

    out_c.resize(p.size());
    out_c[0].set_x0y((c.back() + c.front()) * 0.5);
    for (int i = 0; i + 2 < c.size(); i += 2)
      out_c[i / 2 + 1].set_x0y((c[i + 1] + c[i + 2]) * 0.5);
  }

  static void readTcIdx(Tab<int> &p_tc_idx, Tab<int> &sp_tc_idx, Tab<int> &seg_tc_idx, const DataBlock &b, const char *pname_base)
  {
    for (int ci = 0; ci < NUMMESHTCH; ++ci)
      if (const char *tc_type = b.getStr(String(0, "%s_tc%d", pname_base, ci), NULL))
      {
        if (strcmp(tc_type, "perimeter") == 0)
          p_tc_idx.push_back(ci);
        else if (strcmp(tc_type, "splatting") == 0)
          sp_tc_idx.push_back(ci);
        else if (strcmp(tc_type, "segment") == 0)
          seg_tc_idx.push_back(ci);
        else
          DAEDITOR3.conError("bad %s:t=\"%s\"", String(0, "%s_tc%d", pname_base, ci), tc_type);
      }
  }

  static bool genPrism(Mesh &m, dag::ConstSpan<Point3> p, float y0, float h, int mat_wall, int mat_floor, int mat_ceil,
    const Point2 &_tc_scale_wall, const Point2 &tc_scale_floor, const Point2 &tc_scale_ceil, float jointless_wall_mapping,
    dag::ConstSpan<int> p_tc_idx, dag::ConstSpan<int> sp_tc_idx, dag::ConstSpan<int> seg_tc_idx)
  {
    Point2 tc_scale_wall = _tc_scale_wall;
    Tab<p2t::Point> in_pt;
    std::vector<p2t::Point *> polyline;
    in_pt.resize(p.size());
    for (int i = 0; i < p.size(); i++)
    {
      in_pt[i].x = p[i].x, in_pt[i].y = p[i].z;
      polyline.push_back(&in_pt[i]);
    }

    p2t::CDT cdt(polyline);
    cdt.Triangulate();
    for (int i = 0; i < cdt.GetTriangles().size(); i++)
      for (int j = 0; j < 3; j++)
        if (cdt.GetTriangles()[i]->GetPoint(j) < in_pt.data() || cdt.GetTriangles()[i]->GetPoint(j) >= in_pt.end())
        {
          logerr("bad point=%p in tri[%d][%d], in_pt=%p..%p", cdt.GetTriangles()[i]->GetPoint(j), i, j, in_pt.data(), in_pt.end() - 1);
          return false;
        }

    Tab<TFace> tri(tmpmem);
    tri.resize(cdt.GetTriangles().size());
    for (int i = 0; i < cdt.GetTriangles().size(); i++)
      tri[i].t[0] = cdt.GetTriangles()[i]->GetPoint(0) - in_pt.data(), tri[i].t[1] = cdt.GetTriangles()[i]->GetPoint(1) - in_pt.data(),
      tri[i].t[2] = cdt.GetTriangles()[i]->GetPoint(2) - in_pt.data();

    int c2_vofs = p.size();
    int c2_fofs = tri.size();
    int w_fofs = tri.size() * 2;
    m.vert.resize(p.size() * 2);
    m.face.resize(tri.size() * 2 + p.size() * 2);
    mem_set_0(m.face);

#define P_V m.tvert[p_tc_idx[c]]
#define P_F m.tface[p_tc_idx[c]]
#define S_V m.tvert[sp_tc_idx[c]]
#define S_F m.tface[sp_tc_idx[c]]
#define L_V m.tvert[seg_tc_idx[c]]
#define L_F m.tface[seg_tc_idx[c]]

    for (int c = 0; c < p_tc_idx.size(); c++)
    {
      P_V.resize(p.size() * 2 + (p.size() + 1) * 2);
      P_F.resize(m.face.size());
      mem_set_0(P_F);
    }
    for (int c = 0; c < sp_tc_idx.size(); c++)
    {
      S_V.resize(p.size() * 2 + (p.size() + 1) * 2);
      S_F.resize(m.face.size());
      mem_set_0(S_F);
    }
    for (int c = 0; c < seg_tc_idx.size(); c++)
    {
      L_V.resize(p.size() * 2 + (p.size() + 1) * 2);
      L_F.resize(m.face.size());
      mem_set_0(L_F);
    }

    float plen = 0;
    for (int i = 0; i < p.size(); i++)
    {
      plen += length(Point2::xz(p[i]) - Point2::xz(p[(i + 1) % p.size()]));
      m.vert[i].set_xVz(p[i], y0);
      m.vert[c2_vofs + i].set_xVz(p[i], y0 + h);
      Point2 tv0(m.vert[i].x * tc_scale_floor.x, m.vert[i].z * tc_scale_floor.y);
      Point2 tv1(m.vert[i].x * tc_scale_ceil.x, m.vert[i].z * tc_scale_ceil.y);
      for (int c = 0; c < p_tc_idx.size(); c++)
      {
        P_V[i] = tv0;
        P_V[c2_vofs + i] = tv1;
      }
      for (int c = 0; c < sp_tc_idx.size(); c++)
      {
        S_V[i] = tv0;
        S_V[c2_vofs + i] = tv1;
      }
      for (int c = 0; c < seg_tc_idx.size(); c++)
      {
        L_V[i] = tv0;
        L_V[c2_vofs + i] = tv1;
      }
    }
    if (jointless_wall_mapping > 0 && plen > 0)
    {
      float pu = floorf(plen * tc_scale_wall.x / jointless_wall_mapping + 0.5f);
      if (pu < 1)
        pu = 1;
      tc_scale_wall.x = pu / plen * jointless_wall_mapping;
    }

    for (int i = 0; i < tri.size(); i++)
    {
      m.face[i].mat = mat_ceil;
      m.face[i].smgr = 8;
      m.face[i].v[0] = c2_vofs + tri[i].t[0];
      m.face[i].v[1] = c2_vofs + tri[i].t[2];
      m.face[i].v[2] = c2_vofs + tri[i].t[1];
      for (int c = 0; c < p_tc_idx.size(); c++)
      {
        P_F[i].t[0] = c2_vofs + tri[i].t[0];
        P_F[i].t[1] = c2_vofs + tri[i].t[2];
        P_F[i].t[2] = c2_vofs + tri[i].t[1];
      }
      for (int c = 0; c < sp_tc_idx.size(); c++)
      {
        S_F[i].t[0] = c2_vofs + tri[i].t[0];
        S_F[i].t[1] = c2_vofs + tri[i].t[2];
        S_F[i].t[2] = c2_vofs + tri[i].t[1];
      }
      for (int c = 0; c < seg_tc_idx.size(); c++)
      {
        L_F[i].t[0] = c2_vofs + tri[i].t[0];
        L_F[i].t[1] = c2_vofs + tri[i].t[2];
        L_F[i].t[2] = c2_vofs + tri[i].t[1];
      }

      m.face[c2_fofs + i].mat = mat_floor;
      m.face[c2_fofs + i].smgr = 16;
      m.face[c2_fofs + i].v[0] = tri[i].t[0];
      m.face[c2_fofs + i].v[1] = tri[i].t[1];
      m.face[c2_fofs + i].v[2] = tri[i].t[2];
      for (int c = 0; c < p_tc_idx.size(); c++)
      {
        P_F[c2_fofs + i].t[0] = tri[i].t[0];
        P_F[c2_fofs + i].t[1] = tri[i].t[1];
        P_F[c2_fofs + i].t[2] = tri[i].t[2];
      }
      for (int c = 0; c < sp_tc_idx.size(); c++)
      {
        S_F[c2_fofs + i].t[0] = tri[i].t[0];
        S_F[c2_fofs + i].t[1] = tri[i].t[1];
        S_F[c2_fofs + i].t[2] = tri[i].t[2];
      }
      for (int c = 0; c < seg_tc_idx.size(); c++)
      {
        L_F[c2_fofs + i].t[0] = tri[i].t[0];
        L_F[c2_fofs + i].t[1] = tri[i].t[1];
        L_F[c2_fofs + i].t[2] = tri[i].t[2];
      }
    }

    float path_len = 0;
    for (int i = 0; i < p.size(); i++)
    {
      int smgr = i ? 2 << (i & 1) : 1;
      m.face[w_fofs + i * 2 + 0].mat = mat_wall;
      m.face[w_fofs + i * 2 + 0].smgr = smgr;
      m.face[w_fofs + i * 2 + 0].v[0] = i + 0;
      m.face[w_fofs + i * 2 + 0].v[1] = (i + 1) % p.size();
      m.face[w_fofs + i * 2 + 0].v[2] = i + c2_vofs;
      m.face[w_fofs + i * 2 + 1].mat = mat_wall;
      m.face[w_fofs + i * 2 + 1].smgr = smgr;
      m.face[w_fofs + i * 2 + 1].v[0] = i + c2_vofs;
      m.face[w_fofs + i * 2 + 1].v[1] = (i + 1) % p.size();
      m.face[w_fofs + i * 2 + 1].v[2] = (i + 1) % p.size() + c2_vofs;

      if (i > 0)
        path_len += (Point2::xz(m.vert[i]) - Point2::xz(m.vert[i - 1])).length();
      for (int c = 0; c < p_tc_idx.size(); c++)
      {
        P_V[c2_vofs * 2 + i * 2 + 0].set(path_len * tc_scale_wall.x, h * tc_scale_wall.y);
        P_V[c2_vofs * 2 + i * 2 + 1].set(path_len * tc_scale_wall.x, 0 * tc_scale_wall.y);

        P_F[w_fofs + i * 2 + 0].t[0] = c2_vofs * 2 + i * 2 + 0;
        P_F[w_fofs + i * 2 + 0].t[1] = c2_vofs * 2 + i * 2 + 2;
        P_F[w_fofs + i * 2 + 0].t[2] = c2_vofs * 2 + i * 2 + 1;
        P_F[w_fofs + i * 2 + 1].t[0] = c2_vofs * 2 + i * 2 + 1;
        P_F[w_fofs + i * 2 + 1].t[1] = c2_vofs * 2 + i * 2 + 2;
        P_F[w_fofs + i * 2 + 1].t[2] = c2_vofs * 2 + i * 2 + 3;
      }
      for (int c = 0; c < sp_tc_idx.size(); c++)
      {
        S_V[c2_vofs * 2 + i * 2 + 0].set(i & 1, 1);
        S_V[c2_vofs * 2 + i * 2 + 1].set(i & 1, 0);

        S_F[w_fofs + i * 2 + 0].t[0] = c2_vofs * 2 + i * 2 + 0;
        S_F[w_fofs + i * 2 + 0].t[1] = c2_vofs * 2 + i * 2 + 2;
        S_F[w_fofs + i * 2 + 0].t[2] = c2_vofs * 2 + i * 2 + 1;
        S_F[w_fofs + i * 2 + 1].t[0] = c2_vofs * 2 + i * 2 + 1;
        S_F[w_fofs + i * 2 + 1].t[1] = c2_vofs * 2 + i * 2 + 2;
        S_F[w_fofs + i * 2 + 1].t[2] = c2_vofs * 2 + i * 2 + 3;
      }
      for (int c = 0; c < seg_tc_idx.size(); c++)
      {
        L_V[c2_vofs * 2 + i * 2 + 0].set(i, 1);
        L_V[c2_vofs * 2 + i * 2 + 1].set(i, 0);

        L_F[w_fofs + i * 2 + 0].t[0] = c2_vofs * 2 + i * 2 + 0;
        L_F[w_fofs + i * 2 + 0].t[1] = c2_vofs * 2 + i * 2 + 2;
        L_F[w_fofs + i * 2 + 0].t[2] = c2_vofs * 2 + i * 2 + 1;
        L_F[w_fofs + i * 2 + 1].t[0] = c2_vofs * 2 + i * 2 + 1;
        L_F[w_fofs + i * 2 + 1].t[1] = c2_vofs * 2 + i * 2 + 2;
        L_F[w_fofs + i * 2 + 1].t[2] = c2_vofs * 2 + i * 2 + 3;
      }
    }
    path_len += (Point2::xz(m.vert[0]) - Point2::xz(m.vert.back())).length();
    for (int c = 0; c < p_tc_idx.size(); c++)
    {
      P_V[c2_vofs * 2 + p.size() * 2 + 0].set(path_len * tc_scale_wall.x, h * tc_scale_wall.y);
      P_V[c2_vofs * 2 + p.size() * 2 + 1].set(path_len * tc_scale_wall.x, 0 * tc_scale_wall.y);
    }
    for (int c = 0; c < sp_tc_idx.size(); c++)
    {
      S_V[c2_vofs * 2 + p.size() * 2 + 0].set(p.size() & 1, 1);
      S_V[c2_vofs * 2 + p.size() * 2 + 1].set(p.size() & 1, 0);
    }
    for (int c = 0; c < seg_tc_idx.size(); c++)
    {
      L_V[c2_vofs * 2 + p.size() * 2 + 0].set(p.size(), 1);
      L_V[c2_vofs * 2 + p.size() * 2 + 1].set(p.size(), 0);
    }

    for (int ei = NUMMESHTCH - 1; ei >= 0; ei--)
      if (m.tface[ei].size())
      {
        // fill empty TC with stub vertex to avoid crash in CSG
        for (int i = 0; i < ei; i++)
          if (!m.tface[i].size())
          {
            m.tvert[i].resize(1);
            mem_set_0(m.tvert[i]);
            m.tface[i].resize(m.face.size());
            mem_set_0(m.tface[i]);
          }
        break;
      }
    /*
    debug("mesh %d vert, %d faces, cap_v_ofs=%d cap_f_ofs=%d wall_f_ofs=%d (poly=%d)",
      m.vert.size(), m.face.size(), c2_vofs, c2_fofs, w_fofs, p.size());
    for (int i = 0; i < m.vert.size(); i ++)
      debug("  v[%d]=%@", i, m.vert[i]);
    for (int i = 0; i < m.face.size(); i ++)
      debug("  f[%d]=%d,%d,%d", i, m.face[i].v[0], m.face[i].v[1], m.face[i].v[2]);
    for (int j = 0; j < NUMMESHTCH; j ++)
    {
      if (!m.tface[j].size())
        continue;
      for (int i = 0; i < m.tvert[j].size(); i ++)
        debug("  tv%d[%d]=%@", j, i, m.tvert[j][i]);
      for (int i = 0; i < m.tface[j].size(); i ++)
        debug("  tf%d[%d]=%d,%d,%d", j, i, m.tface[j][i].t[0], m.tface[j][i].t[1], m.tface[j][i].t[2]);
    }
    */
    m.kill_unused_verts(1e-6);
    return true;

#undef P_V
#undef P_F
#undef S_V
#undef S_F
#undef L_V
#undef L_F
  }


  struct GenObjFloorDesc
  {
    struct Obj
    {
      SimpleString name;
      IObjEntity *ent;
      Point3 ofs;
      float wt;
      int stepWd;
      float stepOfs;

      Obj() : ent(NULL), wt(1), stepOfs(0), stepWd(1) {}
      ~Obj() { destroy_it(ent); }
    };
    enum
    {
      SELTYPE_RANDOM,
      SELTYPE_ONCE_SEG,
      SELTYPE_ONCE
    };
    Tab<Obj> objs;
    int selType;
    int selObj;
    char sym;

    int stepW;
    int mpatLen;
    SimpleString mpat;

    void init(char _sym, const DataBlock *fblk, const DataBlock &cblk)
    {
      sym = _sym;
      selType = SELTYPE_RANDOM;
      selObj = -1;
      if (!fblk)
        fblk = &cblk;
      if (const char *sel_type_str = fblk->getStr("selectType", cblk.getStr("selectType", NULL)))
      {
        if (strcmp(sel_type_str, "random") == 0)
          selType = SELTYPE_RANDOM;
        else if (strcmp(sel_type_str, "once") == 0)
          selType = SELTYPE_ONCE;
        else if (strcmp(sel_type_str, "oncePerSeg") == 0)
          selType = SELTYPE_ONCE_SEG;
      }

      stepW = cblk.getInt("steps", 1);
      mpatLen = 0;
      if (stepW <= 0)
        stepW = 1;
      if (stepW > 1)
      {
        mpat = fblk->getStr("genPattern", "*");
        mpatLen = i_strlen(mpat);
        if (!mpatLen)
          mpat = " ", mpatLen = 1;
      }

      float wt = 0;
      int nid = cblk.getNameId("obj");
      objs.reserve(fblk->blockCount());
      for (int i = 0, ie = fblk->blockCount(); i < ie; i++)
        if (fblk->getBlock(i)->getBlockNameId() == nid)
        {
          Obj &o = objs.push_back();
          o.name = fblk->getBlock(i)->getStr("nm", NULL);
          o.wt = fblk->getBlock(i)->getReal("wt", 1);
          o.ofs = fblk->getBlock(i)->getPoint3("ofs", Point3(0, 0, 0));
          o.stepWd = fblk->getBlock(i)->getInt("stepWd", 1);
          if (o.stepWd <= 0)
            o.stepWd = 1;
          if (o.stepWd > stepW)
            o.stepWd = stepW;

          o.stepOfs = fblk->getBlock(i)->getReal("stepOfs", (o.stepWd - 1) / 2.0f);
          if (o.stepOfs < 0 || o.stepOfs > o.stepWd - 1)
            o.stepOfs = o.stepWd - 1;
          wt += o.wt;
        }
      objs.shrink_to_fit();
      if (wt > 0)
        for (int i = 0; i < objs.size(); i++)
          objs[i].wt /= wt;
    }

    IObjEntity *selectEntity(float rnd, Point3 &out_ofs, int o_ord, int max_w, float &out_pt_ofs, int &out_w)
    {
      if (!objs.size())
        return NULL;
      if (selType == SELTYPE_RANDOM || selObj < 0)
      {
        rnd = clamp(rnd, 0.f, 1.f);
        selObj = 0;
        while (selObj < 2 * objs.size())
          if (rnd < objs[selObj % objs.size()].wt && objs[selObj % objs.size()].stepWd <= max_w)
            break;
          else
          {
            rnd -= objs[selObj % objs.size()].wt;
            selObj++;
          }
      }
      out_pt_ofs = objs[selObj].stepOfs;
      out_w = objs[selObj].stepWd;
      if (objs[selObj].name.empty() || out_w > max_w || (mpatLen && mpat[o_ord % mpatLen] != '*'))
        return NULL;
      if (!objs[selObj].ent)
      {
        DagorAsset *ent_a = DAEDITOR3.getAssetByName(objs[selObj].name);
        objs[selObj].ent = ent_a ? DAEDITOR3.createEntity(*ent_a) : NULL;
        if (objs[selObj].ent)
        {
          objs[selObj].ent->setTm(TMatrix::IDENT);
          objs[selObj].ent->setSubtype(polyTileMask);
        }
        else
        {
          DAEDITOR3.conError("failed to create entity %s, asset=%p", objs[selObj].name, ent_a);
          objs[selObj].name = NULL;
        }
      }
      out_ofs = objs[selObj].ofs;
      return objs[selObj].ent;
    }
  };
  struct GenObjDesc
  {
    Tab<GenObjFloorDesc> desc;
    Tab<uint8_t> pattern, pat0, patM, pat1;
    Tab<int> maxCnt;
    int floorCnt;
    bool symmPat, overlapPat, wholePat, mirrorPat;
    Tab<Point2> angRange;

    GenObjDesc(int fcnt, const DataBlock &blk) : desc(tmpmem), floorCnt(fcnt)
    {
      initPattern(patM, blk.getStr("pattern", ""), blk);
      initPattern(pat0, blk.getStr("pattern0", ""), blk);
      initPattern(pat1, blk.getStr("pattern1", ""), blk);
      desc.shrink_to_fit();
      symmPat = blk.getBool("symmPattern", false);
      overlapPat = blk.getBool("overlapPattern", false);
      wholePat = blk.getBool("wholePattern", false);
      mirrorPat = blk.getBool("mirrorPattern", false);

      int maxCount = blk.getInt("maxCount", 128 << 10);
      maxCnt.resize(floorCnt);
      for (int i = 0; i < maxCnt.size(); i++)
        maxCnt[i] = blk.getInt(String(0, "maxCount_f%d", i + 1), maxCount);

      int nid = blk.getNameId("angRange");
      for (int i = 0; i < blk.paramCount(); i++)
        if (blk.getParamNameId(i) == nid && blk.getParamType(i) == blk.TYPE_POINT2)
          angRange.push_back(blk.getPoint2(i) * DEG_TO_RAD);
    }
    void initPattern(Tab<uint8_t> &pat, const char *patstr, const DataBlock &blk)
    {
      pat.resize(strlen(patstr));
      mem_set_ff(pat);
      desc.reserve(floorCnt * pat.size());
      for (int i = 0; i < pat.size(); i++)
      {
        for (int j = 0; j < desc.size(); j += floorCnt)
          if (desc[j].sym == patstr[i])
          {
            pat[i] = j / floorCnt;
            break;
          }
        if (pat[i] == 0xFF)
        {
          const DataBlock *b = blk.getBlockByName(String(0, "%c", patstr[i]));
          if (!b)
            continue;
          pat[i] = desc.size() / floorCnt;
          for (int k = 0; k < floorCnt; k++)
            desc.push_back().init(patstr[i], b->getBlockByName(String(0, "floor_%d", k + 1)), *b);
        }
      }
    }

    int getItemWidth(int ord)
    {
      if (!pattern.size())
        return 1;
      int p = pattern[ord % pattern.size()];
      if (p == 0xFF)
        return 1;
      return desc[p * floorCnt].stepW;
    }
    IObjEntity *selectEntity(int f, int ord, int &seed, Point3 &out_ofs, int o_ord, int max_w, float &out_pt_ofs, int &out_w)
    {
      out_pt_ofs = 0;
      out_w = 1;
      if (!pattern.size() || f < 0 || f >= floorCnt)
        return NULL;
      int p = pattern[ord % pattern.size()];
      if (p == 0xFF)
        return NULL;
      return desc[p * floorCnt + f].selectEntity(_frnd(seed), out_ofs, o_ord, max_w, out_pt_ofs, out_w);
    }
    bool onNewSeg(float ang)
    {
      for (int i = 0; i < desc.size(); i++)
        if (desc[i].selType == desc[i].SELTYPE_ONCE_SEG)
          desc[i].selObj = -1;
      if (!angRange.size())
        return true;

      ang = norm_s_ang(ang);
      for (int i = 0; i < angRange.size(); i++)
        if ((ang > angRange[i].x && ang < angRange[i].y) || (ang > angRange[i].x + 2 * PI && ang < angRange[i].y + 2 * PI))
          return true;
      return false;
    }

    int generateFullPattern(int steps)
    {
      pattern.clear();
      steps -= getPatternWidth(pat0);
      if (steps <= 0)
        return 0;
      steps -= getPatternWidth(pat1);
      if (steps <= 0)
        return 0;

      pattern = pat0;

      if (patM.size())
      {
        int main_w = getPatternWidth(patM), overlap = 0;
        bool mirror_dir = true;
        while (steps + overlap >= main_w)
        {
          steps -= main_w - overlap;
          if (overlap)
            pattern.pop_back();
          else if (overlapPat)
            overlap = getGroupWidth(patM.back());

          if (mirror_dir)
            append_items(pattern, patM.size(), patM.data());
          else
          {
            pattern.reserve(patM.size());
            for (int i = patM.size() - 1; i >= 0; i--)
              pattern.push_back(patM[i]);
          }
          if (mirrorPat)
            mirror_dir = !mirror_dir;
        }

        if (wholePat && symmPat && !mirror_dir)
        {
          int pop_last = patM.size() - ((overlap && pattern.size() > pat0.size() + patM.size()) ? 1 : 0);
          erase_items(pattern, pattern.size() - pop_last, pop_last);
        }

        if (!wholePat && (steps > 0 || !mirror_dir))
        {
          if (patM.size() < 2)
            ; // do nothing
          else if (symmPat)
          {
            if (!mirrorPat || mirror_dir)
            {
              if (overlap)
                pattern.pop_back();
              append_items(pattern, patM.size(), patM.data());
              steps -= main_w - overlap;
              if (mirrorPat)
                mirror_dir = !mirror_dir;
            }
            if (mirrorPat && !mirror_dir)
            {
              if (overlap)
                pattern.pop_back();
              pattern.reserve(patM.size());
              for (int i = patM.size() - 1; i >= 0; i--)
                pattern.push_back(patM[i]);
              steps -= main_w - overlap;
            }

            while (steps < 0 && pattern.size() >= pat0.size() + pat1.size() + 2)
            {
              steps += getGroupWidth(pattern[pat0.size()]) + getGroupWidth(pattern.back());
              pattern.pop_back();
              erase_items(pattern, pat0.size(), 1);
            }
          }
          else
            for (int i = 0; i < patM.size(); i++)
            {
              uint8_t c = patM[mirror_dir ? i : patM.size() - i - 1];
              if (getGroupWidth(c) <= steps)
              {
                pattern.push_back(c);
                steps -= getGroupWidth(c);
              }
              else
                break;
            }
        }
      }

      append_items(pattern, pat1.size(), pat1.data());
      return getPatternWidth(pattern);
    }
    int getGroupWidth(uint8_t p) const { return p != 0xFF ? desc[p * floorCnt].stepW : 0; }
    int getPatternWidth(dag::ConstSpan<uint8_t> pat) const
    {
      int w = 0;
      for (int i = 0; i < pat.size(); i++)
        w += getGroupWidth(pat[i]);
      return w;
    }
  };

  static void generateObjects(BasicCSG *csg, void *&poly_A, const DataBlock &b, dag::ConstSpan<Point3> p,
    dag::ConstSpan<Point2> inner_ofs, float y0, dag::ConstSpan<float> floor_ht, float wall_thick, Tab<IObjEntity *> &out_obj,
    MaterialDataList &mdl, int &seed, float main_ang)
  {
#define FLOAT_PROP(X, DEF) float X = b.getReal(#X, DEF)
#define BOOL_PROP(X, DEF)  bool X = b.getBool(#X, DEF)
    FLOAT_PROP(minOuterOfs, wall_thick + 0.1);
    FLOAT_PROP(minInnerOfs, 0.1);
    BOOL_PROP(symmOfs, true);
    FLOAT_PROP(minStep, 1);
    FLOAT_PROP(maxStep, 100);
    FLOAT_PROP(floorOfs, 0);
    FLOAT_PROP(outOfs, 0);
#undef FLOAT_PROP
#undef BOOL_PROP
    int align = -1;
    if (const char *s = b.getStr("alignment", "start"))
    {
      if (strcmp(s, "start") == 0)
        align = -1;
      else if (strcmp(s, "center") == 0)
        align = 0;
      else if (strcmp(s, "end") == 0)
        align = 1;
      else
        DAEDITOR3.conError("bad %s:t=\"%s\", using \"start\"", "alignment", s);
    }

    GenObjDesc go_desc(floor_ht.size(), b);
    Tab<TMatrix> obj_tm;
    for (int i = 0; i < p.size(); i++)
    {
      Point3 dir(p[(i + 1) % p.size()] - p[i]);
      if (!go_desc.onNewSeg(safe_atan2(dir.x, -dir.z) - main_ang))
        continue;
      generateObjectsPos(obj_tm, p[i], p[(i + 1) % p.size()], max(minOuterOfs, inner_ofs[i][0] + minInnerOfs),
        max(minOuterOfs, inner_ofs[i][1] + minInnerOfs), symmOfs, align, minStep, maxStep, y0 + floorOfs, outOfs, go_desc);
      for (int j = 0, pos = 0, pt_cnt = 0; pos < obj_tm.size(); j++, pos += pt_cnt)
      {
        float ht_ofs = 0;
        pt_cnt = go_desc.getItemWidth(j);
        if (pt_cnt + pos > obj_tm.size())
          pt_cnt = obj_tm.size() - pos;
        for (int k = 0; k < floor_ht.size(); k++)
        {
          if (j >= go_desc.maxCnt[k])
          {
            _rnd(seed);
            continue;
          }
          Point3 ofs;
          float o_ofs = 0;
          for (int m = 0, o_pt_cnt = 0, o_ord = 0; m < pt_cnt; m += o_pt_cnt, o_ord++)
            if (IObjEntity *entity = go_desc.selectEntity(k, j, seed, ofs, o_ord, pt_cnt - m, o_ofs, o_pt_cnt))
            {
              TMatrix tm = obj_tm[pos + m];
              Point3 dir(0, 0, 0);
              if (pos + m > 0)
                dir = tm.getcol(3) - obj_tm[pos + m - 1].getcol(3);
              else if (pos + m + 1 < obj_tm.size())
                dir = obj_tm[pos + m + 1].getcol(3) - tm.getcol(3);

              tm[3][1] += ht_ofs;
              tm.setcol(3, tm * ofs + dir * o_ofs);
              placeEntity(csg, poly_A, out_obj, mdl, entity, tm);
            }

          ht_ofs += floor_ht[k];
        }
      }
    }
  }

  static void placeEntity(BasicCSG *csg, void *&poly_A, Tab<IObjEntity *> &out_obj, MaterialDataList &mdl, IObjEntity *entity,
    const TMatrix &tm)
  {
    if (ICompositObj *co = entity->queryInterface<ICompositObj>())
    {
      TMatrix e_tm;
      for (int i = 0, ie = co->getCompositSubEntityCount(); i < ie; i++)
        if (IObjEntity *ce = co->getCompositSubEntity(i))
        {
          if (isPrefabForCSG(ce))
          {
            ce->getTm(e_tm);
            subtractPrefab(csg, poly_A, mdl, ce, tm * e_tm);
          }
          else if (IObjEntity *e = DAEDITOR3.cloneEntity(ce))
          {
            ce->getTm(e_tm);
            e->setTm(tm * e_tm);
            out_obj.push_back(e);
          }
        }
    }
    else if (isPrefabForCSG(entity))
    {
      subtractPrefab(csg, poly_A, mdl, entity, tm);
    }
    else if (IObjEntity *e = DAEDITOR3.cloneEntity(entity))
    {
      e->setTm(tm);
      out_obj.push_back(e);
    }
  }

  static bool isPrefabForCSG(IObjEntity *e)
  {
    if (e->getAssetTypeId() != prefabClassId)
      return false;
    if (IEntityCollisionState *ecs = e->queryInterface<IEntityCollisionState>())
      return ecs->getAsset()->props.getBool("csgCut", false);
    return false;
  }
  static void subtractPrefab(BasicCSG *csg, void *&poly_A, MaterialDataList &mdl, IObjEntity *entity, const TMatrix &tm)
  {
    if (!csg)
      return;
    IEntityCollisionState *ecs = entity->queryInterface<IEntityCollisionState>();
    if (!ecs)
      return;

    PrefabGeomCacheRec *prefab = PrefabGeomCacheRec::getCache(ecs->getAssetNameId());
    if (!prefab)
      if (!(prefab = PrefabGeomCacheRec::fetchCache(ecs->getAsset())))
        return;

    Tab<int> mat_remap;
    prefab->addMaterials(mdl, mat_remap);
    for (int i = 0; i < prefab->mesh.size(); i++)
    {
      Mesh m = *prefab->mesh[i];
      m.transform(tm);
      for (int j = 0; j < m.face.size(); j++)
        m.face[j].mat = mat_remap[m.face[j].mat];

      void *poly_B = csg->create_poly(NULL, m, 0, true);
      if (!poly_B)
      {
        DAEDITOR3.conWarning("cannot make CSG object for %s", entity->getObjAssetName());
        return;
      }

      void *result = csg->op(BasicCSG::A_MINUS_B, poly_A, NULL, poly_B, NULL);
      csg->delete_poly(poly_B);
      if (!result)
      {
        DAEDITOR3.conWarning("cannot subtract CSG object %s from outer one", entity->getObjAssetName());
        return;
      }
      csg->delete_poly(poly_A);
      poly_A = result;
    }
  }

  static void generateObjectsPos(Tab<TMatrix> &out_pos, const Point3 &p0, const Point3 &p1, float min_ofs0, float min_ofs1,
    bool symm_ofs, int align, float min_step, float max_step, float floor_ofs, float out_ofs, GenObjDesc &go_desc)
  {
    out_pos.clear();

    Point3 dir(Point3::x0z(p1 - p0));
    float segLen = length(dir);
    if (segLen < 1e-3)
      return;
    dir /= segLen;
    if (min_ofs0 < 0)
      min_ofs0 = 0;
    if (min_ofs1 < 0)
      min_ofs1 = 0;
    if (symm_ofs)
      min_ofs0 = min_ofs1 = max(min_ofs0, min_ofs1);

    float inner_len = segLen - min_ofs0 - min_ofs1;
    if (inner_len < 0)
      return;
    int n = floorf(inner_len / min_step);
    n = go_desc.generateFullPattern(n + 1) - 1;
    if (n < 0)
      return;
    if (inner_len > n * max_step)
    {
      float dl = (inner_len - n * max_step);
      if (align == 0)
        min_ofs0 += dl / 2, min_ofs1 += dl / 2;
      else if (align == 1)
        min_ofs1 += dl;
      else if (align == -1)
        min_ofs0 += dl;
      inner_len = segLen - min_ofs0 - min_ofs1;
    }
    out_pos.resize(n + 1);
    Point3 outw(-dir.z, 0, dir.x);
    for (int i = 0; i <= n; i++)
    {
      TMatrix &tm = out_pos[i];
      tm.setcol(1, Point3(0, 1, 0));
      tm.setcol(0, -dir);
      tm.setcol(2, tm.getcol(0) % tm.getcol(1));
      tm.setcol(3, Point3::xVz(p1 - dir * (min_ofs1 + (i ? i * inner_len / n : 0.0f)) + outw * out_ofs, floor_ofs));
    }
  }

  static void generateVertEdges(const DataBlock &b, dag::ConstSpan<Point3> _p, float y0, dag::ConstSpan<float> floor_ht,
    StaticGeometryContainer &genGeom, Tab<IObjEntity *> &out_obj, int inst_seed, int &seed, float main_ang, int editLayerIdx)
  {
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (!assetSrv)
      return;
    const char *splClsNm = b.getStr("splineClass", NULL);
    const splineclass::AssetData *adata = splClsNm ? assetSrv->getSplineClassData(splClsNm) : NULL;
    if (!adata)
      return;

    Tab<Point3> p;
    if (float shift_d = b.getReal("shiftPerimeter", 0))
      shiftContour(p, _p, shift_d);
    else
      p = _p;

    Tab<Point2> angRange;
    Point2 cornerAngRange = b.getPoint2("cornerAngRange", Point2(-360, 360));
    IPoint2 floorRange = b.getIPoint2("floorRange", IPoint2(1, floor_ht.size()));
    float facet = b.getReal("facet", 0);
    float y0_ofs = b.getReal("y0_ofs", 0);
    float y1_ofs = b.getReal("y1_ofs", 0);

    int nid = b.getNameId("angRange");
    for (int i = 0; i < b.paramCount(); i++)
      if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_POINT2)
        angRange.push_back(b.getPoint2(i) * DEG_TO_RAD);

    Tab<Point4> shapePtsOrig;
    Tab<splineclass::LoftGeomGenData::Loft::PtAttr> shapePtsAttrOrig;
    for (int pi = 0; pi < p.size(); pi++)
    {
      Point3 dir0(p[(pi + 1) % p.size()] - p[pi]);
      Point3 dirP(p[pi] - p[(pi + p.size() - 1) % p.size()]);
      Point3 dirM(normalize(dir0) + normalize(dirP));
      float dir_ang0 = safe_atan2(dir0.x, -dir0.z);
      float dir_angP = safe_atan2(dirP.x, -dirP.z);
      float dir_ang = safe_atan2(dirM.x, -dirM.z);
      float corner_ang = -norm_s_ang(dir_ang0 - dir_angP);

      if (angRange.size())
      {
        bool suits = false;
        dir_ang = norm_s_ang(dir_ang - main_ang);
        for (int j = 0; j < angRange.size(); j++)
          if ((dir_ang > angRange[j].x && dir_ang < angRange[j].y) ||
              (dir_ang > angRange[j].x + 2 * PI && dir_ang < angRange[j].y + 2 * PI))
          {
            suits = true;
            break;
          }
        if (!suits)
          continue;
      }
      if ((corner_ang < cornerAngRange.x || corner_ang > cornerAngRange.y) &&
          (corner_ang < cornerAngRange.x + 2 * PI || corner_ang > cornerAngRange.y + 2 * PI))
        continue;

      BezierSpline3d path;
      Point3 pts[2];
      pts[0].set(0, y0 + y0_ofs, 0);
      pts[1].set(0, y0 + y1_ofs, 0);
      for (int j = 0; j < floor_ht.size(); j++)
        if (j + 1 < floorRange[0])
          pts[0].y += floor_ht[j], pts[1].y += floor_ht[j];
        else if (j + 1 <= floorRange[1])
          pts[1].y += floor_ht[j];
        else
          break;
      TMatrix rtm = makeTM(dir0, -PI / 2);
      pts[0] = rtm * pts[0];
      pts[1] = rtm * pts[1];
      path.calculateCatmullRom(pts, 2, false);
      rtm = makeTM(dir0, PI / 2);
      rtm.setcol(3, p[pi]);

      int segCnt = path.segs.size();
      Tab<splineclass::Attr> splineScales(midmem);
      splineScales.resize(segCnt + 1);
      mem_set_0(splineScales);
      for (int i = 0; i < splineScales.size(); i++)
      {
        splineScales[i].scale_h = splineScales[i].scale_w = splineScales[i].opacity = 1.0;
        splineScales[i].followOverride = splineScales[i].roadBhvOverride = -1;
      }

      if (adata->genGeom)
        for (int i = 0; i < adata->genGeom->loft.size(); i++)
        {
          if (!assetSrv->isLoftCreatable(adata->genGeom, i))
            continue;
          splineclass::LoftGeomGenData::Loft &loft = adata->genGeom->loft[i];
          if (loft.makeDelaunayPtCloud || loft.waterSurface)
            continue;
          bendLoftShape(loft, corner_ang, facet, shapePtsOrig, shapePtsAttrOrig);

          for (int materialNo = 0; materialNo < loft.matNames.size(); materialNo++)
          {
            Ptr<MaterialData> material;
            material = assetSrv->getMaterialData(loft.matNames[materialNo]);
            if (!material.get())
            {
              DAEDITOR3.conError("<%s>: invalid material <%s> for loft", splClsNm, loft.matNames[materialNo].str());
              continue;
            }
            StaticGeometryContainer *g = new StaticGeometryContainer;

            Mesh *mesh = new Mesh;

            if (!assetSrv->createLoftMesh(*mesh, adata->genGeom, i, path, 0, segCnt, true, 1.0f, materialNo, splineScales, NULL,
                  splClsNm, 0, 0))
            {
              delete mesh;
              continue;
            }
            mesh->transform(rtm);

            MaterialDataList mat;
            mat.addSubMat(material);

            ObjCreator3d::addNode(splClsNm, mesh, &mat, *g);
            for (int j = 0; j < g->nodes.size(); ++j)
            {
              StaticGeometryNode *node = g->nodes[j];

              node->flags = loft.flags;
              node->normalsDir = loft.normalsDir;

              node->calcBoundBox();
              node->calcBoundSphere();

              if (loft.loftLayerOrder)
                node->script.setInt("layer", loft.loftLayerOrder);
              if (loft.stage)
                node->script.setInt("stage", loft.stage);
              if (!loft.layerTag.empty())
                node->script.setStr("layerTag", loft.layerTag);
              genGeom.addNode(new StaticGeometryNode(*node));
            }
            delete g;
          }
          restoreLoftShape(loft, shapePtsOrig, shapePtsAttrOrig);
        }

      Tab<splineclass::SingleEntityPool> entPool;
      objgenerator::generateBySpline(::toPrecSpline(path), NULL, 0, segCnt, adata, entPool, nullptr, false, polyTileMask, editLayerIdx,
        seed, inst_seed);

      int obj_start = out_obj.size();
      for (int i = 0; i < entPool.size(); i++)
      {
        append_items(out_obj, entPool[i].entPool.size(), entPool[i].entPool.data());
        clear_and_shrink(entPool[i].entPool);
      }
      TMatrix corn_tm = rtm * makeTM(Point3(-dir0.z, 0, dir0.x), -corner_ang / 2);
      for (int i = obj_start; i < out_obj.size(); i++)
      {
        TMatrix tm;
        out_obj[i]->getTm(tm);
        out_obj[i]->setTm(corn_tm * tm);
      }
    }
  }
  static void generateHorEdges(const DataBlock &b, dag::ConstSpan<Point3> _p, float y0, dag::ConstSpan<float> floor_ht,
    StaticGeometryContainer &genGeom, Tab<IObjEntity *> &out_obj, int inst_seed, int &seed, float main_ang, int editLayerIdx)
  {
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (!assetSrv)
      return;
    const char *splClsNm = b.getStr("splineClass", NULL);
    const splineclass::AssetData *adata = splClsNm ? assetSrv->getSplineClassData(splClsNm) : NULL;
    if (!adata)
      return;

    Tab<Point3> p;
    if (float shift_d = b.getReal("shiftPerimeter", 0))
      shiftContour(p, _p, shift_d);
    else
      p = _p;

    Tab<Point2> angRange;
    float x0_ofs = b.getReal("x0_ofs", 0);
    float x1_ofs = b.getReal("x1_ofs", 0);
    y0 += b.getReal("y_ofs", 0);
    for (int i = 0, floor_n = b.getInt("floor", 1) - 1; i < floor_n; i++)
      y0 += floor_ht[i];

    int nid = b.getNameId("angRange");
    for (int i = 0; i < b.paramCount(); i++)
      if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_POINT2)
        angRange.push_back(b.getPoint2(i) * DEG_TO_RAD);

    for (int pi = 0; pi < p.size(); pi++)
    {
      Point3 dir(p[(pi + 1) % p.size()] - p[pi]);
      float dir_ang = safe_atan2(dir.x, -dir.z);

      if (angRange.size())
      {
        bool suits = false;
        dir_ang = norm_s_ang(dir_ang - main_ang);
        for (int j = 0; j < angRange.size(); j++)
          if ((dir_ang > angRange[j].x && dir_ang < angRange[j].y) ||
              (dir_ang > angRange[j].x + 2 * PI && dir_ang < angRange[j].y + 2 * PI))
          {
            suits = true;
            break;
          }
        if (!suits)
          continue;
      }

      BezierSpline3d path;
      Point3 pts[2];
      pts[0].set_xVz(p[(pi + 1) % p.size()], y0);
      pts[1].set_xVz(p[pi], y0);
      if (x0_ofs)
        pts[0] -= x0_ofs * normalize(dir);
      if (x1_ofs)
        pts[1] -= x1_ofs * normalize(dir);

      path.calculateCatmullRom(pts, 2, false);

      int segCnt = path.segs.size();
      Tab<splineclass::Attr> splineScales(midmem);
      splineScales.resize(segCnt + 1);
      mem_set_0(splineScales);
      for (int i = 0; i < splineScales.size(); i++)
      {
        splineScales[i].scale_h = splineScales[i].scale_w = splineScales[i].opacity = 1.0;
        splineScales[i].followOverride = splineScales[i].roadBhvOverride = -1;
      }

      if (adata->genGeom)
        for (int i = 0; i < adata->genGeom->loft.size(); i++)
        {
          if (!assetSrv->isLoftCreatable(adata->genGeom, i))
            continue;
          splineclass::LoftGeomGenData::Loft &loft = adata->genGeom->loft[i];
          if (loft.makeDelaunayPtCloud || loft.waterSurface)
            continue;

          for (int materialNo = 0; materialNo < loft.matNames.size(); materialNo++)
          {
            Ptr<MaterialData> material;
            material = assetSrv->getMaterialData(loft.matNames[materialNo]);
            if (!material.get())
            {
              DAEDITOR3.conError("<%s>: invalid material <%s> for loft", splClsNm, loft.matNames[materialNo].str());
              continue;
            }
            StaticGeometryContainer *g = new StaticGeometryContainer;

            Mesh *mesh = new Mesh;

            if (!assetSrv->createLoftMesh(*mesh, adata->genGeom, i, path, 0, segCnt, true, 1.0f, materialNo, splineScales, NULL,
                  splClsNm, 0, 0))
            {
              delete mesh;
              continue;
            }

            MaterialDataList mat;
            mat.addSubMat(material);

            ObjCreator3d::addNode(splClsNm, mesh, &mat, *g);
            for (int j = 0; j < g->nodes.size(); ++j)
            {
              StaticGeometryNode *node = g->nodes[j];

              node->flags = loft.flags;
              node->normalsDir = loft.normalsDir;

              node->calcBoundBox();
              node->calcBoundSphere();

              if (loft.loftLayerOrder)
                node->script.setInt("layer", loft.loftLayerOrder);
              if (loft.stage)
                node->script.setInt("stage", loft.stage);
              genGeom.addNode(new StaticGeometryNode(*node));
            }
            delete g;
          }
        }

      Tab<splineclass::SingleEntityPool> entPool;
      objgenerator::generateBySpline(::toPrecSpline(path), NULL, 0, segCnt, adata, entPool, nullptr, false, polyTileMask, editLayerIdx,
        seed, inst_seed);

      int obj_start = out_obj.size();
      for (int i = 0; i < entPool.size(); i++)
      {
        append_items(out_obj, entPool[i].entPool.size(), entPool[i].entPool.data());
        clear_and_shrink(entPool[i].entPool);
      }
    }
  }
  static void generatePerimeterGeom(const DataBlock &b, dag::ConstSpan<Point3> _p, float y0, dag::ConstSpan<float> floor_ht,
    StaticGeometryContainer &genGeom, Tab<IObjEntity *> &out_obj, int inst_seed, int &seed, float main_ang, int editLayerIdx)
  {
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (!assetSrv)
      return;
    const char *splClsNm = b.getStr("splineClass", NULL);
    const splineclass::AssetData *adata = splClsNm ? assetSrv->getSplineClassData(splClsNm) : NULL;
    if (!adata)
      return;

    Tab<Point3> p;
    float cornerHelperLen = b.getReal("cornerHelperLen", 0.01);
    if (float shift_d = b.getReal("shiftPerimeter", 0))
      shiftContour(p, _p, shift_d);
    else
      p = _p;

    y0 += b.getReal("y_ofs", 0);
    for (int i = 0, floor_n = b.getInt("floor", 1) - 1; i < floor_n; i++)
      y0 += floor_ht[i];

    Tab<Point3> pts;
    pts.resize((p.size() + 1) * 3);
    for (int pi = 0; pi < pts.size(); pi += 3)
    {
      int cur = (p.size() - pi / 3) % p.size();
      int prev = (cur + 1) % p.size();
      int next = (cur + p.size() - 1) % p.size();
      pts[pi + 1] = Point3::xVz(p[cur], y0);
      pts[pi + 0] = pts[pi + 1] + normalize(Point3::xVz(p[prev], y0) - pts[pi + 1]) * cornerHelperLen;
      pts[pi + 2] = pts[pi + 1] + normalize(Point3::xVz(p[next], y0) - pts[pi + 1]) * cornerHelperLen;
    }

    BezierSpline3d path;
    path.calculate(pts.data(), pts.size(), false);
    clear_and_shrink(pts);
    clear_and_shrink(p);

    int segCnt = path.segs.size();
    Tab<splineclass::Attr> splineScales(midmem);
    splineScales.resize(segCnt + 1);
    mem_set_0(splineScales);
    for (int i = 0; i < splineScales.size(); i++)
    {
      splineScales[i].scale_h = splineScales[i].scale_w = splineScales[i].opacity = 1.0;
      splineScales[i].followOverride = splineScales[i].roadBhvOverride = -1;
    }

    if (adata->genGeom)
      for (int i = 0; i < adata->genGeom->loft.size(); i++)
      {
        if (!assetSrv->isLoftCreatable(adata->genGeom, i))
          continue;
        splineclass::LoftGeomGenData::Loft &loft = adata->genGeom->loft[i];
        if (loft.makeDelaunayPtCloud || loft.waterSurface)
          continue;

        for (int materialNo = 0; materialNo < loft.matNames.size(); materialNo++)
        {
          Ptr<MaterialData> material;
          material = assetSrv->getMaterialData(loft.matNames[materialNo]);
          if (!material.get())
          {
            DAEDITOR3.conError("<%s>: invalid material <%s> for loft", splClsNm, loft.matNames[materialNo].str());
            continue;
          }
          StaticGeometryContainer *g = new StaticGeometryContainer;

          Mesh *mesh = new Mesh;

          if (!assetSrv->createLoftMesh(*mesh, adata->genGeom, i, path, 0, segCnt, true, 1.0f, materialNo, splineScales, NULL,
                splClsNm, 0, 0))
          {
            delete mesh;
            continue;
          }

          MaterialDataList mat;
          mat.addSubMat(material);

          ObjCreator3d::addNode(splClsNm, mesh, &mat, *g);
          for (int j = 0; j < g->nodes.size(); ++j)
          {
            StaticGeometryNode *node = g->nodes[j];

            node->flags = loft.flags;
            node->normalsDir = loft.normalsDir;

            node->calcBoundBox();
            node->calcBoundSphere();

            if (loft.loftLayerOrder)
              node->script.setInt("layer", loft.loftLayerOrder);
            if (loft.stage)
              node->script.setInt("stage", loft.stage);
            genGeom.addNode(new StaticGeometryNode(*node));
          }
          delete g;
        }
      }

    Tab<splineclass::SingleEntityPool> entPool;
    objgenerator::generateBySpline(::toPrecSpline(path), NULL, 0, segCnt, adata, entPool, nullptr, false, polyTileMask, editLayerIdx,
      seed, inst_seed);

    int obj_start = out_obj.size();
    for (int i = 0; i < entPool.size(); i++)
    {
      append_items(out_obj, entPool[i].entPool.size(), entPool[i].entPool.data());
      clear_and_shrink(entPool[i].entPool);
    }
  }
  static void bendLoftShape(splineclass::LoftGeomGenData::Loft &loft, float corner_ang, float facet, Tab<Point4> &pts_orig,
    Tab<splineclass::LoftGeomGenData::Loft::PtAttr> &pts_attr_orig)
  {
    pts_orig = loft.shapePts;
    pts_attr_orig = loft.shapePtsAttr;

    int center_pt = -1;
    float sa, ca;
    sincos(corner_ang, sa, ca);

    for (int i = 0; i < loft.shapePts.size(); i++)
      if (fabsf(loft.shapePts[i].x) < 1e-3 && center_pt < 0)
      {
        center_pt = i;
        Point4 &p = loft.shapePts[i];
        float hsa, hca;
        sincos(corner_ang / 2, hsa, hca);
        p.set(p.x * hca + p.y * hsa, -p.x * hsa + p.y * hca, p.z, p.w + 0.01);
      }
      else if (loft.shapePts[i].x > 0)
      {
        Point4 &p = loft.shapePts[i];
        p.set(p.x * ca + p.y * sa, -p.x * sa + p.y * ca, p.z, p.w);
      }
    //== use facet parameter
  }
  static void restoreLoftShape(splineclass::LoftGeomGenData::Loft &loft, Tab<Point4> &pts_orig,
    Tab<splineclass::LoftGeomGenData::Loft::PtAttr> &pts_attr_orig)
  {
    loft.shapePts = pts_orig;
    loft.shapePtsAttr = pts_attr_orig;
  }

  void copyFrom(const CsgEntity &e)
  {
    aMgr = e.aMgr;
    assetNameId = e.assetNameId;
    clear_and_shrink(foundationPath);
    foundationPathClosed = false;
    clearEntites();
  }

  void clearEntites()
  {
    for (int i = 0; i < entList.size(); i++)
      destroy_it(entList[i]);
    entList.clear();
  }

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };
  enum
  {
    FLG_CLIP_EDITOR = 0x0001,
    FLG_CLIP_GAME = 0x0002,
  };

  unsigned idx;
  int rndSeed;
  TMatrix tm;
  GeomObject *geom;
  collisionpreview::Collision collision;
  float visRad;
  BBox3 bbox;
  BSphere3 bsph;
  Tab<Point3> foundationPath;
  bool foundationPathClosed;
  Tab<IObjEntity *> entList;
};
DAG_DECLARE_RELOCATABLE(CsgEntity::GenObjFloorDesc::Obj);


class CsgEntityManagementService : public IEditorService,
                                   public IObjEntityMgr,
                                   public IRenderingService,
                                   public IGatherStaticGeometry,
                                   public IDagorEdCustomCollider,
                                   public IDagorAssetChangeNotify
{
public:
  CsgEntityManagementService()
  {
    csgEntityClassId = IDaEditor3Engine::get().getAssetTypeId("csg");
    rendinstClassId = IDaEditor3Engine::get().getAssetTypeId("rendInst");
    prefabClassId = IDaEditor3Engine::get().getAssetTypeId("prefab");
    compositClassId = IDaEditor3Engine::get().getAssetTypeId("composit");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    polyTileMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");

    visible = true;
  }


  ~CsgEntityManagementService() { PrefabGeomCacheRec::clear(); }


  // IEditorService interface
  virtual const char *getServiceName() const { return "_csgEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) CSG entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    if (!(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & collisionSubtypeMask))
      return;

    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    if (!ent.size())
      return;

    begin_draw_cached_debug_lines();
    const E3DCOLOR color(0, 255, 0);
    for (int i = 0, ie = ent.size(); i < ie; i++)
      if (ent[i] && ((ent[i]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE) ||
                      ((ent[i]->getFlags() & ent[i]->FLG_CLIP_GAME) == ent[i]->FLG_CLIP_GAME)))
        collisionpreview::drawCollisionPreview(ent[i]->collision, TMatrix::IDENT, color);
    end_draw_cached_debug_lines();
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("CsgMgr: %d entities", entPool.getUsedEntityCount());
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, IGatherStaticGeometry);
    return NULL;
  }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if ((st_mask & rendEntGeomMask) != rendEntGeomMask)
      return;

    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();

    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE:
      case STG_RENDER_SHADOWS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->geom->render();
        break;

      case STG_RENDER_STATIC_TRANS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->geom->renderTrans();
        break;
    }
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const { return csgEntityClassId >= 0 && csgEntityClassId == entity_class; }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, csgEntityClassId);
      asset.getMgr().subscribeUpdateNotify(this, -1, prefabClassId);
    }

    if (virtual_ent)
    {
      VirtualCsgEntity *ent = new VirtualCsgEntity(csgEntityClassId);
      ent->setup(asset);
      return ent;
    }

    CsgEntity *ent = entPool.allocEntity();
    if (!ent)
      ent = new CsgEntity(csgEntityClassId);

    ent->setup(asset);
    entPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    CsgEntity *o = reinterpret_cast<CsgEntity *>(origin);
    CsgEntity *ent = entPool.allocEntity();
    if (!ent)
      ent = new CsgEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    entPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IGatherStaticGeometry interface
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &cont)
  {
    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
        addGeometry(ent[i], i, cont, StaticGeometryNode::FLG_RENDERABLE);
  }
  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &cont) {}
  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont)
  {
    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask) &&
          (ent[i]->getFlags() & ent[i]->FLG_CLIP_GAME) == ent[i]->FLG_CLIP_GAME)
        addGeometry(ent[i], i, cont, StaticGeometryNode::FLG_COLLIDABLE);
  }
  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont)
  {
    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask) &&
          (ent[i]->getFlags() & ent[i]->FLG_CLIP_EDITOR) == ent[i]->FLG_CLIP_EDITOR)
        addGeometry(ent[i], i, cont, StaticGeometryNode::FLG_COLLIDABLE);
  }
  void addGeometry(CsgEntity *ent, int idx, StaticGeometryContainer &cont, int node_flags)
  {
    const StaticGeometryContainer *g = ent->geom->getGeometryContainer();

    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      if (node && (node->flags & node_flags) == node_flags)
      {
        StaticGeometryNode *n = new (midmem) StaticGeometryNode(*node);

        n->wtm = node->wtm;
        n->flags = node->flags;
        n->visRange = node->visRange;
        n->lighting = node->lighting;

        n->name = String(256, "%s_%d_%s", ent->assetName(), idx, node->name.str());

        if (node->topLodName.length())
          n->topLodName = String(256, "%s_%d_%s", ent->assetName(), idx, node->topLodName.str());

        cont.addNode(n);
      }
    }
  }

  // IDagorEdCustomCollider interface
  virtual bool traceRay(const Point3 &p0, const Point3 &dir, real &maxt, Point3 *norm)
  {
    if (maxt <= 0)
      return false;

    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    bool ret = false;
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask) &&
          ent[i]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE)
        if (ent[i]->geom->traceRay(p0, dir, maxt, norm))
          ret = true;
    return ret;
  }
  virtual bool shadowRayHitTest(const Point3 &p0, const Point3 &dir, real maxt)
  {
    if (maxt <= 0)
      return false;

    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    bool ret = false;
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask) &&
          ent[i]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE)
        if (ent[i]->geom->shadowRayHitTest(p0, dir, maxt))
          return true;

    return false;
  }
  virtual const char *getColliderName() const { return getServiceFriendlyName(); }
  virtual bool isColliderVisible() const { return visible; }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    if (asset_type == prefabClassId)
      PrefabGeomCacheRec::evictCache(asset_name_id);
    if (asset_type != csgEntityClassId)
      return;
    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->geom && ent[i]->assetNameId == asset_name_id)
        del_it(ent[i]->geom);
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    if (asset_type == prefabClassId)
      PrefabGeomCacheRec::evictCache(asset_name_id);
    if (asset_type != csgEntityClassId)
      return;
    dag::ConstSpan<CsgEntity *> ent = entPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->assetNameId == asset_name_id)
      {
        DAEDITOR3.conNote("regen CSG entity[%d] due to %s asset changed", i, ent[i]->assetName());
        ent[i]->setFoundationPath(make_span(ent[i]->foundationPath), ent[i]->foundationPathClosed);
      }
  }

protected:
  CsgEntityPool entPool;
  bool visible;
};


void init_csg_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) CsgEntityManagementService);
}
