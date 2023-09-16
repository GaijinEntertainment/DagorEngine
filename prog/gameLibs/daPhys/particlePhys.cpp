#include <daPhys/particlePhys.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <regExp/regExp.h>
#include <util/dag_oaHashNameMap.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug3d.h>

using namespace daphys;

ParticlePoint::ParticlePoint(const TMatrix &initial_tm)
{
  limits.lim[0].zero();
  limits.lim[1].zero();
  initialTm = initial_tm;
  initialTm.orthonormalize();
  helperTm = inverse(initialTm) * initial_tm;
  tm = initialTm;
  invInitialTm = inverse(initialTm);
  gnNodeId = dag::Index16();
}

Point3 ParticlePoint::limitDir(const Point3 &dir)
{
  return Point3(clamp(dir.x, limits.lim[0].x, limits.lim[1].x), clamp(dir.y, limits.lim[0].y, limits.lim[1].y),
    clamp(dir.z, limits.lim[0].z, limits.lim[1].z));
}

void ParticlePoint::addDelta(const Point3 &delta)
{
  Point3 limitedDelta = limitDir(delta);
  tm.setcol(3, tm.getcol(3) + limitedDelta);
}


struct EdgeConstraint : public Constraint
{
  ParticlePoint *ptA;
  ParticlePoint *ptB;
  float edgeLen;
  bool moveNodes;
  bool rotateSecond;
  bool moveSecond;
  bool moveOnly;
  bool isSpring;
  Point3 posInitialA;
  Point3 posInitialB;

  EdgeConstraint(ParticlePoint *pt_a, ParticlePoint *pt_b) :
    moveNodes(true), rotateSecond(false), moveSecond(true), moveOnly(false), isSpring(false)
  {
    init(pt_a, pt_b);
  }

  void init(ParticlePoint *pt_a, ParticlePoint *pt_b)
  {
    ptA = pt_a;
    ptB = pt_b;
    edgeLen = length(ptA->getPos() - ptB->getPos());
    posInitialA = ptA->invInitialTm * ptB->getInitialPos();
    posInitialB = ptB->invInitialTm * ptA->getInitialPos();
  }

  virtual bool solve()
  {
    if (!moveNodes)
      return false;
    Point3 posA = ptA->getPos();
    Point3 posB = ptB->getPos();

    Point3 dir = posB - posA;
    float curLen = length(dir);
    dir *= safeinv(curLen);
    Point3 limitedDirA = ptA->limitDir(-dir * (edgeLen - curLen) * 0.5f);
    Point3 limitedDirB = ptB->limitDir(+dir * (edgeLen - curLen) * 0.5f);
    float lenA = length(limitedDirA);
    float lenB = length(limitedDirB);
    limitedDirA *= fabsf(safediv(edgeLen - curLen - lenB, lenA));
    limitedDirB *= fabsf(safediv(edgeLen - curLen - lenA, lenB));
    ptA->addDelta(limitedDirA);
    ptB->addDelta(limitedDirB);
    const float EDGE_EPSILON = 1e-3f;
    return fabsf(edgeLen - curLen) > EDGE_EPSILON;
  }

  void updateNodeWtmRotation(ParticlePoint *pt_a, ParticlePoint *pt_b, const Point3 &pos_initial, GeomNodeTree *tree,
    const TMatrix &render_space_tm)
  {
    if (auto nodeA = pt_a->gnNodeId)
    {
      TMatrix rotTm = moveOnly ? TMatrix::IDENT : makeTM(quat_rotation_arc(pos_initial, inverse(pt_a->tm) * pt_b->getPos()));
      if (isSpring)
        rotTm.setcol(2, rotTm.getcol(2) * safediv(length(pt_a->getPos() - pt_b->getPos()), edgeLen));
      TMatrix finalTm = render_space_tm * ((pt_a->tm * rotTm) * pt_a->helperTm);
      tree->setNodeWtmScalar(nodeA, finalTm);
      tree->markNodeTmInvalid(nodeA);
    }
  }

