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
#include <libtools/csgFaceRemover/csgFaceRemover.h>
#include <debug/dag_debug.h>

#define REMOVE_OPEN_WITH_TRACE 1

#if REMOVE_OPEN_WITH_TRACE
#include <sceneRay/dag_sceneRay.h>
#endif


// typedef carve::poly::Polyhedron carve_poly_t;
typedef carve::mesh::MeshSet<3> carve_poly_t;
typedef carve_poly_t::mesh_t mesh_t;
typedef carve::geom::vector<3> vector3;
typedef carve::math::Matrix matrix;
#if REMOVE_OPEN_WITH_TRACE
template <class T>
__forceinline void swap(T &a, T &b)
{
  T c = a;
  a = b;
  b = c;
}

static bool classify_scene_ray(carve_poly_t *poly, carve_poly_t *open, Bitarray *&openSkip)
{
  SmallTab<unsigned int, TmpmemAlloc> face;
  SmallTab<Point3, TmpmemAlloc> verts;

  clear_and_resize(verts, poly->vertex_storage.size());
  for (int vi = 0; vi < verts.size(); ++vi)
    verts[vi] = Point3(poly->vertex_storage[vi].v.v[0], poly->vertex_storage[vi].v.v[1], poly->vertex_storage[vi].v.v[2]);

  int openFacesCount = 0;
  for (int i = 0; i < open->meshes.size(); ++i)
    openFacesCount += open->meshes[i]->faces.size();
  int removeFaces = 0;

  for (int i = 0; i < poly->meshes.size(); ++i)
  {
    mesh_t::aabb_t aabb = poly->meshes[i]->getAABB();
    mesh_t::aabb_t::vector_t width = aabb.max() - aabb.min();
    Point3 meshWidth(width.v[0], width.v[1], width.v[2]);
    BuildableStaticSceneRayTracer *brt = create_buildable_staticmeshscene_raytracer(meshWidth / 16.f, 8);

    clear_and_resize(face, poly->meshes[i]->faces.size() * 3);
    std::vector<mesh_t::vertex_t *> fverts;
    const mesh_t::vertex_t *const baseVert = &poly->vertex_storage[0];
    for (int fi = 0; fi < poly->meshes[i]->faces.size(); ++fi)
    {
      fverts.resize(0);
      poly->meshes[i]->faces[fi]->getVertices(fverts);
      G_ASSERT(fverts.size() == 3);
      face[fi * 3 + 0] = fverts[0] - baseVert;
      face[fi * 3 + 1] = fverts[1] - baseVert;
      face[fi * 3 + 2] = fverts[2] - baseVert;
    }
    brt->addmesh(verts.data(), verts.size(), face.data(), sizeof(int) * 3, face.size() / 3, NULL, true);
    brt->setCullFlags(brt->CULL_BOTH);


    Bitarray vertsIn;
    vertsIn.resize(open->vertex_storage.size());
    vertsIn.reset();
    bool thereVertsIn = false;
    float maxHeight = brt->getBox()[1].y + 0.5f;
    for (int vi = 0; vi < open->vertex_storage.size(); ++vi)
    {
      Point3 v(open->vertex_storage[vi].v.v[0], open->vertex_storage[vi].v.v[1], open->vertex_storage[vi].v.v[2]);
      if (!(brt->getBox() & v))
        continue;
      int intersections = 0;
      Point3 startV = Point3::xVz(v, maxHeight);
      float t = startV.y - v.y;
      int lastIntersectFace = -1;
      for (int iteration = 0; iteration < 128; ++iteration) // maxiteration limit!
      {
        int fi;
        // if ((fi = brt->getHeightBelow(startV, ht)) >= 0)//classify with raytracer
        if ((fi = brt->tracerayNormalized(startV, Point3(0, -1, 0), t, lastIntersectFace)) >= 0) // classify with raytracer
        {
          if (lastIntersectFace != fi)
          {
            intersections++;
            lastIntersectFace = fi;
          }
          startV.y -= t;
          t = startV.y - v.y;
        }
        else
          break;
      }

      if (intersections & 1)
      {
        vertsIn.set(vi);
        thereVertsIn = true;
      }
    }
    if (thereVertsIn)
    {
      for (int openAll = 0, j = 0; j < open->meshes.size(); ++j)
      {
        std::vector<mesh_t::vertex_t *> fverts;
        const mesh_t::vertex_t *const baseVert = &open->vertex_storage[0];
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
            for (int ei = 0; ei < 3; ++ei) // check edges
            {
              Point3 v0(fverts[ei]->v.v[0], fverts[ei]->v.v[1], fverts[ei]->v.v[1]),
                v1(fverts[(ei + 1) % 3]->v.v[0], fverts[(ei + 1) % 3]->v.v[1], fverts[(ei + 1) % 3]->v.v[1]);
              float t = 1.0001f;
              if (brt->traceray(v0, v1 - v0, t) >= 0)
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
            removeFaces++;
          }
        }
      }
    }
    delete brt;
  }
  if (!removeFaces)
    del_it(openSkip);
  return removeFaces > 0;
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
#if REMOVE_OPEN_WITH_TRACE
    SmallTab<StaticSceneRayTracer *, TmpmemAlloc> sceneRay;
#endif
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
#if REMOVE_OPEN_WITH_TRACE
      for (int i = 0; i < sceneRay.size(); ++i)
        del_it(sceneRay[i]);
#endif
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
        switch ((*i).first)
        {
          case carve::csg::FACE_IN: has_cutted = true; break; // remove only out
        }
        if (has_cutted)
          break;
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

  void *create_poly(const TMatrix *wtm, const Mesh &mesh, int node_id, bool only_closed, BBox3 &box)
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
  void delete_poly(void *poly)
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
  void *makeUnion(void *a_, void *b_)
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
  int removeUnusedFaces(Mesh &mesh, void *in_, int node_id)
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

BaseCSGRemoval *make_new_csg() { return new PerformCSG(); }

void delete_csg(BaseCSGRemoval *csg) { delete ((PerformCSG *)csg); }
