//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_boundingSphere.h>
#include <math/dag_capsule.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug3d.h>
#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <util/dag_fastIntList.h>


namespace collisionpreview
{
struct LightMesh;
}
DAG_DECLARE_RELOCATABLE(collisionpreview::LightMesh);

namespace collisionpreview
{
struct LightMesh
{
  LightMesh() : v(midmem), edge(midmem) {}
  ~LightMesh() { clear(); }

  inline void clear()
  {
    edge.clear();
    v.clear();
  }

  Tab<Point3> v;
  EdgeTab<Edge> edge;
};

struct Sphere3
{
  real r;
  Point3 c;
};

struct Caps
{
  Point3 begin, end;
  real r;
};

struct Collision
{
  Collision() : ltMesh(midmem), box(midmem), sphere(midmem), capsule(midmem) {}

  void clear()
  {
    capsule.clear();
    ltMesh.clear();
    sphere.clear();
    box.clear();
  }

  Tab<LightMesh> ltMesh;
  Tab<TMatrix> box;
  Tab<Sphere3> sphere;
  Tab<Caps> capsule;
};


static inline float getMax3(float v_1, float v_2, float v_3)
{
  if (v_2 > v_1)
    v_1 = v_2;
  return (v_1 > v_3) ? v_1 : v_3;
}

static inline void addMeshCollision(LightMesh &m, const Mesh &mesh, const TMatrix &wtm)
{
  int vertCnt = mesh.getVertCount();
  m.v.resize(vertCnt);
  for (int i = 0; i < vertCnt; i++)
    m.v[i] = wtm * mesh.getVert()[i];

  int faceCnt = mesh.getFaceCount();
  m.edge.reserve(faceCnt * 3);
  if (vertCnt < 65536)
  {
    static FastIntList pairs;
    pairs.reset();
    for (int i = 0; i < faceCnt; i++)
    {
      const Face &face = mesh.getFace()[i];
      int v0 = face.v[0], v1 = face.v[1], v2 = face.v[2];

      if ((v0 == v1) | (v0 == v2) | (v1 == v2))
        continue;

#define PAIR(a, b) ((a < b ? a : b) << 16) | (a < b ? b : a)
      if (pairs.addInt(PAIR(v0, v1)))
        m.edge.push_back().set(v0, v1);
      if (pairs.addInt(PAIR(v1, v2)))
        m.edge.push_back().set(v1, v2);
      if (pairs.addInt(PAIR(v0, v2)))
        m.edge.push_back().set(v0, v2);
#undef PAIR
    }
    m.edge.shrink_to_fit();
    return;
  }

  for (int i = 0; i < faceCnt; i++)
  {
    const Face &face = mesh.getFace()[i];

    m.edge.add(Edge(face.v[0], face.v[1]));
    m.edge.add(Edge(face.v[1], face.v[2]));
    m.edge.add(Edge(face.v[2], face.v[0]));
  }
  m.edge.shrink_to_fit();
}

template <typename LightMesh, typename LandRayTracer>
static inline void addLandRtMeshCellCollision(LightMesh &m, const LandRayTracer &lrt, int cell_idx)
{
  if (cell_idx < 0 || cell_idx >= lrt.getCellCount())
    return;

  dag::ConstSpan<typename LandRayTracer::Vertex> verts = lrt.getCellVerts(cell_idx);
  dag::ConstSpan<uint16_t> indices = lrt.getCellFaces(cell_idx);
  vec4f c_scale = lrt.getCellPackScale(cell_idx), c_ofs = lrt.getCellPackOffset(cell_idx);

  m.v.resize(verts.size());
  Point3_vec4 p3;
  for (int i = 0; i < verts.size(); i++)
  {
    vec4i verti = v_lduush(verts[i].v);
    v_st(&p3.x, v_madd(v_cvt_vec4f(verti), c_scale, c_ofs));
    m.v[i] = p3;
  }

  static FastIntList pairs;
  pairs.reset();
  m.edge.reserve(indices.size());
  for (int i = 0; i < indices.size(); i += 3)
  {
    int v0 = indices[i], v1 = indices[i + 1], v2 = indices[i + 2];

    if ((v0 == v1) | (v0 == v2) | (v1 == v2))
      continue;

#define PAIR(a, b) ((a < b ? a : b) << 16) | (a < b ? b : a)
    if (pairs.addInt(PAIR(v0, v1)))
      m.edge.push_back().set(v0, v1);
    if (pairs.addInt(PAIR(v1, v2)))
      m.edge.push_back().set(v1, v2);
    if (pairs.addInt(PAIR(v0, v2)))
      m.edge.push_back().set(v0, v2);
#undef PAIR
  }
  m.edge.shrink_to_fit();
}
template <typename Collision, typename LandRayTracer>
static inline void addLandRtMeshCollision(Collision &c, const LandRayTracer *lrt)
{
  if (!lrt)
    return;
  for (int i = 0; i < lrt->getCellCount(); i++)
    addLandRtMeshCellCollision(c.ltMesh.push_back(), *lrt, i);
}

static inline void addBoxCollision(TMatrix &box_tm, const Mesh &m, const TMatrix &wtm)
{
  BBox3 box;
  int vertCnt = m.getVert().size();
  for (int vi = 0; vi < vertCnt; vi++)
    box += m.getVert()[vi];

  Point3 w = box.width();
  Point3 ax = wtm.getcol(0) * w.x;
  Point3 ay = wtm.getcol(1) * w.y;
  Point3 az = wtm.getcol(2) * w.z;

  box_tm.setcol(0, ax);
  box_tm.setcol(1, ay);
  box_tm.setcol(2, az);
  box_tm.setcol(3, wtm * box.center() - (ax + ay + az) / 2.f);
}

static inline void addSphereCollision(Sphere3 &sphere, const Mesh &m, const TMatrix &wtm)
{
  BSphere3 sph = ::mesh_bounding_sphere(m.getVert().data(), m.getVert().size());

  float rs = sqrtf(getMax3(lengthSq(wtm.getcol(0)), lengthSq(wtm.getcol(1)), lengthSq(wtm.getcol(2))));

  sphere.c = wtm * sph.c;
  sphere.r = rs * sph.r;
}

static inline void addCapsuleCollision(Caps &capsule, const Mesh &m, const TMatrix &wtm)
{
  BBox3 box;

  int vertCnt = m.getVert().size();
  for (int vi = 0; vi < vertCnt; vi++)
    box += m.getVert()[vi];

  Capsule c;
  c.set(box);
  c.transform(wtm);

  capsule.r = c.r;
  capsule.begin = c.a;
  capsule.end = c.b;
}


static void assembleCollisionFromNodes(Collision &c, const Tab<StaticGeometryNode *> &nodes, bool req_collidable_prop = true)
{
  c.clear();

  int cnt = nodes.size();
  for (int i = 0; i < cnt; i++)
  {
    StaticGeometryNode &n = *nodes[i];

    if (!nodes[i]->script.getBool("collidable", false))
    {
      if (req_collidable_prop)
        continue;
      if (!(nodes[i]->flags & StaticGeometryNode::FLG_COLLIDABLE))
        continue;
    }

    const char *primitive = nodes[i]->script.getStr("collision", NULL);
    if (!primitive || (stricmp(primitive, "mesh") == 0))
      addMeshCollision(c.ltMesh.push_back(), n.mesh->mesh, n.wtm);
    else if (stricmp(primitive, "box") == 0)
      addBoxCollision(c.box.push_back(), n.mesh->mesh, n.wtm);
    else if (stricmp(primitive, "sphere") == 0)
      addSphereCollision(c.sphere.push_back(), n.mesh->mesh, n.wtm);
    else if (stricmp(primitive, "capsule") == 0)
      addCapsuleCollision(c.capsule.push_back(), n.mesh->mesh, n.wtm);
    else
      DAEDITOR3.conWarning("unknown collision type: <%s>, ignored", primitive);
  }
}

static bool haveCollisionPreview(const Collision &collision)
{
  return collision.ltMesh.size() + collision.box.size() + collision.sphere.size() + collision.capsule.size() > 0;
}

static void drawCollisionPreview(const Collision &collision, const TMatrix &wtm, const E3DCOLOR &color)
{
  set_cached_debug_lines_wtm(wtm);

  int meshCnt = collision.ltMesh.size();
  for (int l = 0; l < meshCnt; l++)
  {
    const LightMesh &mesh = collision.ltMesh[l];

    int linkCnt = mesh.edge.size();
    for (int i = 0; i < linkCnt; i++)
      draw_cached_debug_line(mesh.v[mesh.edge[i].v0], mesh.v[mesh.edge[i].v1], color);
  }

  int boxCnt = collision.box.size();
  for (int l = 0; l < boxCnt; l++)
  {
    const TMatrix &m = collision.box[l];
    draw_cached_debug_box(m.getcol(3), m.getcol(0), m.getcol(1), m.getcol(2), color);
  }

  if (collision.sphere.size())
  {
    int sphCnt = collision.sphere.size();
    for (int l = 0; l < sphCnt; l++)
      draw_cached_debug_sphere(collision.sphere[l].c, collision.sphere[l].r, color);
  }

  for (int l = 0; l < collision.capsule.size(); l++)
  {
    Capsule c;
    c.set(collision.capsule[l].begin, collision.capsule[l].end, collision.capsule[l].r);
    draw_cached_debug_capsule_w(c, color);
  }
}
}; // namespace collisionpreview
