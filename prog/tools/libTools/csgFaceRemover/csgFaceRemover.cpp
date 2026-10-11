// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_mesh.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <util/dag_bitArray.h>

#include <carve/csg.hpp>
#include <carve/mesh.hpp>
#include <carve/tree.hpp>
#include <carve/interpolator.hpp>
#include <carve/csg_triangulator.hpp>
#include <math/dag_bounds3.h>
#include <libTools/csgFaceRemover/csgFaceRemover.h>
#include <debug/dag_debug.h>

#define REMOVE_OPEN_WITH_TRACE 1

#if REMOVE_OPEN_WITH_TRACE
#include <daBVH/dag_meshBLAS.h>
#include <daBVH/dag_swBLAS_leaf.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <dag/dag_vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <EASTL/sort.h>
#endif


// typedef carve::poly::Polyhedron carve_poly_t;
typedef carve::mesh::MeshSet<3> carve_poly_t;
typedef carve_poly_t::mesh_t mesh_t;
typedef carve::geom::vector<3> vector3;
typedef carve::math::Matrix matrix;
#if REMOVE_OPEN_WITH_TRACE
// every point enters one local frame in carve's doubles, so the pack's rounding is the extent's, not the world position's
static Point3 local_point(const vector3 &v, const vector3 &origin)
{
  return Point3(float(v.v[0] - origin.v[0]), float(v.v[1] - origin.v[1]), float(v.v[2] - origin.v[2]));
}

// the build drops the verts no face reaches, so the whole poly's list goes in unremapped
static bool build_carve_mesh_blas(MeshBLAS &out, dag::ConstSpan<Point3> poly_verts, const carve_poly_t *poly, const mesh_t *mesh)
{
  const mesh_t::vertex_t *const baseVert = &poly->vertex_storage[0];
  dag::Vector<uint32_t, framemem_allocator> idx;
  idx.reserve(mesh->faces.size() * 3);

  std::vector<mesh_t::vertex_t *> fverts;
  for (int fi = 0; fi < mesh->faces.size(); ++fi)
  {
    fverts.resize(0);
    mesh->faces[fi]->getVertices(fverts);
    G_ASSERT(fverts.size() == 3);
    for (int vi = 0; vi < 3; ++vi)
      idx.push_back((uint32_t)(fverts[vi] - baseVert));
  }
  return build_mesh_blas(out, poly_verts, make_span_const(idx));
}

// watertight kernel: RayTriangleIntersectInfXZ is sign-exact, so only a crossed triangle answers
// constraint: top and bottom_y are in the frame the BLAS was built from, and top is above the mesh
// constraint: a closed surface alternates facing along a line, so a same-facing run is one crossing;
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
      ++crossings, runFacing = hits[i].y;
  }
  return crossings & 1;
}

// watertight kernel: RayTriangleIntersectWT holds a shared edge's lane as one exact value on both faces,
// up to the sign their windings give, so a segment past an edge answers from at least one face where the
// third corners lie on opposite sides of it, and from both or neither where they lie on the same side,
// crossing two faces or none
// constraint: from and len are in the frame the BLAS was built from, and dir is unit, so the
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

