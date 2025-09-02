// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_rect.h>
#include <3d/dag_texMgr.h>

#include <math/dag_bounds2.h>
#include <math/dag_Point3.h>
#include <EditorCore/ec_interface.h>
#include <util/dag_string.h>
#include <gameMath/objgenPrng.h>
#include <ioSys/dag_genIo.h>
#include <libTools/staticGeom/geomObject.h>
#include <coolConsole/coolConsole.h>

enum
{
  GEOM_TYPE_PRISM = 1,
  GEOM_TYPE_BILLBOARDS,
};

class DynRenderBuffer;
class Point2;

class GeomResourcesHelper;

static inline Point4 toPoint4(const Point3 &p, real w) { return Point4(p.x, p.y, p.z, w); }

static bool pt_intersects(const Point3 &p, float rad, int x, int y, IGenViewportWnd *vp)
{
  BBox2 ptBox;
  Point2 pt;

  vp->worldToClient(p, pt);
  ptBox[0] = pt - Point2(rad, rad);
  ptBox[1] = pt + Point2(rad, rad);
  return ptBox & Point2(x, y);
}

static bool pt_intersects(const Point3 &p, float rad, const EcRect &rect, IGenViewportWnd *vp)
{
  BBox2 ptBox;
  BBox2 rectBox;
  Point2 pt;

  vp->worldToClient(p, pt);
  ptBox[0] = pt - Point2(rad, rad);
  ptBox[1] = pt + Point2(rad, rad);

  rectBox[0].x = rect.l;
  rectBox[1].x = rect.r;
  rectBox[0].y = rect.t;
  rectBox[1].y = rect.b;

  return ptBox & rectBox;
}

static constexpr unsigned CID_IvyObject = 0xA407C109u; // IvyObject

/** an ivy node */
class IvyNode
{
public:
  IvyNode() : climb(false), length(0.0f), floatingLength(0.0f) {}

  unsigned short iteration;

  /** climbing state */
  bool climb;

  /** node position */
  Point3 pos;

  /** primary grow direction, a weighted sum of the previous directions */
  Point3 primaryDir;

  /** adhesion vector as a result from other scene objects */
  Point3 adhesionVector;
  Point3 faceNormal;

  /** a smoothed adhesion vector computed and used during the birth phase,
  since the ivy leaves are align by the adhesion vector, this smoothed vector
  allows for smooth transitions of leaf alignment */
  Point3 smoothAdhesionVector;

  /** length of the associated ivy branch at this node */
  float length;

  /** length at the last node that was climbing */
  float floatingLength;

  void save(IGenSave &cwr, bool alive)
  {
    cwr.write(&pos, sizeof(Point3));

    if (alive)
    {
      primaryDir = primaryDir * 0.5f + Point3(0.5f, 0.5f, 0.5f);
      cwr.writeIntP<1>(real2uchar(primaryDir.x));
      cwr.writeIntP<1>(real2uchar(primaryDir.y));
      cwr.writeIntP<1>(real2uchar(primaryDir.z));

      cwr.write(&adhesionVector, sizeof(Point3));
      cwr.writeReal(length);
      cwr.writeReal(floatingLength);
    }
  }

  void load(IGenLoad &crd, bool alive)
  {
    crd.read(&pos, sizeof(Point3));

    if (alive)
    {
      real x = (char)crd.readIntP<1>() / 255.f;
      real y = (char)crd.readIntP<1>() / 255.f;
      real z = (char)crd.readIntP<1>() / 255.f;

      Point3 pd(x, y, z);
      pd -= Point3(0.5f, 0.5f, 0.5f);
      pd /= 0.5f;

      primaryDir = pd;

      crd.read(&adhesionVector, sizeof(Point3));
      length = crd.readReal();
      floatingLength = crd.readReal();
    }
  }
};


/** an ivy root point */
class IvyRoot
{
public:
  IvyRoot() : nodes(midmem) { aliveIteration = -1; }

  /** a number of nodes */
  Tab<IvyNode *> nodes;

  /** alive state */
  unsigned alive : 1;

  /** number of parents, represents the level in the root hierarchy */
  unsigned parents : 15;
  unsigned iteration : 16;