  virtual void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm)
  {
    updateNodeWtmRotation(ptA, ptB, posInitialA, tree, render_space_tm);

    if (rotateSecond)
      updateNodeWtmRotation(ptB, ptA, posInitialB, tree, render_space_tm);
    else if (moveSecond)
    {
      if (auto nodeB = ptB->gnNodeId)
      {
        tree->setNodeWtmRelScalar(nodeB, render_space_tm * (ptB->tm * ptB->helperTm));
        tree->markNodeTmInvalid(nodeB);
      }
    }
  }

  virtual void renderDebug(const TMatrix &render_space_tm) const
  {
    begin_draw_cached_debug_lines(false, false);
    Point3 ptApos = (render_space_tm * ptA->tm).getcol(3);
    Point3 ptBpos = (render_space_tm * ptB->tm).getcol(3);
    draw_cached_debug_sphere(ptApos, 0.02f, E3DCOLOR_MAKE(255, 255, 0, 255));
    draw_cached_debug_line(ptApos, ptBpos, E3DCOLOR_MAKE(255, 255, 255, 255));
    end_draw_cached_debug_lines();
  }
};

struct RigidConstraint : public Constraint
{
  ParticlePoint *ptPivot;
  ParticlePoint *ptAttached;
  Point3 relativePos;

  RigidConstraint(ParticlePoint *pt_pivot, ParticlePoint *pt_attached)
  {
    ptPivot = pt_pivot;
    ptAttached = pt_attached;
    relativePos = ptPivot->invInitialTm * ptAttached->getInitialPos();
  }

  virtual bool solve()
  {
    ptAttached->tm.setcol(3, ptPivot->tm * relativePos);
    return false;
  }

  virtual void updateGeomNodeTree(GeomNodeTree * /*tree*/, const TMatrix & /*render_space_tm*/) {}
  virtual void renderDebug(const TMatrix &render_space_tm) const
  {
    begin_draw_cached_debug_lines(false, false);
    Point3 ptApos = (render_space_tm * ptPivot->tm).getcol(3);
    Point3 ptBpos = (render_space_tm * ptAttached->tm).getcol(3);
    draw_cached_debug_sphere(ptApos, 0.02f, E3DCOLOR_MAKE(255, 255, 0, 255));
    draw_cached_debug_line(ptApos, ptBpos, E3DCOLOR_MAKE(255, 255, 255, 255));
    end_draw_cached_debug_lines();
  }
};


struct SliderConstraint : public Constraint
{
  struct Edge
  {
    ParticlePoint *ptA;
    ParticlePoint *ptB;

    Edge(ParticlePoint *pt_a, ParticlePoint *pt_b) : ptA(pt_a), ptB(pt_b) {}
  };
  ParticlePoint *pt;
  Edge edge;

  SliderConstraint() : pt(NULL), edge(NULL, NULL) {}

  SliderConstraint(ParticlePoint *pt_initial, ParticlePoint *pt_a, ParticlePoint *pt_b) : pt(pt_initial), edge(pt_a, pt_b) {}

  void init(ParticlePoint *pt_initial, const Edge &edge_)
  {
    pt = pt_initial;
    edge = edge_;
  }

  virtual bool solve()
  {
    Point3 posA = edge.ptA->getPos();
    Point3 posB = edge.ptB->getPos();
    Point3 posOnEdge = pt->getPos();
    float t = 0.f;
    Point3 ptOnEdge = posOnEdge;
    distanceToSeg(posOnEdge, posA, posB, t, ptOnEdge);
    Point3 dir = ptOnEdge - posOnEdge;
    pt->addDelta(dir);
    const float SLIDER_EPSILON_SQ = 1e-6f;
    return lengthSq(dir) > SLIDER_EPSILON_SQ;
  }

  virtual void updateGeomNodeTree(GeomNodeTree * /*tree*/, const TMatrix & /*render_space_tm*/) {}
  virtual void renderDebug(const TMatrix & /*render_space_tm*/) const {}
};

struct DirectionConstraint : public Constraint
{
  ParticlePoint *pt; // we move this one
  ParticlePoint *ptA;
  ParticlePoint *ptB;
  Point3 dir;

  DirectionConstraint() : pt(NULL), ptA(NULL), ptB(NULL), dir(0.f, 1.f, 0.f) {}