static bool classify_scene_ray(carve_poly_t *poly, carve_poly_t *open, Bitarray *&openSkip)
{
  int openFacesCount = 0;
  for (int i = 0; i < open->meshes.size(); ++i)
    openFacesCount += open->meshes[i]->faces.size();

  const vector3 origin = poly->getAABB().pos;
  dag::Vector<Point3, framemem_allocator> polyVerts, openVerts;
  polyVerts.resize_noinit(poly->vertex_storage.size());
  for (int vi = 0; vi < poly->vertex_storage.size(); ++vi)
    polyVerts[vi] = local_point(poly->vertex_storage[vi].v, origin);
  openVerts.resize_noinit(open->vertex_storage.size());
  for (int vi = 0; vi < open->vertex_storage.size(); ++vi)
    openVerts[vi] = local_point(open->vertex_storage[vi].v, origin);

  for (int i = 0; i < poly->meshes.size(); ++i)
  {
    MeshBLAS blas;
    if (!build_carve_mesh_blas(blas, make_span_const(polyVerts), poly, poly->meshes[i]))
      continue;
    const BBox3 &box = blas.box;

    Bitarray vertsIn;
    vertsIn.resize(open->vertex_storage.size());
    vertsIn.reset();
    bool thereVertsIn = false;
    float maxHeight = box[1].y + 0.5f;
    for (int vi = 0; vi < open->vertex_storage.size(); ++vi)
    {
      const Point3 &v = openVerts[vi];
      if (!(box & v))
        continue;
      if (inside_by_parity(blas, Point3::xVz(v, maxHeight), v.y))
      {
        vertsIn.set(vi);
        thereVertsIn = true;
      }
    }
    if (!thereVertsIn)
      continue;

    std::vector<mesh_t::vertex_t *> fverts;
    const mesh_t::vertex_t *const baseVert = &open->vertex_storage[0];
    for (int openAll = 0, j = 0; j < open->meshes.size(); ++j)
    {
      for (int fi = 0; fi < open->meshes[j]->faces.size(); ++fi, ++openAll)
      {
        fverts.resize(0);
        open->meshes[j]->faces[fi]->getVertices(fverts);
        bool vertOut = false;
        for (int vi = 0; vi < fverts.size(); ++vi)
          if (!vertsIn.get(fverts[vi] - baseVert))
          {
            vertOut = true;
            break;
          }
        if (!vertOut)
        {
          for (int ei = 0; ei < 3; ++ei)
          {
            const Point3 &v0 = openVerts[fverts[ei] - baseVert], &v1 = openVerts[fverts[(ei + 1) % 3] - baseVert];
            const Point3 edge = v1 - v0;
            float len = edge.length();
            if (len > 0.f && any_crossing(blas, v0, edge / len, len * 1.0001f))
            {
              vertOut = true;
              break;
            }
          }
        }
        if (!vertOut)
        {
          if (!openSkip)
          {
            openSkip = new Bitarray();
            openSkip->resize(openFacesCount);
            openSkip->reset();
          }
          openSkip->set(openAll);
        }
      }
    }
  }
  return openSkip != nullptr;
}
#endif

class PerformCSG : public BaseCSGRemoval
{
  carve::interpolate::FaceAttr<uint64_t> f_id;

