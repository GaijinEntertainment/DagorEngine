// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <set>
#include <map>
#include <generic/dag_smallTab.h>
#include <util/dag_bitArray.h>
#include <math/dag_mesh.h>
#include <math/dag_Point3.h>
#include "mxQMetric3.h"
#include <cstdio>
#include <cstdlib>
#include <meshOptimize/quadrics.h>
#include <math/dag_mathBase.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
// #define _DEBUG
#define MAX_NORMAL_CHANGE_COSINE 0.1
#define BOUNDARY_WEIGHT          10000.0
#define CHECK_SANITY             0
#define USE_SET                  1 // optimzation only

namespace meshopt
{
// typedef std::map<Pair, ErrorAlpha> Errors;
static bool is_face_seam(Mesh &m, int f, int e0_, int e1_, int f1, int e0, int e1)
{
  if (m.face[f1].mat != m.face[f].mat)
  {
    return true;
  }

  // tc seam
  if (m.getTFace(0).size())
  {
    if (m.getTFace(0)[f1].t[e0] != m.getTFace(0)[f].t[e0_] || m.getTFace(0)[f1].t[e1] != m.getTFace(0)[f].t[e1_])
      return true;
  }

  // normals seam
  /*
  if (m.getNormFace().size())
  {
    if (m.getNormFace()[f1][e0] != m.getNormFace()[f][e0_] ||
        m.getNormFace()[f1][e1] != m.getNormFace()[f][e1_])
      return true;
  }
   else
    if ( !(m.getFaceSMGR(f1) & m.getFaceSMGR(f)))
      return true;
  */
  return false;
}

void create_face_flags(Mesh &m, uint8_t *face_flags)
{
  VertToFaceVertMap map;
  m.buildVertToFaceVertMap(map);
  memset(face_flags, 0, m.getFace().size());
  for (int v = 0; v < m.getVert().size(); ++v)
    for (int j = 0, cnt = map.getNumFaces(v); j < cnt; ++j)
    {
      int fid = map.getVertFaceIndex(v, j);
      int fi = fid / 3, vi;

      for (vi = 0; vi < 3; ++vi)
        if (m.getFace()[fi].v[vi] == v)
          break;
      int vi2 = (vi + 1) % 3;
      int v2 = m.getFace()[fi].v[vi2];
      // find other faces for edge on v1, v2.
      int skip = fi * 3 + vi2;
      // debug("%d: %d(%d, %d) %d(%d)", fi, vi, v, m.getFace()[fi].v[vi], vi2, v2);
      // G_ASSERT(m.getFace()[fi].v[vi] == v);
      // G_ASSERT(v2 != v);
      for (int j2 = 0, cnt2 = map.getNumFaces(v2); j2 < cnt2; ++j2)
      {
        int fid2 = map.getVertFaceIndex(v2, j2);
        // if (fid2 == skip)
        //   continue;
        int fj = fid2 / 3;
        if (fj == fi)
          continue;
        const Face &f = m.getFace()[fj];
        int look_vi, orig_vi;

        for (look_vi = 0; look_vi < 3; ++look_vi)
          if (f.v[look_vi] == v)
            break;

        for (orig_vi = 0; orig_vi < 3; ++orig_vi)
          if (f.v[orig_vi] == v2)
            break;

        G_ASSERT(orig_vi != 3);
        G_ASSERT(orig_vi != look_vi);
        G_ASSERT(f.v[orig_vi] == v2);
        if (look_vi < 3 && orig_vi < 3)
        {
          // common edge for two faces
          if (is_face_seam(m, fi, vi, vi2, fj, look_vi, orig_vi))
          {
            // mark as edge for both faces;
            face_flags[fi] |= max(vi, vi2) << min(vi, vi2);
            face_flags[fj] |= max(look_vi, orig_vi) << min(look_vi, orig_vi);
          }
        }
      }
    }
};

typedef std::pair<int, int> Pair;
enum
{
  NO_COLLAPSE = 0,
  COLLAPSE_TO = 1,
  COLLAPSE_FROM = 2,
  COLLAPSE_NEW = 3
};

struct ErrorAlpha
{
  double error;
  float alpha;