  DirectionConstraint(ParticlePoint *pt_initial, ParticlePoint *pt_a, ParticlePoint *pt_b) : pt(pt_initial), ptA(pt_a), ptB(pt_b)
  {
    Point3 edge0 = ptA->getInitialPos() - pt->getInitialPos();
    Point3 edge1 = ptB->getInitialPos() - pt->getInitialPos();
    dir = edge0 % edge1;
  }

  virtual bool solve()
  {
    Point3 posA = ptA->getPos();
    Point3 posB = ptB->getPos();
    Point3 pos = pt->getPos();
    Point3 edge0 = posA - pos;
    Point3 edge1 = posB - pos;
    Point3 curDir = edge0 % edge1;
    if (curDir * dir < 0.f)
    {
      Point3 line = posB - posA;
      Point3 lineDir = normalize(line);
      float t = -(edge0 * lineDir);
      Point3 ptOnLine = posA + t * lineDir;
      Point3 mirroredDir = 2.f * (ptOnLine - pos);
      pt->addDelta(mirroredDir);
      return true;
    }
    return false;
  }

  virtual void updateGeomNodeTree(GeomNodeTree * /*tree*/, const TMatrix & /*render_space_tm*/) {}
  virtual void renderDebug(const TMatrix & /*render_space_tm*/) const {}
};

struct ProjectionConstraint : public SliderConstraint
{
  Point3 ptOnEdge;
  Point3 posInitial;

  float projectionDist;
  float distA;
  float distB;

  bool moveNodes;
  bool movePoint;
  bool moveEdge;
  bool moveEdgeNodes;

  ProjectionConstraint(ParticlePoint *pt_initial, ParticlePoint *pt_a, ParticlePoint *pt_b) :
    SliderConstraint(pt_initial, pt_a, pt_b), moveNodes(true), movePoint(true), moveEdge(false), moveEdgeNodes(true)
  {
    ptOnEdge.zero();

    Point3 posA = edge.ptA->getPos();
    Point3 posB = edge.ptB->getPos();
    Point3 posOnEdge = pt->getPos();
    float t = 0.f;
    projectionDist = distanceToSeg(posOnEdge, posA, posB, t, ptOnEdge);

    distA = t;
    distB = length(posB - posA) - t;

    posInitial = pt->invInitialTm * ptOnEdge;
  }

  virtual bool solve()
  {
    Point3 posA = edge.ptA->getPos();
    Point3 posB = edge.ptB->getPos();
    Point3 posOnEdge = pt->getPos();
    float t = 0.f;
    const float curDist = distanceToSeg(posOnEdge, posA, posB, t, ptOnEdge);

    if (!moveNodes)
      return false;

    Point3 dir = ptOnEdge - posOnEdge;
    dir *= safeinv(curDist);

    if (movePoint)
      pt->addDelta(+dir * (curDist - projectionDist));

    if (moveEdge)
    {
      Point3 delta = -dir * (curDist - projectionDist) * 0.5f;
      edge.ptA->addDelta(delta);
      edge.ptB->addDelta(delta);
    }

    if (moveEdgeNodes)
    {
      const float edgeLen = length(posB - posA);
      Point3 edgeDir = posB - posA;
      edgeDir *= safeinv(edgeLen);

      const float curDistA = t;
      const float curDistB = edgeLen - t;

      edge.ptA->addDelta(-edgeDir * (distA - curDistA) * 0.5f);
      edge.ptB->addDelta(+edgeDir * (distB - curDistB) * 0.5f);
    }

    const float PROJ_EPSILON = 1e-3f;
    return rabs(curDist - projectionDist) > PROJ_EPSILON;
  }

