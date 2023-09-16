#include <math/dag_mesh.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <util/dag_bitArray.h>

#include <carve/csg.hpp>
#include <carve/mesh.hpp>
#include <carve/tree.hpp>
#include <carve/interpolator.hpp>
#include <carve/csg_triangulator.hpp>

#include "csg.h"

// typedef carve::poly::Polyhedron carve_poly_t;
typedef carve::mesh::MeshSet<3> carve_poly_t;
typedef carve::geom::vector<3> vector3;
typedef carve::math::Matrix matrix;

struct Point20 : public Point2
{
  Point20() { x = y = 0; }
  Point20(const Point2 &t) : Point2(t) {}
};

class PerformCSG : public BaseCSG
{
  carve::interpolate::FaceVertexAttr<Point20> fv_tex[NUMMESHTCH];
  int tiUsed;
  carve::interpolate::FaceAttr<uint64_t> f_id;
  carve::interpolate::FaceAttr<int> f_tex_num;
  carve::interpolate::FaceAttr<unsigned> f_smgr_num;

  carve::csg::CSG csg;
  carve::csg::CSG_TreeNode *left;

public:
  PerformCSG(int tc)
  {
    tiUsed = tc;
    for (int i = 0; i < tiUsed; ++i)
      fv_tex[i].installHooks(csg);
    f_tex_num.installHooks(csg);
    f_id.installHooks(csg);
    f_smgr_num.installHooks(csg);
    left = NULL;
    // perform triangulation on every compute.
    csg.hooks.registerHook(new carve::csg::CarveTriangulatorWithImprovement, carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
    // csg.hooks.registerHook(new carve::csg::CarveTriangulator, carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
  }
  void *create_poly(const TMatrix *wtm, const Mesh &mesh, int node_id, bool only_closed, BBox3 &box)
  {
    carve_poly_t *poly = texturedMesh(mesh, wtm, node_id, box);
    if (only_closed && !poly->isClosed())
    {
      printf("not closed mesh!\n");
      delete poly;
      return NULL;
    }
    return poly;
  }
  bool hasOpenManifolds(void *poly)
  {
    if (!poly)
      return true;

    return !(((carve_poly_t *)poly)->isClosed());
  }
  void delete_poly(void *poly)
  {
    if (poly)
      delete ((carve_poly_t *)poly);
  }
  static inline matrix tm2matrix(const TMatrix &tm)
  {
    matrix m(tm.m[0][0], tm.m[1][0], tm.m[2][0], tm.m[3][0], tm.m[0][1], tm.m[1][1], tm.m[2][1], tm.m[3][1], tm.m[0][2], tm.m[1][2],
      tm.m[2][2], tm.m[3][2], 0, 0, 0, 1);
    return m;
  }
  static carve::csg::CSG::OP csg_type(OpType op_tp)
  {
    switch (op_tp)
    {
      case UNION: return carve::csg::CSG::UNION;
      case INTERSECTION: return carve::csg::CSG::INTERSECTION;
      case A_MINUS_B: return carve::csg::CSG::A_MINUS_B;
      case B_MINUS_A: return carve::csg::CSG::B_MINUS_A;
      case SYMMETRIC_DIFFERENCE: return carve::csg::CSG::SYMMETRIC_DIFFERENCE;
    };
    return carve::csg::CSG::UNION;
  }
  void *op(OpType op_tp, void *a_, const TMatrix *atm, void *b_, const TMatrix *btm)
  {
    try
    {
      carve_poly_t *a = (carve_poly_t *)a_;
      carve_poly_t *b = (carve_poly_t *)b_;
      if (atm)
      {
        // transform a
        a->transform(carve::math::matrix_transformation(tm2matrix(*atm)));
      }
      if (btm)
        // transform b
        b->transform(carve::math::matrix_transformation(tm2matrix(*btm)));
      G_ASSERT(a);
      G_ASSERT(b);
      carve_poly_t *result = csg.compute(a, b, csg_type(op_tp), NULL,
        carve::csg::CSG::CLASSIFY_EDGE); // carve::csg::CSG::CLASSIFY_NORMAL);//
      return result;
    }
    catch (const carve::exception &ex)
    {
      printf("Error in performing operation '%s'\n", ex.str().c_str());
      return NULL;
    }
  }
  bool slice_and_classify(void *open_, const TMatrix *openTM, void *closed_, const TMatrix *closedTM, void *&open_in,
    void *&open_on_in, void *&open_out, void *&open_on_out)
  {
    try
    {
      carve_poly_t *open = (carve_poly_t *)open_;
      carve_poly_t *closed = (carve_poly_t *)closed_;
      if (openTM)
      {
        // transform a
        open->transform(carve::math::matrix_transformation(tm2matrix(*openTM)));
      }
      if (closedTM)
        // transform b
        closed->transform(carve::math::matrix_transformation(tm2matrix(*closedTM)));

      std::list<std::pair<carve::csg::FaceClass, carve::mesh::MeshSet<3> *>> open_sliced;
      csg.sliceAndClassify(closed, open, open_sliced);
      int n = 0;
      for (std::list<std::pair<carve::csg::FaceClass, carve::mesh::MeshSet<3> *>>::iterator i = open_sliced.begin();
           i != open_sliced.end(); ++i)
      {
        switch ((*i).first)
        {
          case carve::csg::FACE_IN: open_in = (*i).second; break;
          case carve::csg::FACE_OUT: open_out = (*i).second; break;
          case carve::csg::FACE_ON_ORIENT_OUT: open_on_out = (*i).second; break;
          case carve::csg::FACE_ON_ORIENT_IN: open_on_in = (*i).second; break;
        }
        ++n;
      }
      if (n > 1)
        printf("Too much meshes! skipping %d of them\n", n - 1);
      return true;
    }
    catch (const carve::exception &ex)
    {
      printf("Error in performing operation '%s'\n", ex.str().c_str());
      return false;
    }
  }
  /*void *slice(void *a_, void *b_)
  {
    carve_poly_t *a = (carve_poly_t *)a_;
    carve_poly_t *b = (carve_poly_t *)b_;
    std::list<std::pair<carve::csg::FaceClass, carve::mesh::MeshSet<3> *> > b_sliced;
    csg.sliceAndClassify(a, b, b_sliced);
  }*/

  void convert(void *in, Mesh &mesh) { convert(*(carve_poly_t *)in, mesh); }

  void removeUnusedFaces(Mesh &mesh, void *in_, int node_id)
  {
    if (!in_)
      return;
    carve_poly_t &in = *(carve_poly_t *)in_;
    Bitarray used;
    used.resize(mesh.face.size());
    uint64_t nodeId = node_id;
    // for (int i = 0, l = in.faces.size(); i != l; ++i)
    for (carve::mesh::MeshSet<3>::face_iter i = in.faceBegin(); i != in.faceEnd(); ++i)
    {
      carve_poly_t::face_t &f = *(*i);
      uint64_t fid = f_id.getAttribute(&f, 0);
      if ((fid >> 32) != nodeId)
        continue;
      fid &= 0xFFFFFFFF;
      G_ASSERT(fid < mesh.face.size());
      used.set((int)fid);
    }

    for (int i = mesh.face.size() - 1; i >= 0; --i)
    {
      if (used[i])
        continue;
      int j;
      for (j = i - 1; j >= 0; --j)
        if (used[j])
          break;
      mesh.removeFacesFast(j + 1, i - j);
      i = j + 1;
    }
  }

protected:
  void convert(carve_poly_t &in, Mesh &mesh)
  {
    // carve::poly::Polyhedron *poly = carve::polyhedronFromMesh(&inSet, -1);
    // carve::poly::Polyhedron &in = *poly;
    int faces = 0;
    for (carve_poly_t::face_iter fi = in.faceBegin(); fi != in.faceEnd(); ++fi)
      ++faces;


    mesh.face.resize(faces);
    for (int ti = 0; ti < tiUsed; ++ti)
    {
      mesh.tface[ti].resize(faces);
      mem_set_0(mesh.tface[ti]);
    }
    mesh.vert.resize(in.vertex_storage.size());
    for (int i = 0; i < mesh.vert.size(); ++i)
      mesh.vert[i] = Point3(in.vertex_storage[i].v[0], in.vertex_storage[i].v[1], in.vertex_storage[i].v[2]);

    std::vector<carve_poly_t::vertex_t *> faceVerts;

    int faceId = 0;
    for (carve_poly_t::face_iter fi = in.faceBegin(); fi != in.faceEnd(); ++fi)
    {
      const carve_poly_t::face_t &f = **fi;

      G_ASSERT(f.nVertices() == 3);
      faceVerts.resize(0);
      f.getVertices(faceVerts);
      for (int j = 0; j < f.nVertices(); ++j)
      {
        int vi = faceVerts[j] - &in.vertex_storage[0];

        for (int ti = 0; ti < tiUsed; ++ti)
        {
          if (fv_tex[ti].hasAttribute(&f, j))
          {
            Point2 tc = fv_tex[ti].getAttribute(&f, j);
            mesh.tface[ti][faceId].t[j] = mesh.tvert[ti].size();
            mesh.tvert[ti].push_back(tc);
          }
        }
        mesh.face[faceId].v[j] = vi;
      }
      mesh.face[faceId].mat = f_tex_num.getAttribute(&f, 0);
      mesh.face[faceId].smgr = f_smgr_num.getAttribute(&f, 0);
      ++faceId;
    }
    mesh.optimize_tverts();
    // del_it(poly);
  }

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

    for (int i = 0; i < mesh.face.size(); ++i)
      f_tex_num.setAttribute(faces[i], mesh.face[i].mat);

    for (int i = 0; i < mesh.face.size(); ++i)
      f_smgr_num.setAttribute(faces[i], mesh.face[i].smgr);

    for (int ti = 0; ti < tiUsed; ++ti)
      for (int i = 0; i < mesh.tface[ti].size(); ++i)
      {
        fv_tex[ti].setAttribute(faces[i], 0, mesh.tvert[ti][mesh.tface[ti][i].t[0]]);
        fv_tex[ti].setAttribute(faces[i], 1, mesh.tvert[ti][mesh.tface[ti][i].t[1]]);
        fv_tex[ti].setAttribute(faces[i], 2, mesh.tvert[ti][mesh.tface[ti][i].t[2]]);
      }

    carve_poly_t *poly = new carve_poly_t(faces);

    return poly;
  }
};

BaseCSG *make_new_csg(int tccnt) { return new PerformCSG(tccnt); }

void delete_csg(BaseCSG *csg) { delete ((PerformCSG *)csg); }
