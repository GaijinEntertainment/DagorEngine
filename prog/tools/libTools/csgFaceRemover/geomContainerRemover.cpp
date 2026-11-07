// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/csgFaceRemover/geomContainerRemover.h>
#include <libTools/meshCSG/meshCSG.h>
#include <daBVH/dag_meshBLAS.h>
#include <daBVH/dag_swBLAS_leaf.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <dag/dag_vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <math/dag_bounds3.h>
#include <math/dag_math3d.h>
#include <math/dag_mesh.h>
#include <memory/dag_framemem.h>
#include <util/dag_bitArray.h>
#include <util/dag_string.h>
#include <EASTL/sort.h>

// watertight kernel: RayTriangleIntersectInfXZ is sign-exact, so only a crossed triangle answers
// top and bottom_y are in the frame the BLAS was built from, and top is above the mesh
// a closed surface alternates facing along a line, so a same-facing run is one crossing;
// the hits at one height are one contact, a touch when their facings differ
static bool inside_by_parity(const MeshBLAS &blas, const Point3 &top, float bottom_y)
{
  const vec3f org = v_ldu_p3_safe(&top.x);
  const float len = top.y - bottom_y;
  const BlasBoxRay boxRay = BlasBoxRay::make(org, v_make_vec4f(0.f, -1.f, 0.f, 0.f), blas.scale, blas.ofs);
  dag::RelocatableFixedVector<Point2, 32, true, framemem_allocator> hits; // height, facing along -y
  BLASTraverse<>::iterateFilteredVerts(
    blas.data.data(), 0, blas.blasSize, [&](vec3f bmn, vec3f bmx) { return boxRay.admits(bmn, bmx, len); },
    [&](vec3f v0, vec3f v1, vec3f v2, int) {
      float y;
      Point2 bc;
      if (RayTriangleIntersectInfXZ(org, v0, v1, v2, y, bc) && y > bottom_y) // top is above the mesh, so no hit lies above it
        hits.push_back(Point2(y, v_extract_y(v_cross3(v_sub(v1, v0), v_sub(v2, v0))) > 0.f ? -1.f : 1.f));
      return false; // every leaf, so no crossing is missed
    },
    Vert21MetricLoader::fromPack(blas.scale, blas.ofs));
  eastl::sort(hits.begin(), hits.end(), [](const Point2 &a, const Point2 &b) { return a.x < b.x; });
  int crossings = 0;
  float runFacing = 0.f;
  for (int i = 0, j; i < hits.size(); i = j)
  {
    bool touch = false;
    for (j = i + 1; j < hits.size() && hits[j].x == hits[i].x; ++j)
      touch |= hits[j].y != hits[i].y;
    if (!touch && hits[i].y != runFacing)
    {
      ++crossings;
      runFacing = hits[i].y;
    }
  }
  return crossings & 1;
}

// watertight kernel: RayTriangleIntersectWT holds a shared edge's lane as one exact value on both faces,
// up to the sign their windings give, so a segment past an edge answers from at least one face where the
// third corners lie on opposite sides of it, and from both or neither where they lie on the same side,
// crossing two faces or none
// from and len are in the frame the BLAS was built from, and dir is unit, so the
// kernel's t reads as a length against len
static bool any_crossing(const MeshBLAS &blas, const Point3 &from, const Point3 &dir, float len)
{
  const vec3f org = v_ldu_p3_safe(&from.x), d = v_ldu_p3_safe(&dir.x); // from can be the last heap Point3
  const BlasBoxRay boxRay = BlasBoxRay::make(org, d, blas.scale, blas.ofs);
  return BLASTraverse<>::iterateFilteredVerts(
    blas.data.data(), 0, blas.blasSize, [&](vec3f bmn, vec3f bmx) { return boxRay.admits(bmn, bmx, len); },
    [&](vec3f v0, vec3f v1, vec3f v2, int) { return RayTriangleIntersectWT(org, d, len, v0, v1, v2); },
    Vert21MetricLoader::fromPack(blas.scale, blas.ofs));
}

// center frame: pack rounding follows extent
static Point3 local_point(const Point3 &p, const DPoint3 &origin)
{
  return Point3(float(p.x - origin.x), float(p.y - origin.y), float(p.z - origin.z));
}

static bool build_union_blas(const dacsg::Mesh &m, MeshBLAS &blas, DPoint3 &origin)
{
  if (!m.faceCount())
    return false;
  BBox3 box;
  for (const Point3 &p : m.pos)
    box += p;
  origin = DPoint3(box.center());
  dag::Vector<Point3, framemem_allocator> verts;
  verts.resize_noinit(m.pos.size());
  for (uint32_t i = 0; i < m.pos.size(); ++i)
    verts[i] = local_point(m.pos[i], origin);
  return build_mesh_blas(blas, make_span_const(verts), make_span_const(m.tri));
}