  virtual void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm)
  {
    if (auto node = pt->gnNodeId)
    {
      Point3 posNow = inverse(pt->tm) * ptOnEdge;
      Quat rot = quat_rotation_arc(posInitial, posNow);
      TMatrix rotTm = makeTM(rot);
      TMatrix finalTm = render_space_tm * ((pt->tm * rotTm) * pt->helperTm);
      tree->setNodeWtmRelScalar(node, finalTm);
      tree->markNodeTmInvalid(node);
    }
  }

  virtual void renderDebug(const TMatrix &render_space_tm) const
  {
    begin_draw_cached_debug_lines(false, false);
    Point3 ptPos = (render_space_tm * pt->tm).getcol(3);

    Point3 d = pt->getPos() - edge.ptA->getPos();
    Point3 seg = edge.ptB->getPos() - edge.ptA->getPos();
    Point3 proj = (d * seg) * safeinv(seg.lengthSq()) * seg + edge.ptA->getPos();

    Point3 projectionPointPos = render_space_tm * proj;
    draw_cached_debug_sphere(ptPos, 0.01f, E3DCOLOR_MAKE(255, 0, 0, 255));
    draw_cached_debug_sphere(projectionPointPos, 0.01f, E3DCOLOR_MAKE(0, 0, 255, 255));
    Point3 ptApos = (render_space_tm * edge.ptA->tm).getcol(3);
    Point3 ptBpos = (render_space_tm * edge.ptB->tm).getcol(3);
    draw_cached_debug_line(ptApos, ptBpos, E3DCOLOR_MAKE(255, 0, 255, 255));
    end_draw_cached_debug_lines();
  }
};

struct RevoluteConstraint : public Constraint
{
  ParticlePoint *pt;
  ParticlePoint *ptA;
  ParticlePoint *ptB;
  TMatrix invBaseA;
  TMatrix invBaseB;

  RevoluteConstraint() : pt(NULL), ptA(NULL), ptB(NULL), invBaseA(1), invBaseB(1) {}

  RevoluteConstraint(ParticlePoint *pt_initial, ParticlePoint *pt_a, ParticlePoint *pt_b) : pt(pt_initial), ptA(pt_a), ptB(pt_b)
  {
    Point3 edge0 = normalize(ptA->getInitialPos() - pt->getInitialPos());
    Point3 edge1 = normalize(ptB->getInitialPos() - pt->getInitialPos());
    Point3 axis = normalize(edge0 % edge1);
    invBaseA = inverse(makeBase(edge0, axis));
    invBaseB = inverse(makeBase(edge1, axis));
  }

  virtual bool solve() { return false; }

  static TMatrix makeBase(const Point3 &edge, const Point3 &axis)
  {
    TMatrix newBase;
    newBase.setcol(0, edge);
    newBase.setcol(1, axis);
    newBase.setcol(2, -(edge % axis));
    newBase.setcol(3, ZERO<Point3>());
    return newBase;
  }

  void updateNodeWtmRotation(ParticlePoint *pt_a, const TMatrix &inv_base_tm, const Point3 &edge, const Point3 &axis,
    GeomNodeTree *tree, const TMatrix &render_space_tm)
  {
    if (auto nodeA = pt_a->gnNodeId)
    {
      TMatrix tm = makeBase(edge, axis) % inv_base_tm % pt_a->tm;
      tm.setcol(3, pt_a->tm.getcol(3));
      TMatrix finalTm = render_space_tm * (tm * pt_a->helperTm);
      tree->setNodeWtmRelScalar(nodeA, finalTm);
      tree->markNodeTmInvalid(nodeA);
    }
  }

  virtual void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm)
  {
    Point3 edge0 = normalize(ptA->getPos() - pt->getPos());
    Point3 edge1 = normalize(ptB->getPos() - pt->getPos());
    Point3 axis = normalize(edge0 % edge1);

    updateNodeWtmRotation(ptA, invBaseA, edge0, axis, tree, render_space_tm);
    updateNodeWtmRotation(pt, invBaseB, edge1, axis, tree, render_space_tm);
  }

  virtual void renderDebug(const TMatrix &render_space_tm) const
  {
    begin_draw_cached_debug_lines(false, false);
    Point3 edge0 = ptA->getPos() - pt->getPos();
    Point3 edge1 = ptB->getPos() - pt->getPos();
    Point3 axis = normalize(edge0 % edge1) * 0.25f;
    Point3 ptApos = render_space_tm * (pt->tm.getcol(3) - axis);
    Point3 ptBpos = render_space_tm * (pt->tm.getcol(3) + axis);
    draw_cached_debug_line(ptApos, ptBpos, E3DCOLOR_MAKE(0, 255, 0, 255));
    end_draw_cached_debug_lines();
  }
};