  unsigned minVertEdgeCost : 30;
  unsigned collapseTo : 2; // if 2 - collapse to new Vertex, if 1 - collapse to this, if 2 - collapse to edge, 0 -invalid
};

struct Vertex
{
  std::set<unsigned> vNeighboors;
  std::set<unsigned> fNeighboors;
  ErrorAlpha e;
};

class Quadrics
{
public:
  Quadrics(Tab<Vsplit> &vsplits);
  ~Quadrics();
  int optimize(const uint8_t *faceIndices, int stride, bool short_indices, int fn, const uint8_t *face_flags, const Point3 *verts,
    int vn, int target_num_faces, bool onlyExistingVerts);
  struct Face
  {
    uint32_t v[3];
  };

private:
  int optimize(int target_num_faces);
  SmallTab<Point3, TmpmemAlloc> mvert;
  SmallTab<Face, TmpmemAlloc> mface;
  SmallTab<Point3, TmpmemAlloc> origFaceNorm;
  // fields
  Tab<MxQuadric3> quadrics; /* quadric for each vertex */
  Tab<Vsplit> &vsplits;
  Tab<Vertex> verts;
  Bitarray removedFaces;
  Bitarray removedVerts;

  Bitarray boundaryVerts;
  bool doNotProduceNewVertices;
  // methods
  void select_pair(const uint8_t *face_flags);
  ErrorAlpha calculate_error(int id_v1, int id_v2);
  // inline methods
  inline double vertex_error(const MxQuadric3 &q, const Point3 &v);
  MxQuadric3 calcFaceQuadrics(int i);
  void updateVert(int i);
  bool checkSanity();
  bool checkQuadrics();
  inline float get_flip_error(int from, int v2, const Point3 &to);
  friend struct VertexPtr;
};

Quadrics::Quadrics(Tab<Vsplit> &vsp) : vsplits(vsp), quadrics(tmpmem), verts(tmpmem) { doNotProduceNewVertices = true; }

Quadrics::~Quadrics() {}

template <class T>
static inline void decode(Quadrics::Face *f, int fn, const uint8_t *ind, int stride)
{
  for (int i = 0; i < fn; ++i, ind += stride)
  {
    f[i].v[0] = *(((T *)ind));
    f[i].v[1] = *(((T *)ind) + 1);
    f[i].v[2] = *(((T *)ind) + 2);
  }
}

int Quadrics::optimize(const uint8_t *faceIndices, int stride, bool short_indices, int fn, const uint8_t *face_flags,
  const Point3 *verts, int vn, int tf, bool onlyExistingVerts)
{
  doNotProduceNewVertices = onlyExistingVerts;
  clear_and_resize(mface, fn);
  clear_and_resize(mvert, vn);
  mvert = make_span_const(verts, vn);
  if (short_indices)
    decode<uint16_t>(mface.data(), fn, faceIndices, stride);
  else
    decode<uint32_t>(mface.data(), fn, faceIndices, stride);

  boundaryVerts.resize(mvert.size());
  // boundaryVerts.reset();

  removedFaces.resize(mface.size());
  // removedFaces.reset();
  removedVerts.resize(mvert.size());
  // removedVerts.reset();

  // calculate initial error for each valid pair
  select_pair(face_flags);
  return optimize(tf);
}

struct EdgeSeam
{
  uint8_t cnt : 1;
  uint8_t seam : 1;
  EdgeSeam(unsigned has_seam) : cnt(0), seam(has_seam ? 1 : 0) {}
  void increase(unsigned has_seam)
  {
    if (!cnt)
      cnt = 1;
    if (has_seam)
      seam = 1;
  }
  bool hasSeam()
  {
    if (seam)
      return true;
    if (!cnt)
      return true;
    return false;
  }
};

void Quadrics::updateVert(int i)
{
  if (removedVerts[i])
    return;
  ErrorAlpha minErr;
  minErr.error = FLT_MAX;
  minErr.collapseTo = NO_COLLAPSE;
  for (std::set<unsigned>::iterator pos = verts[i].vNeighboors.begin(); pos != verts[i].vNeighboors.end(); ++pos)
  {
    ErrorAlpha err = calculate_error(i, *pos);
    if (err.error < minErr.error)
      minErr = err;
  }
  verts[i].e = minErr;
}

MxQuadric3 Quadrics::calcFaceQuadrics(int i)
{
  Point3 n = (mvert[mface[i].v[1]] - mvert[mface[i].v[0]]) % (mvert[mface[i].v[2]] - mvert[mface[i].v[0]]);
  float area = n.length();
  if (area > 1.e-7)
    n *= 1.0 / area;
  origFaceNorm[i] = n;
  // area used to increase error
  area *= 0.5;
  MxQuadric3 Q(n.x, n.y, n.z, -(n * mvert[mface[i].v[0]]), area);
  Q *= area;
  return Q;
}

void Quadrics::select_pair(const uint8_t *face_flags)
{
  int max_vid;
  int min_vid;
  int i, j;
  quadrics.resize(mvert.size());
  mem_set_0(quadrics);
  verts.clear();
  verts.reserve(mvert.size() * 2);
  verts.resize(mvert.size());
  for (i = 0; i < verts.size(); i++)
    verts[i].e.collapseTo = NO_COLLAPSE;
  // compute initial quadric
  clear_and_resize(origFaceNorm, mface.size());
  for (int i = 0; i < mface.size(); i++)
  {
    if (removedFaces[i])
      continue;
    Point3 n = (mvert[mface[i].v[1]] - mvert[mface[i].v[0]]) % (mvert[mface[i].v[2]] - mvert[mface[i].v[0]]);
    MxQuadric3 Q = calcFaceQuadrics(i);
    for (int j = 0; j < 3; j++)
      quadrics[mface[i].v[j]] += Q;
  }
  for (int i = 0; i < mface.size(); i++)
  {
    if (removedFaces[i])
      continue;
    for (int j = 0; j < 3; j++)
    {
      int vi = mface[i].v[j], vi2 = mface[i].v[(j + 1) % 3];
      verts[vi].fNeighboors.insert((i << 2) + j);
      verts[vi].vNeighboors.insert(vi2);
      verts[vi2].vNeighboors.insert(vi);
    }
  }
  typedef std::map<Pair, EdgeSeam> BoundaryEdge;
  BoundaryEdge boundEdges;
  /* (v1, v2) is an edge */
  /* id_v1 < id_v2*/
  unsigned seam = 0;
  for (i = 0; i < mface.size(); i++)
  {
    if (removedFaces[i])
      continue;
    min_vid = min(mface[i].v[0], mface[i].v[1]);
    max_vid = max(mface[i].v[0], mface[i].v[1]);
    if (face_flags)
      seam = face_flags[i] & SEAM_EDGE_01;
    BoundaryEdge::iterator cerror;
    cerror = boundEdges.find(Pair(min_vid, max_vid));
    if (cerror == boundEdges.end())
      boundEdges.insert(BoundaryEdge::value_type(Pair(min_vid, max_vid), seam));
    else
      cerror->second.increase(seam);

    min_vid = min(mface[i].v[0], mface[i].v[2]);
    max_vid = max(mface[i].v[0], mface[i].v[2]);
    if (face_flags)
      seam = face_flags[i] & SEAM_EDGE_02;
    cerror = boundEdges.find(Pair(min_vid, max_vid));
    if (cerror == boundEdges.end())
      boundEdges.insert(BoundaryEdge::value_type(Pair(min_vid, max_vid), seam));
    else
      cerror->second.increase(seam);

    min_vid = min(mface[i].v[1], mface[i].v[2]);
    max_vid = max(mface[i].v[1], mface[i].v[2]);
    if (face_flags)
      seam = face_flags[i] & SEAM_EDGE_12;
    cerror = boundEdges.find(Pair(min_vid, max_vid));
    if (cerror == boundEdges.end())
      boundEdges.insert(BoundaryEdge::value_type(Pair(min_vid, max_vid), seam));
    else
      cerror->second.increase(seam);
  }

  for (BoundaryEdge::iterator iter = boundEdges.begin(); iter != boundEdges.end(); iter++)
  {
    if (!iter->second.hasSeam())
      continue;

    boundaryVerts.set(iter->first.first);
    boundaryVerts.set(iter->first.second);
  }
  for (i = 0; i < mvert.size(); i++)
    updateVert(i);
}

inline float Quadrics::get_flip_error(int from, int toid, const Point3 &to)
{
  const float FLIP_ERROR = FLT_MAX;
  for (std::set<unsigned>::iterator pos = verts[from].fNeighboors.begin(); pos != verts[from].fNeighboors.end(); ++pos)
  {
    unsigned i = *pos >> 2;
    if (removedFaces[i])
      continue;
    if (mface[i].v[0] == toid || mface[i].v[1] == toid || mface[i].v[2] == toid)
      continue; // removed face
    Point3 v[3];
    v[0] = mvert[mface[i].v[0]];
    v[1] = mvert[mface[i].v[1]];
    v[2] = mvert[mface[i].v[2]];
    Point3 nOld = normalize((v[1] - v[0]) % (v[2] - v[0])); // current normal
    v[(*pos) & 3] = to;
    Point3 nNew = normalize((v[1] - v[0]) % (v[2] - v[0])); // new normal
    float dot = nNew * nOld;
    // do not allow to cross edge, and check if facenornal is still there
    if (nNew * nOld < MAX_NORMAL_CHANGE_COSINE || nNew * origFaceNorm[i] < 0.0)
      return FLIP_ERROR;
  }
  return 0.f;
}

ErrorAlpha Quadrics::calculate_error(int id_v1, int id_v2)
{
  ErrorAlpha ret;
  bool bv1 = boundaryVerts[id_v1], bv2 = boundaryVerts[id_v2];
  ret.collapseTo = NO_COLLAPSE;
  ret.minVertEdgeCost = id_v2;
  if (bv1 && bv2)
  {
    ret.error = FLT_MAX;
    ret.alpha = 0;
    ret.collapseTo = NO_COLLAPSE;
    return ret;
  }

  MxQuadric3 q_bar;
  q_bar = quadrics[id_v1] + quadrics[id_v2];
  if (doNotProduceNewVertices | bv1 | bv2)
  {
    float error1, error2;
    if (bv1)
    {
      ret.alpha = 0;
      ret.collapseTo = COLLAPSE_TO;
      ret.error = vertex_error(q_bar, mvert[id_v2]) + get_flip_error(id_v2, id_v1, mvert[id_v1]);
    }
    else if (bv2)
    {
      ret.alpha = 1;
      ret.collapseTo = COLLAPSE_FROM;
      ret.error = vertex_error(q_bar, mvert[id_v1]) + get_flip_error(id_v1, id_v2, mvert[id_v2]);
    }
    else
    {
      error1 = vertex_error(q_bar, mvert[id_v1]) + get_flip_error(id_v2, id_v1, mvert[id_v1]);

      error2 = vertex_error(q_bar, mvert[id_v2]) + get_flip_error(id_v1, id_v2, mvert[id_v2]);
      if (error1 < error2)
      {
        ret.alpha = 0;
        ret.collapseTo = COLLAPSE_TO;
        ret.error = error1;
      }
      else
      {
        ret.alpha = 1;
        ret.collapseTo = COLLAPSE_FROM;
        ret.error = error2;
      }
    }
  }
  else
  {

    if (q_bar.optimize(ret.alpha, mvert[id_v2], mvert[id_v1])) // note that det(q_delta) equals to M44
    {
      Point3 v = mvert[id_v2] * ret.alpha + mvert[id_v1] * (1.0f - ret.alpha);
      ret.error = vertex_error(q_bar, v) + get_flip_error(id_v1, id_v2, v) + get_flip_error(id_v2, id_v1, v);
      if (ret.alpha == 0)
        ret.collapseTo = COLLAPSE_TO;
      else if (ret.alpha == 1)
        ret.collapseTo = COLLAPSE_FROM;
      else
        ret.collapseTo = COLLAPSE_NEW;
    }
    else
    {
      //
      // if q_delta is NOT invertible, select
      // vertex from v1, v2, and (v1+v2)/2
      float error1 = vertex_error(q_bar, mvert[id_v1]) + get_flip_error(id_v2, id_v1, mvert[id_v1]);
      float error2 = vertex_error(q_bar, mvert[id_v2]) + get_flip_error(id_v1, id_v2, mvert[id_v2]);
      Point3 v = (mvert[id_v2] + mvert[id_v1]) * 0.5;
      float error3 = vertex_error(q_bar, v) + +get_flip_error(id_v2, id_v1, v) + get_flip_error(id_v1, id_v2, v);

      if (error1 <= error2 && error1 <= error3)
      {
        ret.error = error1;
        ret.alpha = 0;
        ret.collapseTo = COLLAPSE_TO;
      }
      else if (error2 <= error1 && error2 <= error3)
      {
        ret.error = error2;
        ret.alpha = 1;
        ret.collapseTo = COLLAPSE_FROM;
      }
      else
      {
        ret.error = error3;
        ret.alpha = 0.5;
        ret.collapseTo = COLLAPSE_NEW;
      }
    }
  }
  // todo: inrease error using attributes and boundaries
  G_ASSERT(ret.collapseTo);
  if (ret.error >= FLT_MAX)
    ret.collapseTo = NO_COLLAPSE;
  return ret;
}

#if USE_SET
static void print_vert(const Vertex &v, int id)
{
  printf("%d cost: %g collapseTo: %d edge n: %d\n", id, v.e.error, v.e.collapseTo, v.e.minVertEdgeCost);
}
struct VertexPtr
{
  VertexPtr(int i) : _index(i) {}
  static Quadrics *_meshptr;
  int _index; // ptr to vertex position in mesh