// inside: corners by parity, no edge crossing
static void clear_inside_faces(const MeshBLAS &blas, const DPoint3 &origin, const Mesh &mesh, const TMatrix &wtm, Bitarray &used)
{
  const BBox3 &box = blas.box;
  const float maxHeight = box[1].y + 0.5f;
  dag::Vector<Point3, framemem_allocator> verts;
  verts.resize_noinit(mesh.vert.size());
  Bitarray in;
  in.resize(mesh.vert.size());
  in.reset();
  for (int vi = 0; vi < mesh.vert.size(); ++vi)
  {
    const Point3 v = verts[vi] = local_point(wtm * mesh.vert[vi], origin);
    if ((box & v) && inside_by_parity(blas, Point3::xVz(v, maxHeight), v.y))
      in.set(vi);
  }
  for (int f = 0; f < mesh.face.size(); ++f)
  {
    const uint32_t *fv = mesh.face[f].v;
    if (!in.get(fv[0]) || !in.get(fv[1]) || !in.get(fv[2]))
      continue;
    bool crossed = false;
    for (int e = 0; e < 3 && !crossed; ++e)
    {
      const Point3 &v0 = verts[fv[e]];
      const Point3 edge = verts[fv[(e + 1) % 3]] - v0;
      const float len = edge.length();
      crossed = len > 0.f && any_crossing(blas, v0, edge / len, len * 1.0001f);
    }
    if (!crossed)
      used.reset(f);
  }
}

struct CSGNode
{
  StaticGeometryNode *node;
  BBox3 box;
};

// refused nodes: never cut, only cleared
static int remove_in_chain(dag::ConstSpan<CSGNode> polys, dag::ConstSpan<int> chain, RemoverNotificationCB *cb)
{
  Tab<mesh_csg::Input> inputs(tmpmem);
  for (int k : chain)
  {
    const StaticGeometryNode &node = *polys[k].node;
    inputs.push_back(mesh_csg::Input{&node.mesh->mesh, node.wtm, node.name.str()});
  }
  mesh_csg::Result r;
  const bool ok = mesh_csg::evaluate(inputs, mesh_csg::Op::Union, {}, r);
  if (cb)
    for (const eastl::string &w : r.warnings)
      cb->note(String(0, "CSG: %s", w.c_str()));
  if (!ok)
  {
    if (cb)
      cb->warning(String(0, "CSG: no faces removed from %d nodes: %s", chain.size(), r.error.c_str()));
    return 0;
  }

  MeshBLAS blas;
  DPoint3 origin(0, 0, 0);
  bool blasTried = false, blasBuilt = false;
  int removed = 0;
  for (int j = 0; j < chain.size(); ++j)
  {
    StaticGeometryNode &node = *polys[chain[j]].node;
    Mesh &mesh = node.mesh->mesh;
    Bitarray used;
    mesh_csg::surface_faces(r, j, mesh.face.size(), used);
    if (!r.refused[j].empty())
    {
      if (cb)
        cb->warning(String(0, "CSG: node %s is not cut, only faces in accepted nodes go: %s", inputs[j].name, r.refused[j].c_str()));
      if (!blasTried)
      {
        blasTried = true;
        blasBuilt = build_union_blas(r.mesh, blas, origin);
      }
      if (blasBuilt)
        clear_inside_faces(blas, origin, mesh, node.wtm, used);
    }
    const int before = mesh.face.size();
    mesh.removeFacesFast(used);
    removed += before - mesh.face.size();
  }
  return removed;
}

void remove_inside_faces(StaticGeometryContainer &cont, RemoverNotificationCB *cb)
{
  Tab<CSGNode> polys(tmpmem);
  polys.reserve(cont.nodes.size());
  if (cb)
    cb->note("Start CSG face removing");

  int removedFaces = 0, totalFaces = 0;
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    if (!node.mesh)
      continue;

    node.mesh->mesh.kill_unused_verts(0.0001f);
    node.mesh->mesh.kill_bad_faces();
    totalFaces += node.mesh->mesh.getFaceCount();
    if (node.mesh->getRefCount() > 1)
    {
      // clone mesh when it is used by more than one node
      StaticGeometryMesh *m = new StaticGeometryMesh;
      m->mats = node.mesh->mats;
      m->mesh = node.mesh->mesh;
      node.mesh = m;
    }
    BBox3 box;
    for (const Point3 &v : node.mesh->mesh.vert)
      box += node.wtm * v;
    polys.push_back(CSGNode{&node, box});
  }
  SmallTab<int, TmpmemAlloc> polysMainNode;
  clear_and_resize(polysMainNode, polys.size());
  mem_set_ff(polysMainNode);

  Tab<Tabint> polysLists(tmpmem);
  polysLists.reserve(polys.size());
  // form different chains of mutually intersecting nodes (potentially intersecting, via bbox)
  for (int i = 0; i < polys.size(); ++i)
  {
    if (polysMainNode[i] >= 0) // already in some chain
      continue;
    // start a new chain
    polysMainNode[i] = i;
    int newListId = append_items(polysLists, 1);
    Tabint &currentList = polysLists[newListId];
    currentList.push_back(i);
    for (;;)
    {
      bool nodeAdded = false;
      for (int j = 0; j < polys.size(); ++j)
      {
        if (polysMainNode[j] >= 0) // already in some chain
          continue;
        bool intersects = false;
        for (int k = 0; k < currentList.size(); ++k)
        {
          G_ASSERT(polysMainNode[currentList[k]] == i);
          if ((polys[currentList[k]].box & polys[j].box)) // intersecting nodes
          {
            intersects = true;
            break;
          }
        }
        if (intersects)
        {
          currentList.push_back(j);
          polysMainNode[j] = i;
          nodeAdded = true;
        }
      }
      if (!nodeAdded)
        break;
    }
  }

  for (const Tabint &chain : polysLists)
    if (chain.size() > 1)
      removedFaces += remove_in_chain(make_span_const(polys), make_span_const(chain), cb);

  if (cb)
  {
    cb->note(String(0, "Removed %d out of %d total faces (%d percent)", removedFaces, totalFaces,
      totalFaces ? removedFaces * 100 / totalFaces : 0));
    cb->note("End CSG face removing");
  }
}