ParticlePhysSystem::~ParticlePhysSystem()
{
  clear_all_ptr_items_and_shrink(particles);
  clear_all_ptr_items_and_shrink(constraints);
}

int ParticlePhysSystem::findParticleId(dag::Index16 gn_node_id) const
{
  for (int i = 0; i < particles.size(); ++i)
    if (particles[i]->gnNodeId == gn_node_id)
      return i;
  return -1;
}

ParticlePoint *ParticlePhysSystem::findParticle(dag::Index16 gn_node_id) const
{
  int idx = findParticleId(gn_node_id);
  return idx >= 0 ? particles[idx] : nullptr;
}

ParticlePoint *ParticlePhysSystem::getParticle(int particle_id) const
{
  return particle_id >= 0 && particle_id < particles.size() ? particles[particle_id] : nullptr;
}

static const char *find_child_name_by_regexp(const GeomNodeTree *tree, dag::Index16 node, RegExp &reg_exp, int occurence)
{
  if (!node)
    return NULL;
  for (unsigned i = 0, ie = tree->getChildCount(node); i < ie; ++i)
  {
    const char *name = tree->getNodeName(tree->getChildNodeIdx(node, i));
    if (!reg_exp.test(name))
      continue;
    if ((--occurence) > 0)
      continue;
    return name;
  }
  return NULL;
}

static ParticlePoint *create_point_at_node(const DataBlock *blk, const GeomNodeTree *tree, dag::Index16 node_id)
{
  TMatrix nodeTm;
  tree->getNodeWtmScalar(node_id, nodeTm);
  ParticlePoint *pt = new ParticlePoint(nodeTm);
  pt->gnNodeId = node_id;
  pt->limits.lim[0] = blk->getPoint3("limitMin", ZERO<Point3>());
  pt->limits.lim[1] = blk->getPoint3("limitMax", ZERO<Point3>());
  return pt;
}

static ParticlePoint *create_point_on_edge(const DataBlock *blk, const NameMap &point_names, dag::ConstSpan<ParticlePoint *> points)
{
  G_ASSERT(point_names.nameCount() == points.size());
  const char *pt1Name = blk->getStr("point1", NULL);
  const char *pt2Name = blk->getStr("point2", NULL);
  int pt1Idx = point_names.getNameId(pt1Name);
  int pt2Idx = point_names.getNameId(pt2Name);
  G_ASSERTF(pt1Idx >= 0, "Cannot find '%s'", pt1Name);
  G_ASSERTF(pt2Idx >= 0, "Cannot find '%s'", pt2Name);
  if (pt1Idx < 0 || pt2Idx < 0)
    return NULL;

  ParticlePoint *pt1 = points[pt1Idx];
  ParticlePoint *pt2 = points[pt2Idx];
  TMatrix initialTm = TMatrix::IDENT;
  initialTm.setcol(3, lerp(pt1->getInitialPos(), pt2->getInitialPos(), blk->getReal("pos", 0.5f)));
  ParticlePoint *ptOnEdge = new ParticlePoint(initialTm);
  ptOnEdge->limits.lim[0] = blk->getPoint3("limitMin", ZERO<Point3>());
  ptOnEdge->limits.lim[1] = blk->getPoint3("limitMax", ZERO<Point3>());
  return ptOnEdge;
}

static ParticlePoint *create_point_at_pivot(const DataBlock *blk, RegExp &reg_exp, const GeomNodeTree *tree)
{
  const char *nodeName = blk->getStr("name", NULL);
  G_ASSERT(nodeName);
  const char *finalNodeName = reg_exp.replace2(nodeName);
  auto nodeId = tree->findNodeIndex(finalNodeName);
  G_ASSERTF(nodeId || blk->getBool("optional", false), "Cannot find node '%s'", finalNodeName);
  del_it(finalNodeName);
  if (!nodeId)
    return NULL;

  return create_point_at_node(blk, tree, nodeId);
}