  carve::csg::CSG csg;
  carve::csg::CSG_TreeNode *left;

public:
  static void delete_mesh(carve_poly_t *&poly)
  {
    delete poly;
    poly = NULL;
  }
  struct CSGNode
  {
    carve_poly_t *poly;      // closed poly
    carve_poly_t *open_poly; // open
    Bitarray *openSkip;
    bool hasOpenNodes;
    bool isResultOfOp;
    bool sliced;
    CSGNode(carve_poly_t *p, bool isUnion, carve_poly_t *op) :
      poly(p), isResultOfOp(isUnion), open_poly(op), sliced(false), openSkip(NULL)
    {}
    ~CSGNode()
    {
      delete_mesh(poly);
      delete_mesh(open_poly);
      del_it(openSkip);
    }
  };
  PerformCSG()
  {
    f_id.installHooks(csg);
    left = NULL;
    // perform triangulation on every compute.
    // csg.hooks.registerHook(new carve::csg::CarveTriangulatorWithImprovement,//with improver triangulation is better - but we don't
    // care
    csg.hooks.registerHook(new carve::csg::CarveTriangulator, carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
    // csg.hooks.registerHook(new carve::csg::CarveTriangulator, carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
  }

  static carve_poly_t *my_clone(carve_poly_t *poly)
  {
    carve_poly_t *newPoly = new carve_poly_t(poly->vertex_storage, poly->meshes);
    poly->meshes.erase(poly->meshes.begin(), poly->meshes.end());
    return newPoly;
  }
  static carve_poly_t *meshset_from_meshes_ofmeshsets(carve_poly_t *poly, std::vector<mesh_t *> &meshes)
  {
    carve_poly_t *newPoly = new carve_poly_t(poly->vertex_storage, meshes);
    if (!poly->meshes.size())
      return newPoly;
    for (int j = 0; j < newPoly->meshes.size(); ++j)
    {
      for (std::vector<mesh_t *>::iterator i = poly->meshes.begin(); i != poly->meshes.end(); ++i)
      {
        if (*i == newPoly->meshes[j])
        {
          poly->meshes.erase(i);
          break;
        }
      }
    }
    return newPoly;
  }

  carve_poly_t *one_slice(carve_poly_t *closed, carve_poly_t *open, bool &has_cutted)
  {
    has_cutted = false;
    try
    {
      std::list<std::pair<carve::csg::FaceClass, carve_poly_t *>> open_sliced;
      csg.sliceAndClassify(closed, open, open_sliced, NULL);
      for (std::list<std::pair<carve::csg::FaceClass, carve_poly_t *>>::iterator i = open_sliced.begin(); i != open_sliced.end(); ++i)
      {
        if ((*i).first == carve::csg::FACE_IN) // remove only out
        {
          has_cutted = true;
          break;
        }
      }
      if (!has_cutted)
      {
        for (std::list<std::pair<carve::csg::FaceClass, carve_poly_t *>>::iterator i = open_sliced.begin(); i != open_sliced.end();
             ++i)
          delete_mesh((*i).second);
        return NULL;
      }
      carve_poly_t *result_open = NULL;
      for (std::list<std::pair<carve::csg::FaceClass, carve_poly_t *>>::iterator i = open_sliced.begin(); i != open_sliced.end(); ++i)
      {
        switch ((*i).first)
        {
          case carve::csg::FACE_IN: delete_mesh((*i).second); break; // remove only in
          case carve::csg::FACE_OUT:
          case carve::csg::FACE_ON_ORIENT_OUT:
          case carve::csg::FACE_ON_ORIENT_IN:
            if (!result_open)
              result_open = (*i).second;
            else
            {
              carve_poly_t *result2 = concatenate_meshes(result_open, (*i).second);
              delete_mesh(result_open);
              delete_mesh((*i).second);
              result_open = result2;
            }
            break;

          case carve::csg::FACE_UNCLASSIFIED:
          case carve::csg::FACE_ON: break;
        }
      }
      return result_open;
    }
    catch (const carve::exception &e)
    {
      debug("exception %s", (const char *)e.str().c_str());
      has_cutted = false;
      return NULL;
    }
    catch (...)
    {
      logerr("unknown exception");
      has_cutted = false;
      return NULL;
    }
  }
  static void delete_owned(carve_poly_t *&poly)
  {
    poly->meshes.erase(poly->meshes.begin(), poly->meshes.end());
    delete_mesh(poly);
  }
  carve_poly_t *accurate_slice(carve_poly_t *closed, carve_poly_t *open, bool &has_cutted)
  {
    carve_poly_t *result = NULL;
    std::vector<mesh_t *> closedMeshes(closed->meshes);

    bool ownedOpen = true;
    for (int j = 0; j < closedMeshes.size(); ++j)
    {
      closed->meshes = std::vector<mesh_t *>(1, closedMeshes[j]);
      bool has_cutted_this = false;
      carve_poly_t *new_open = one_slice(closed, open, has_cutted_this);
      if (has_cutted_this)
      {
        if (!ownedOpen)
          delete_mesh(open);
        result = new_open;
        open = result;
        ownedOpen = false;
      }
    }
    closed->meshes = closedMeshes;

    return result;
  }

  carve_poly_t *union_meshes(carve_poly_t *a, carve_poly_t *b)
  {
    if (!a && !b)
      return NULL;
    if (!a)
      return my_clone(b);
    if (!b)
      return my_clone(a);
    return csg.compute(a, b, carve::csg::CSG::UNION, NULL, carve::csg::CSG::CLASSIFY_EDGE); //
  }

  carve_poly_t *concatenate_meshes(carve_poly_t *a, carve_poly_t *b)
  {
    if (!a && !b)
      return NULL;
    if (!a)
      return my_clone(b);
    if (!b)
      return my_clone(a);

    return csg.compute(a, b, carve::csg::CSG::ALL, NULL, carve::csg::CSG::CLASSIFY_EDGE); //
    // std::vector<mesh_t *> meshes(a->meshes);
    // meshes.insert(meshes.end(), b->meshes.begin(), b->meshes.end());
    // return meshset_from_meshes_ofmeshsets(meshes);
  }

  void *create_poly(const TMatrix *wtm, const Mesh &mesh, int node_id, bool only_closed, BBox3 &box) override
  {
    carve_poly_t *poly = texturedMesh(mesh, wtm, node_id, box);
    bool isClosed = poly->isClosed();
    if (!isClosed)
    {
      return new CSGNode(NULL, false, poly);
      /*
      //splitting is not working correctly
      int openMeshes = 0;
      for (size_t i = 0; i < poly->meshes.size(); ++i)
        if (!poly->meshes[i]->isClosed())
          openMeshes++;
      if (openMeshes == poly->meshes.size())
        return new CSGNode(NULL, false, poly);

      std::vector<mesh_t *> open_meshes;
      std::vector<mesh_t *> closed_meshes;
      for (size_t i = 0; i < poly->meshes.size(); ++i)
        if (!poly->meshes[i]->isClosed())
          open_meshes.push_back(poly->meshes[i]);
        else
          closed_meshes.push_back(poly->meshes[i]);

      carve_poly_t * open_poly = meshset_from_meshes_ofmeshsets(poly, open_meshes);//!!

      poly->meshes = closed_meshes;
      debug("open=%d closed = %d", open_meshes.size(), poly->meshes.size());
      return new CSGNode(poly, false, open_poly);*/
    }
    return new CSGNode(poly, false, NULL);
  }
  bool hasOpenManifolds(void *poly)
  {
    if (!poly)
      return true;

    return ((CSGNode *)poly)->open_poly != NULL;
  }
  void delete_poly(void *poly) override
  {
    if (poly)
      delete ((CSGNode *)poly);
  }
  static inline matrix tm2matrix(const TMatrix &tm)
  {
    matrix m(tm.m[0][0], tm.m[1][0], tm.m[2][0], tm.m[3][0], tm.m[0][1], tm.m[1][1], tm.m[2][1], tm.m[3][1], tm.m[0][2], tm.m[1][2],
      tm.m[2][2], tm.m[3][2], 0, 0, 0, 1);
    return m;
  }
  void *makeUnion(void *a_, void *b_) override
  {
    if (!a_ || !b_)
      return NULL;
    CSGNode *a = ((CSGNode *)a_);
    CSGNode *b = ((CSGNode *)b_);
    carve_poly_t *result = NULL, *open_result = NULL;
    try
    {
      result = union_meshes(a->poly, b->poly);
      // carve::csg::CSG::CLASSIFY_NORMAL);//
    }
    catch (const carve::exception &e)
    {
      debug("exception %s", (const char *)e.str().c_str());
      return NULL;
    }
    try
    {
      open_result = concatenate_meshes(a->open_poly, b->open_poly);
    }
    catch (const carve::exception &e)
    {
      debug("exception %s", (const char *)e.str().c_str());
      return NULL;
    }
    if (!result && !open_result)
      return NULL;
    return new CSGNode(result, true, open_result);
  }

  /*void *slice(void *a_, void *b_)
  {
    carve_poly_t *a = (carve_poly_t *)a_;
    carve_poly_t *b = (carve_poly_t *)b_;
    std::list<std::pair<carve::csg::FaceClass, carve::mesh::MeshSet<3> *> > b_sliced;
    csg.sliceAndClassify(a, b, b_sliced);
  }*/

  /*void convert(void *in, Mesh& mesh)
  {
    convert(*(carve_poly_t *)in, mesh);
  }*/
  int mark_faces(Bitarray &used, carve_poly_t *a, int node_id, Bitarray *skip)
  {
    if (!a)
      return 0;
    int count = used.size();
    int setFaces = 0;
    for (int all = 0, i = 0; i < a->meshes.size(); ++i)
      for (int j = 0; j < a->meshes[i]->faces.size(); ++j, ++all)
      {
        if (skip && skip->get(all))
          continue;
        carve_poly_t::face_t *f = a->meshes[i]->faces[j];
        uint64_t fid = f_id.getAttribute(f, 0);
        if (((uint32_t)(fid >> 32)) != node_id)
          continue;
        fid &= 0xFFFFFFFF;
        G_ASSERT(fid < count);
        if (!used.get((int)fid))
        {
          used.set((int)fid);
          setFaces++;
        }
      }
    return setFaces;
  }
  int removeUnusedFaces(Mesh &mesh, void *in_, int node_id) override
  {
    if (!in_)
      return 0;
    CSGNode *node = (CSGNode *)in_;
    if (!node->sliced && node->poly && node->open_poly)
    {
#if REMOVE_OPEN_WITH_TRACE
      node->isResultOfOp |= classify_scene_ray(node->poly, node->open_poly, node->openSkip);
      node->sliced = true;
#else
      bool has_cutted = false;
      carve_poly_t *newOpenPoly = accurate_slice(node->poly, node->open_poly, has_cutted);
      node->sliced = true;
      if (has_cutted)
      {
        node->isResultOfOp = true;
        delete_mesh(node->open_poly);
        node->open_poly = newOpenPoly;
      }
#endif
    }
    if (!node->isResultOfOp)
      return 0;
    Bitarray used;
    used.resize(mesh.face.size());
    int removedFaces = mesh.face.size();
    used.reset();
    removedFaces -= mark_faces(used, node->poly, node_id, NULL);
    removedFaces -= mark_faces(used, node->open_poly, node_id, node->openSkip);

    mesh.removeFacesFast(used);
    return removedFaces;
  }

protected:
  /*void convert(const carve_poly_t &in, Mesh& mesh)
  {
    mesh.face.resize(in.faces.size());
    mesh.tface[0].resize(in.faces.size());
    mem_set_0(mesh.tface[0]);
    mesh.vert.resize(in.vertices.size());
    for (int i = 0; i < mesh.vert.size(); ++i)
      mesh.vert[i] = Point3(in.vertices[i].v[0],in.vertices[i].v[1],in.vertices[i].v[2]);
    for (int i = 0, l = in.faces.size(); i != l; ++i)
    {
      carve_poly_t::face_t &f = in.faces[i];

      G_ASSERT(f.vertices.size() == 3);
      for (size_t j = 0; j < f.vertices.size(); ++j)
      {
        int vi = in.vertexToIndex(f.vertices[j]);

        if (fv_tex.hasAttribute(&f, j))
        {
          Point2 tc = fv_tex.getAttribute(&f, j);
          mesh.tface[0][i].t[j] = mesh.tvert[0].size();
          mesh.tvert[0].push_back(tc, 1024);
        }
        mesh.face[i].v[j] = vi;
      }
      mesh.face[i].mat = f_tex_num.getAttribute(&f, 0);
      mesh.face[i].smgr = f_smgr_num.getAttribute(&f, 0);
    }
    mesh.optimize_tverts();
  }*/


  carve_poly_t *texturedMesh(const Mesh &mesh, const TMatrix *wtm, int node_id, BBox3 &box)
  {
    box.setempty();

    std::vector<carve_poly_t::vertex_t> v;
    v.resize(mesh.vert.size());
    for (int i = 0; i < mesh.vert.size(); ++i)
    {
      Point3 mv;
      if (wtm)
        mv = (*wtm) * mesh.vert[i];
      else
        mv = mesh.vert[i];
      v[i] = carve::geom::VECTOR(mv.x, mv.y, mv.z);
      box += mv;
    }

    std::vector<carve_poly_t::face_t *> faces;
    faces.resize(mesh.face.size());
    for (int i = 0; i < mesh.face.size(); ++i)
      faces[i] = new carve_poly_t::face_t(&v[mesh.face[i].v[0]], &v[mesh.face[i].v[1]], &v[mesh.face[i].v[2]]);


    uint64_t nodeId = node_id;
    nodeId <<= 32;
    for (int i = 0; i < mesh.face.size(); ++i)
      f_id.setAttribute(faces[i], nodeId | ((uint64_t)i));

    carve_poly_t *poly = new carve_poly_t(faces);

    return poly;
  }
};

BaseCSGRemoval *make_new_csg_removal() { return new PerformCSG(); }

void delete_csg(BaseCSGRemoval *csg) { delete ((PerformCSG *)csg); }