  int aliveIteration;

  void save(IGenSave &cwr)
  {
    cwr.writeIntP<1>((char)alive);
    cwr.writeInt(parents);
    cwr.writeInt(nodes.size());

    for (int i = 0; i < nodes.size(); i++)
      nodes[i]->save(cwr, alive);
  }

  void load(IGenLoad &crd)
  {
    alive = (bool)crd.readIntP<1>();
    parents = crd.readInt();
    int cnt = crd.readInt();

    for (int i = 0; i < cnt; i++)
    {
      IvyNode *n = new IvyNode;
      n->load(crd, alive);
      nodes.push_back(n);
    }
  }
};


//
// IvyObject
//
class IvyObject : public RenderableEditableObject
{
protected:
  ~IvyObject() override {}

public:
  class CollisionCallback
  {
  public:
    /** compute the adhesion of scene objects at a point pos*/
    virtual Point3 computeAdhesion(const Point3 &pos, float maxDist, Point3 &norm) = 0;

    virtual bool traceRay(const Point3 &oldPos, const Point3 &dir, float &at, Point3 &normal) = 0;
  };

  IvyObject(int uid);

  void defaults();
  void clipMax();

  void update(float) override;

  void beforeRender() override {}
  void render() override;
  void renderTrans() override;
  void renderPts(DynRenderBuffer &dynBuf, const TMatrix4 &gtm, const Point2 &s, bool start = false);

  void renderGeom();

  static void renderPoint(DynRenderBuffer &db, const Point4 &p, const Point2 &s, E3DCOLOR c);
  static void initStatics();
  static void initCollisionCallback();

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool getWorldBox(BBox3 &box) const override;

  void fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  // restrict rotate/scale transformations to BASIS_Local/selCenter

  void putMoveUndo() override;

  void moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis) override;
  void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override {}
  void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override {}

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  void setWtm(const TMatrix &wtm) override;

  bool mayRename() override { return true; }
  bool mayDelete() override { return true; }

  EO_IMPLEMENT_RTTI(CID_IvyObject)

  int getUid() const { return uid; }
  Point3 getPt() const { return pt; }

  void saveIvy(DataBlock &blk);
  void loadIvy(const DataBlock &blk);

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags);

  IvyObject *clone();

  void markFinishGeneration() { finishGeneration = true; }

public:
  SimpleString assetName;

  real size;
  real primaryWeight, randomWeight, gravityWeight, adhesionWeight;
  real branchingProbability;
  real maxFloatingLength, maxAdhesionDistance;
  real ivyBranchSize, ivyLeafSize, leafDensity;

  real primaryWeight_, randomWeight_, adhesionWeight_;

  int genGeomType;
  real nodesAngleDelta;
  real maxTriangles;

  bool visible;
  int growing;

  bool needRenderDebug, needRenderGeom;

  Tab<IvyRoot *> roots;
  int currentSeed, startSeed;

  int growed;
  int trisCount;

  Point3 pt;
  String lastChoosed;

  int uid;
  GeomObject *ivyGeom;
  int needToGrow;
  bool needDecreaseGrow;
  bool canGrow;
  bool firstAct;
  bool finishGeneration;

  void startGrow();
  void clearIvy();
  void reGrow();
  void seed();
  void grow();
  void decreaseGrow();
  void generateGeom(CoolConsole &con);
  void recalcLighting();
  inline void stopAutoGrow() { growing = 0; }
  Point3 computeAdhesion(const Point3 &pos, CollisionCallback &cb, Point3 &norm);
  bool computeCollision(const Point3 &oldPos, Point3 &newPos, bool &climbing, CollisionCallback &cb);
  inline Point3 getRandomized()
  {
    using namespace objgenerator; // prng
    return normalize(Point3(frnd(currentSeed) - 0.5f, frnd(currentSeed) - 0.5f, frnd(currentSeed) - 0.5f));
  }
  void debugRender();


  static E3DCOLOR norm_col, sel_col, sel2_col, hlp_col, norm_col_start_point;
  static TEXTUREID texPt;
  static float ptScreenRad;
  static float ptScreenRadSm;
  static int ptRenderPassId;
  static bool objectWasMoved;
};