static ParticlePoint *create_point_at_child(const DataBlock *blk, const GeomNodeTree *tree, const NameMap &point_names,
  dag::ConstSpan<ParticlePoint *> points)
{
  G_ASSERT(tree);
  const char *parentName = blk->getStr("parent", NULL);
  int parentNid = point_names.getNameId(parentName);
  G_ASSERTF(parentNid >= 0, "Cannot find parent '%s'", parentName);
  if (parentNid < 0)
    return NULL;

  ParticlePoint *parent = points[parentNid];
  G_ASSERTF_RETURN(parent->gnNodeId, nullptr, "Parent should be node in GeomNodeTree");

  const char *findPattern = blk->getStr("find", NULL);
  int occurence = blk->getInt("occurence", 1); // first occurence by default
  RegExp re;
  re.compile(findPattern, "");
  const char *nodeName = find_child_name_by_regexp(tree, parent->gnNodeId, re, occurence);
  auto nodeId = nodeName ? tree->findNodeIndex(nodeName) : dag::Index16();
  G_ASSERTF_RETURN(nodeId, nullptr, "Cannot find child of '%s' by pattern '%s'", parentName, findPattern);

  return create_point_at_node(blk, tree, nodeId);
}

static Constraint *create_edge_constraint(const DataBlock *blk, const NameMap &point_names, dag::ConstSpan<ParticlePoint *> points)
{
  const char *fromName = blk->getStr("from", NULL);
  const char *toName = blk->getStr("to", NULL);
  int fromId = point_names.getNameId(fromName);
  int toId = point_names.getNameId(toName);
  G_ASSERTF(fromId >= 0 || blk->getBool("optional", false), "Cannot find '%s' point for edge constraint", fromName);
  G_ASSERTF(toId >= 0 || blk->getBool("optional", false), "Cannot find '%s' point for edge constraint", toName);
  if (fromId < 0 || toId < 0)
    return NULL;

  EdgeConstraint *constr = new EdgeConstraint(points[fromId], points[toId]);
  constr->moveNodes = blk->getBool("moveNodes", true);
  constr->rotateSecond = blk->getBool("rotateSecond", false);
  constr->moveSecond = blk->getBool("moveSecond", true);
  constr->moveOnly = blk->getBool("moveOnly", false);
  constr->isSpring = blk->getBool("isSpring", false);
  return constr;
}

static Constraint *create_rigid_constraint(const DataBlock *blk, const NameMap &point_names, dag::ConstSpan<ParticlePoint *> points)
{
  const char *pivotName = blk->getStr("pivot", NULL);
  const char *attachedName = blk->getStr("attached", NULL);
  int pivotId = point_names.getNameId(pivotName);
  int attachedId = point_names.getNameId(attachedName);
  G_ASSERTF(pivotId >= 0 || blk->getBool("optional", false), "Cannot find '%s' point for rigid constraint", pivotName);
  G_ASSERTF(attachedId >= 0 || blk->getBool("optional", false), "Cannot find '%s' point for rigid constraint", attachedName);
  if (pivotId < 0 || attachedId < 0)
    return NULL;

  RigidConstraint *constr = new RigidConstraint(points[pivotId], points[attachedId]);
  return constr;
}

static Constraint *create_slider_constraint(const DataBlock *blk, const NameMap &point_names, dag::ConstSpan<ParticlePoint *> points)
{
  const char *pointName = blk->getStr("point", NULL);
  const char *fromName = blk->getStr("from", NULL);
  const char *toName = blk->getStr("to", NULL);
  int pointId = point_names.getNameId(pointName);
  int fromId = point_names.getNameId(fromName);
  int toId = point_names.getNameId(toName);
  G_ASSERTF(pointId >= 0, "Cannot find '%s' point for slider constraint", pointName);
  G_ASSERTF(fromId >= 0, "Cannot find '%s' point for slider constraint", fromName);
  G_ASSERTF(toId >= 0, "Cannot find '%s' point for slider constraint", toName);
  if (pointId < 0 || fromId < 0 || toId < 0)
    return NULL;

  return new SliderConstraint(points[pointId], points[fromId], points[toId]);
}