  bool operator<(const VertexPtr &vp) const
  {
    if ((!_meshptr->verts[_index].e.collapseTo && !_meshptr->verts[vp._index].e.collapseTo) ||
        _meshptr->verts[_index].e.error == _meshptr->verts[vp._index].e.error)
      return (_index < vp._index);

    return (_meshptr->verts[_index].e.error < _meshptr->verts[vp._index].e.error);
  }
  typedef std::set<VertexPtr, std::less<VertexPtr>> VertexPtrSet;
  static void dumpset(VertexPtrSet &ms);
};
Quadrics *VertexPtr::_meshptr = 0;

void VertexPtr::dumpset(VertexPtrSet &ms)
{
  printf("+++ Dumping set of vertices +++\n");

  VertexPtrSet::iterator iter;
  int i = 0;
  for (iter = ms.begin(); iter != ms.end(); ++iter)
  {
    const VertexPtr v = *iter;
    printf("\tvertex %d in set : ", i++);
    print_vert(v._meshptr->verts[v._index], v._index);
  }
}

#endif

int Quadrics::optimize(int target_num_faces)
{
  vsplits.clear();
  vsplits.reserve((mface.size() - target_num_faces) / 2); // each split removes 2 faces in manfold mesh
#if USE_SET
  VertexPtr::_meshptr = this;
  VertexPtr::VertexPtrSet vertSet;
  for (int i = 0; i < mvert.size(); i++) /// optimize!
  {
    if (!verts[i].e.collapseTo || removedVerts[i])
      continue;
    vertSet.insert(i);
  }
#endif

  int cface = mface.size();
  while (cface > target_num_faces)
  {
    if (!checkSanity())
      DAG_FATAL("errors");
#if USE_SET
    if (0 == vertSet.size())
      // we're done
      break;
    // VertexPtr::dumpset(vertSet);
    VertexPtr::VertexPtrSet::iterator start = vertSet.begin();
    VertexPtr vp = *start; // This is a copy of the first element
    int removeI = vp._index;
    vertSet.erase(start);
#else
    int removeI = -1;
    double min_error = FLT_MAX;
    for (int i = 0; i < mvert.size(); i++) /// optimize!
    {
      if (!verts[i].e.collapseTo || removedVerts[i])
        continue;
      if (min_error > verts[i].e.error)
      {
        removeI = i;
        min_error = verts[i].e.error;
      }
    }
    if (removeI < 0)
      break;
#endif
    // printf("%d %g %d\n", removeI, verts[removeI].e.error);
    Vsplit v;
    v.newVertex = (verts[removeI].e.collapseTo == COLLAPSE_NEW ? 1 : 0);
    v.v1 = removeI;
    v.v2 = verts[removeI].e.minVertEdgeCost;
    v.alpha = verts[removeI].e.alpha;
    int id_v3; // new vertex
    int id_rv; // removed vertex (collapse from)
    int id_cv; // collapse to vertex
    switch (verts[removeI].e.collapseTo)
    {
      case COLLAPSE_FROM:
        v.vf = id_cv = v.v2;
        id_rv = v.v1;
        break;
      case COLLAPSE_TO:
        v.vf = id_cv = v.v1;
        id_rv = v.v2;
        break;
      case COLLAPSE_NEW:
        v.vf = id_cv = v.v1;
        id_rv = v.v2;
        mvert[v.vf] = mvert[v.v2] * v.alpha + mvert[v.v1] * (1.0f - v.alpha);
        break;
      default:
#if USE_SET
        VertexPtr::dumpset(vertSet);
#endif
        G_ASSERT(0);
    }
    // printf("remove %d to %d save=%d\n", id_rv, id_cv, v.vf);
    G_ASSERT(!removedVerts[id_rv]);
    vsplits.push_back(v);

    for (std::set<unsigned>::iterator pos = verts[v.vf].fNeighboors.begin(); pos != verts[v.vf].fNeighboors.end();)
    {
      int i = *pos >> 2;
      int j = *pos & 3;
      if (mface[i].v[0] == id_rv || mface[i].v[1] == id_rv || mface[i].v[2] == id_rv)
      {
        removedFaces.set(i);
        for (int vi = 0; vi < 3; ++vi)
          if (vi != j)
          {
            int v = mface[i].v[vi];
            for (std::set<unsigned>::iterator pos = verts[v].fNeighboors.begin(); pos != verts[v].fNeighboors.end(); ++pos)
              if (*pos >> 2 == i)
              {
                verts[v].fNeighboors.erase(pos);
                break;
              }
          }
        pos = verts[v.vf].fNeighboors.erase(pos);
        cface--;
      }
      else
        ++pos;
    }
    for (std::set<unsigned>::iterator pos = verts[id_rv].fNeighboors.begin(); pos != verts[id_rv].fNeighboors.end();)
    {
      int i = *pos >> 2;
      int j = *pos & 3;
      if (mface[i].v[0] == id_cv || mface[i].v[1] == id_cv || mface[i].v[2] == id_cv)
      {
        pos = verts[id_rv].fNeighboors.erase(pos);
      }
      else
      {
        mface[i].v[j] = v.vf;
        ++pos;
      }
    }
    verts[v.vf].fNeighboors.insert(verts[id_rv].fNeighboors.begin(), verts[id_rv].fNeighboors.end());

    G_ASSERT(removedVerts.size() > id_rv);
    removedVerts.set(id_rv);
    verts[id_rv].vNeighboors.erase(id_cv);
    verts[id_rv].e.collapseTo = NO_COLLAPSE;
    verts[v.vf].vNeighboors.erase(id_rv);
    verts[v.vf].vNeighboors.insert(verts[id_rv].vNeighboors.begin(), verts[id_rv].vNeighboors.end());
    for (std::set<unsigned>::iterator pos = verts[v.vf].vNeighboors.begin(); pos != verts[v.vf].vNeighboors.end(); ++pos)
    {
      verts[*pos].vNeighboors.erase(id_rv);
      verts[*pos].vNeighboors.insert(v.vf);
    }

    quadrics[v.vf] = quadrics[v.v1] + quadrics[v.v2];
#if USE_SET
    VertexPtr::VertexPtrSet::iterator st = vertSet.find(id_rv);
    if (st != vertSet.end())
      vertSet.erase(st);
    st = vertSet.find(id_cv);
    if (st != vertSet.end())
      vertSet.erase(st);
#endif

    for (std::set<unsigned>::iterator pos = verts[v.vf].vNeighboors.begin(); pos != verts[v.vf].vNeighboors.end(); ++pos)
    {
      if (verts[*pos].e.collapseTo != NO_COLLAPSE)
      {
#if USE_SET
        VertexPtr::VertexPtrSet::iterator st = vertSet.find(*pos);
        if (st != vertSet.end())
          vertSet.erase(st);
#endif
        updateVert(*pos);
#if USE_SET
        if (verts[*pos].e.collapseTo != NO_COLLAPSE)
          vertSet.insert(*pos);
#endif
        // printf(" -> %d)", verts[*pos].minVertEdgeCost);
      }
    }
    updateVert(v.vf);
#if USE_SET
    if (verts[v.vf].e.collapseTo != NO_COLLAPSE)
      vertSet.insert(v.vf);
#endif
  }

  return cface;
}

inline double Quadrics::vertex_error(const MxQuadric3 &q, const Point3 &v)
{
  double err = q(v);
  // if (q.area())
  //   ;err/=q.area();
  return err;
}


#if CHECK_SANITY
bool Quadrics::checkSanity()
{
  bool valid = true;
  for (int i = 0; i < mface.size(); i++)
  {
    if (removedFaces[i])
      continue;
    for (int j = 0; j < 3; j++)
    {
      int vi = mface[i].v[j];
      if (verts[vi].fNeighboors.find((i << 2) + j) == verts[vi].fNeighboors.end())
      {
        printf("ERROR! vert %d is not at corner %d:%d\n", vi, i, j);
        printf("corners ");
        for (std::set<unsigned>::iterator pos = verts[vi].fNeighboors.begin(); pos != verts[vi].fNeighboors.end(); ++pos)
        {
          printf("%d:%d\n", *pos >> 2, *pos & 3);
        }
        printf("\n");
        valid = false;
      }
      int vi2 = mface[i].v[(j + 1) % 3];
      if (verts[vi].vNeighboors.find(vi2) == verts[vi].vNeighboors.end())
      {
        printf("ERROR! vert %d is not neighboor of %d\n", vi, vi2);
        valid = false;
      }
      if (verts[vi2].vNeighboors.find(vi) == verts[vi2].vNeighboors.end())
      {
        printf("ERROR! vert %d is not neighboor of %d\n", vi, vi2);
        valid = false;
      }
    }
  }
  for (int i = 0; i < mvert.size(); i++) /// optimize!
  {
    if (removedVerts[i])
      continue;
    for (std::set<unsigned>::iterator pos = verts[i].fNeighboors.begin(); pos != verts[i].fNeighboors.end(); ++pos)
    {
      for (std::set<unsigned>::iterator pos2 = verts[i].fNeighboors.begin(); pos2 != verts[i].fNeighboors.end(); ++pos2)
        if (pos != pos2 && (*pos >> 2) == (*pos2 >> 2))
        {
          printf("ERROR! face %d appears twice (%d %d) at vert %d\n", *pos >> 2, *pos & 3, *pos2 & 3, i);
          valid = false;
        }
      if (mface[*pos >> 2].v[*pos & 3] != i)
      {
        printf("ERROR! vert %d is not face corner %d:%d (%d)\n", i, *pos >> 2, *pos & 3, mface[*pos >> 2].v[*pos & 3]);
        valid = false;
      }
    }
    for (std::set<unsigned>::iterator pos = verts[i].vNeighboors.begin(); pos != verts[i].vNeighboors.end(); ++pos)
    {
      if (*pos == i)
      {
        printf("ERROR! vert %d neighboors itself\n", i);
        valid = false;
      }
      if (removedVerts[*pos])
      {
        printf("ERROR! vert %d has rececently deleted neighboor %d\n", i, *pos);
        valid = false;
      }
      if (verts[*pos].vNeighboors.find(i) == verts[*pos].vNeighboors.end())
      {
        printf("ERROR! vert %d and %d are not mutual neighboors\n", i, *pos);
        valid = false;
      }
      /*if (*pos == id_cv)
      {
        printf("ERROR! vert %d has rececently deleted %d\n",i, id_rv);
      }*/
    }
    if (verts[i].e.collapseTo != NO_COLLAPSE)
    {
      if (verts[i].e.minVertEdgeCost == i)
      {
        printf("ERROR! vert %d points to itseld\n", i);
        valid = false;
      }
      if (removedVerts[verts[i].e.minVertEdgeCost])
      {
        printf("ERROR! vert %d points to rececently deleted %d\n", i, verts[i].e.minVertEdgeCost);
        valid = false;
      }
      if (verts[i].vNeighboors.find(verts[i].e.minVertEdgeCost) == verts[i].vNeighboors.end())
      {
        printf("ERROR! vert %d points to not neighboor %d\n", i, verts[i].e.minVertEdgeCost);
        valid = false;
      }
    }
  }
  return valid;
}

bool Quadrics::checkQuadrics()
{
  Tab<MxQuadric3> refquadrics(tmpmem); /* quadric for each vertex */
  refquadrics.resize(mvert.size());
  mem_set_0(refquadrics);
  for (int i = 0; i < mface.size(); i++)
  {
    if (removedFaces[i])
      continue;
    MxQuadric3 Q = calcFaceQuadrics(i);
    for (int j = 0; j < 3; j++)
    {
      refquadrics[mface[i].v[j]] += Q;
    }
  }
  printf(" ref\n");
  bool res = true;
  for (int i = 0; i < refquadrics.size(); i++)
  {
    if (memcmp(&refquadrics[i], &quadrics[i], sizeof(quadrics[i])))
    {
      printf("i=%d:", i);
      printf("%g %g %g %g %g %g %g %g %g %g %g \n", refquadrics[i]);
      printf("%g %g %g %g %g %g %g %g %g %g %g \n", quadrics[i]);
      res = false;
    }
  }
  return res;
}
#else
bool Quadrics::checkQuadrics() { return true; }
bool Quadrics::checkSanity() { return true; }
#endif

bool make_vsplits(const uint8_t *face_indices, int stride, bool short_indices, int fn, const uint8_t *face_flags, const Point3 *verts,
  int vn, Tab<Vsplit> &vsplits, int target_num_faces, bool onlyExistingVerts)
{
  Quadrics q(vsplits);
  int fcnt = q.optimize(face_indices, stride, short_indices, fn, face_flags, verts, vn, target_num_faces, onlyExistingVerts);
  return fcnt != fn;
}

static bool get_vert2facemap(Mesh &m, Tab<Tabint> &map, int sf = 0, int numf = -1)
{
  map.clear();
  map.resize(m.vert.size());
  int i;
  if (numf < 0)
    numf = m.face.size() - sf;
  else
    numf += sf;
  for (i = sf; i < numf; ++i)
  {
    G_ASSERT(m.face[i].v[0] < m.vert.size());
    G_ASSERT(m.face[i].v[1] < m.vert.size());
    G_ASSERT(m.face[i].v[2] < m.vert.size());
    map[m.face[i].v[0]].push_back((i << 2) + 0);
    map[m.face[i].v[1]].push_back((i << 2) + 1);
    map[m.face[i].v[2]].push_back((i << 2) + 2);
  }
  return true;
}

static int update_mesh_vert(Tab<Tabint> &v2fmap, Mesh &m, int v1, int vfi)
{
  int vfF = vfi >> 2, vfC = vfi & 3;
  int vf = m.face[vfF].v[vfC];
  if (v1 == vf)
    return v2fmap[v1][0]; //
  int fci;
  for (int i = v2fmap[v1].size() - 1; i >= 0; i--)
  {
    fci = v2fmap[v1][i];
    int fi = fci >> 2, corner = fci & 3;
    v2fmap[vf].push_back(fci);

    m.face[fi].v[corner] = vf;
    if (m.facengr.size())
      m.facengr[fi][corner] = m.facengr[vfF][vfC];
#define UPDATE(arr) \
  if (arr.size())   \
    arr[fi].t[corner] = arr[vfF].t[vfC];
    UPDATE(m.cface)

    for (int c = 0; c < NUMMESHTCH; ++c)
      UPDATE(m.tface[c])

    for (int c = 0; c < m.extra.size(); ++c)
      UPDATE(m.extra[c].fc)
#undef UPDATE
  }
  v2fmap[v1].clear();
  return fci;
}

int progressive_optimize(Mesh &mesh, const Vsplit *vsplits, int vcnt, bool update_existing)
{
  Tab<Tabint> v2fmap(tmpmem);
  get_vert2facemap(mesh, v2fmap);
  if (update_existing)
  {
    for (int i = 0; i < vcnt; ++i, ++vsplits)
    {
      int vfi = v2fmap[vsplits->vf][0];
      int fci1 = update_mesh_vert(v2fmap, mesh, vsplits->v1, vfi);
      int fci2 = update_mesh_vert(v2fmap, mesh, vsplits->v2, vfi);
      if (vsplits->newVertex)
      {
        mesh.vert[vsplits->vf] = mesh.vert[vsplits->v2] * vsplits->alpha + mesh.vert[vsplits->v1] * (1.0 - vsplits->alpha);
      }
    }
  }
  else
  {
    Tabint newvertmap;
    newvertmap.reserve(mesh.vert.size() * 1.5);

    newvertmap.resize(mesh.vert.size());
    for (int i = 0; i < newvertmap.size(); ++i)
      newvertmap[i] = i;

    for (int i = 0; i < vcnt; ++i, ++vsplits)
    {
      int vf = newvertmap[vsplits->vf], v1 = newvertmap[vsplits->v1], v2 = newvertmap[vsplits->v2];
      if (vsplits->newVertex)
      {
        vf = mesh.vert.size();
        mesh.vert.push_back(mesh.vert[v2] * vsplits->alpha + mesh.vert[v1] * (1.0 - vsplits->alpha));
        int vf2 = append_items(v2fmap, 1);
        G_ASSERT(vf2 == vf);
        v2fmap[vf] = v2fmap[vsplits->vf];
        newvertmap[vsplits->vf] = vf;
      }
      int vfi = v2fmap[vsplits->vf][0];
      int fci1 = update_mesh_vert(v2fmap, mesh, vsplits->v1, vfi);
      int fci2 = update_mesh_vert(v2fmap, mesh, vsplits->v2, vfi);
    }
  }

  mesh.kill_bad_faces();
  return mesh.face.size();
}

}; // namespace meshopt
