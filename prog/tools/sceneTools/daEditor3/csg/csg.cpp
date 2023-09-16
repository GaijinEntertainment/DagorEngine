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
  carve::interpolate::FaceVertexAttr<Point20> fv_tex;
  carve::interpolate::FaceAttr<uint64_t> f_id;
  carve::interpolate::FaceAttr<int> f_tex_num;
  carve::interpolate::FaceAttr<unsigned> f_smgr_num;

  carve::csg::CSG csg;
  carve::csg::CSG_TreeNode *left;

public:
  PerformCSG()
  {
    fv_tex.installHooks(csg);
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
  void *op(void *a_, void *b_)
  {
    carve_poly_t *a = (carve_poly_t *)a_;
    carve_poly_t *b = (carve_poly_t *)b_;
    carve_poly_t *result = csg.compute(a, b, carve::csg::CSG::UNION, NULL,
      carve::csg::CSG::CLASSIFY_EDGE); // carve::csg::CSG::CLASSIFY_NORMAL);//
    return result;
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
          mesh.tvert[0].push_back(tc);
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
    /*
    for (int i = 0; i < mesh.face.size(); ++i)
      f_tex_num.setAttribute(&faces[i], mesh.face[i].mat);

    for (int i = 0; i < mesh.face.size(); ++i)
      f_smgr_num.setAttribute(&faces[i], mesh.face[i].smgr);

    for (int i = 0; i < mesh.tface[0].size(); ++i)
    {
      fv_tex.setAttribute(&faces[i], 0, mesh.tvert[0][mesh.tface[0][i].t[0]]);
      fv_tex.setAttribute(&faces[i], 1, mesh.tvert[0][mesh.tface[0][i].t[1]]);
      fv_tex.setAttribute(&faces[i], 2, mesh.tvert[0][mesh.tface[0][i].t[2]]);
    }*/

    carve_poly_t *poly = new carve_poly_t(faces);

    return poly;
  }
};

BaseCSG *make_new_csg() { return new PerformCSG(); }

void delete_csg(BaseCSG *csg) { delete ((PerformCSG *)csg); }