static Constraint *create_direction_constraint(const DataBlock *blk, const NameMap &point_names,
  dag::ConstSpan<ParticlePoint *> points)
{
  const char *pointName = blk->getStr("point", NULL);
  const char *fromName = blk->getStr("from", NULL);
  const char *toName = blk->getStr("to", NULL);
  int pointId = point_names.getNameId(pointName);
  int fromId = point_names.getNameId(fromName);
  int toId = point_names.getNameId(toName);
  G_ASSERTF(pointId >= 0, "Cannot find '%s' point for direction constraint", pointName);
  G_ASSERTF(fromId >= 0, "Cannot find '%s' point for direction constraint", fromName);
  G_ASSERTF(toId >= 0, "Cannot find '%s' point for direction constraint", toName);
  if (pointId < 0 || fromId < 0 || toId < 0)
    return NULL;
  return new DirectionConstraint(points[pointId], points[fromId], points[toId]);
}

static Constraint *create_projection_constraint(const DataBlock *blk, const NameMap &point_names,
  dag::ConstSpan<ParticlePoint *> points)
{
  const char *pointName = blk->getStr("point", NULL);
  const char *fromName = blk->getStr("from", NULL);
  const char *toName = blk->getStr("to", NULL);
  int pointId = point_names.getNameId(pointName);
  int fromId = point_names.getNameId(fromName);
  int toId = point_names.getNameId(toName);
  G_ASSERTF_RETURN(pointId >= 0, NULL, "Cannot find '%s' point for projection constraint", pointName);
  G_ASSERTF_RETURN(fromId >= 0, NULL, "Cannot find '%s' point for projection constraint", fromName);
  G_ASSERTF_RETURN(toId >= 0, NULL, "Cannot find '%s' point for projection constraint", toName);

  ProjectionConstraint *constr = new ProjectionConstraint(points[pointId], points[fromId], points[toId]);
  constr->moveNodes = blk->getBool("moveNodes", true);
  constr->movePoint = blk->getBool("movePoint", true);
  constr->moveEdge = blk->getBool("moveEdge", false);
  constr->moveEdgeNodes = blk->getBool("moveEdgeNodes", true);
  return constr;
}

static Constraint *create_revolute_constraint(const DataBlock *blk, const NameMap &point_names, dag::ConstSpan<ParticlePoint *> points)
{
  const char *pointName = blk->getStr("point", NULL);
  const char *fromName = blk->getStr("from", NULL);
  const char *toName = blk->getStr("to", NULL);
  int pointId = point_names.getNameId(pointName);
  int fromId = point_names.getNameId(fromName);
  int toId = point_names.getNameId(toName);
  G_ASSERTF(pointId >= 0, "Cannot find '%s' point for revolute constraint", pointName);
  G_ASSERTF(fromId >= 0, "Cannot find '%s' point for revolute constraint", fromName);
  G_ASSERTF(toId >= 0, "Cannot find '%s' point for revolute constraint", toName);
  if (pointId < 0 || fromId < 0 || toId < 0)
    return NULL;
  return new RevoluteConstraint(points[pointId], points[fromId], points[toId]);
}

void ParticlePhysSystem::loadData(const DataBlock *blk, const GeomNodeTree *tree)
{
  int searchChildrenNid = blk->getNameId("searchChildren");
  int createOnEdgeNid = blk->getNameId("createOnEdge");
  int edgeNid = blk->getNameId("edge");
  int sliderNid = blk->getNameId("slider");
  int directionNid = blk->getNameId("direction");
  int projectionNid = blk->getNameId("projection");
  int revoluteNid = blk->getNameId("revolute");
  int rigidNid = blk->getNameId("rigid");

  const char *findPattern = blk->getStr("find", NULL);
  if (!findPattern)
    return;
  RegExp re;
  re.compile(findPattern, "");
  for (dag::Index16 nodeId(0), end(tree->nodeCount()); nodeId != end; ++nodeId)
  {
    if (!re.test(tree->getNodeName(nodeId)))
      continue;

    const DataBlock *pointsBlk = blk->getBlockByNameEx("points");
    bool allBlocksValid = true;
    NameMap particleNames;
    Tab<ParticlePoint *> points(framemem_ptr());
    points.reserve(pointsBlk->blockCount());
    for (int i = 0; i < pointsBlk->blockCount(); ++i)
    {
      const DataBlock *particleBlk = pointsBlk->getBlock(i);
      ParticlePoint *point = NULL;
      for (int j = 0; j < particleBlk->blockCount(); ++j)
      {
        const DataBlock *ruleBlock = particleBlk->getBlock(j);
        int blockNameId = ruleBlock->getBlockNameId();
        if (blockNameId == searchChildrenNid)
        {
          point = create_point_at_child(ruleBlock, tree, particleNames, points);
          break;
        }
        else if (blockNameId == createOnEdgeNid)
        {
          point = create_point_on_edge(ruleBlock, particleNames, points);
          break;
        }
      }
      if (!point)
        point = create_point_at_pivot(particleBlk, re, tree);
      if (!particleBlk->getBool("optional", false) && !point)
      {
        allBlocksValid = false;
        break;
      }
      if (point)
      {
        const char *particleName = particleBlk->getBlockName();
        G_ASSERT(particleNames.getNameId(particleName) < 0);
        particleNames.addNameId(particleName);
        points.push_back(point);
      }
    }
    if (!allBlocksValid)
    {
      clear_all_ptr_items_and_shrink(points);
      continue;
    }
    // Points valid, we need to go through constraints now
    const DataBlock *constraintsBlk = blk->getBlockByNameEx("constraints");
    Tab<Constraint *> constrs(framemem_ptr());
    constrs.reserve(constraintsBlk->blockCount());
    for (int i = 0; i < constraintsBlk->blockCount(); ++i)
    {
      const DataBlock *constrBlk = constraintsBlk->getBlock(i);
      int blockNameId = constrBlk->getBlockNameId();
      Constraint *constraint = NULL;
      if (blockNameId == edgeNid)
        constraint = create_edge_constraint(constrBlk, particleNames, points);
      else if (blockNameId == sliderNid)
        constraint = create_slider_constraint(constrBlk, particleNames, points);
      else if (blockNameId == directionNid)
        constraint = create_direction_constraint(constrBlk, particleNames, points);
      else if (blockNameId == projectionNid)
        constraint = create_projection_constraint(constrBlk, particleNames, points);
      else if (blockNameId == revoluteNid)
        constraint = create_revolute_constraint(constrBlk, particleNames, points);
      else if (blockNameId == rigidNid)
        constraint = create_rigid_constraint(constrBlk, particleNames, points);

      if (!constrBlk->getBool("optional", false) && !constraint)
      {
        allBlocksValid = false;
        break;
      }
      if (constraint)
        constrs.push_back(constraint);
    }
    if (!allBlocksValid || constrs.empty())
    {
      clear_all_ptr_items_and_shrink(points);
      clear_all_ptr_items_and_shrink(constrs);
      continue;
    }
    append_items(particles, points.size(), points.data());
    append_items(constraints, constrs.size(), constrs.data());
  }
}

void ParticlePhysSystem::loadFromBlk(const DataBlock *blk, const GeomNodeTree *tree)
{
  clear_and_shrink(particles);
  clear_and_shrink(constraints);

  loadData(blk, tree);

  const int groupNid = blk->getNameId("group");
  for (int i = 0; i < blk->blockCount(); ++i)
    if (blk->getBlock(i)->getBlockNameId() == groupNid)
      loadData(blk->getBlock(i), tree);
}

void ParticlePhysSystem::update()
{
  const int maxNumIterations = 5;
  for (int iter = 0; iter < maxNumIterations; ++iter)
  {
    bool haveChanged = false;
    for (int i = 0; i < constraints.size(); ++i)
      haveChanged |= constraints[i]->solve();
    if (!haveChanged)
      return;
  }
}

void ParticlePhysSystem::updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm)
{
  for (int i = 0; i < constraints.size(); ++i)
    constraints[i]->updateGeomNodeTree(tree, render_space_tm);
}

void ParticlePhysSystem::renderDebug(const TMatrix &render_space_tm) const
{
  for (int i = 0; i < constraints.size(); ++i)
    constraints[i]->renderDebug(render_space_tm);
}
