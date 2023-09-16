#include <sceneRay/dag_sceneRay.h>
#include <sceneRay/dag_cachedRtVecFaces.h>
#include <math/dag_math3d.h>
#include <math/dag_bounds2.h>
#include <util/dag_bitArray.h>
#include <math/dag_capsuleTriangle.h>
#include <math/dag_capsuleTriangleCached.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint4.h>
#include <supp/dag_prefetch.h>

#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <stdlib.h>

#include <EASTL/bitvector.h>
#include <memory/dag_framemem.h>

static inline unsigned get_u32(const SceneRayI24F8 &fi) { return fi.u32; }
static inline unsigned get_u32(const uint16_t &fi) { return StaticSceneRayTracer::CULL_CCW << 24; }

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define IF_CONSTEXPR if constexpr
#else
#define IF_CONSTEXPR if
#endif

template <typename FI>
StaticSceneRayTracerT<FI>::StaticSceneRayTracerT() :
  lastU(0), lastV(0), skipFlags(USER_INVISIBLE << 24), useFlags(CULL_BOTH << 24), cullFlags(0)
{
  G_ASSERTF((((intptr_t)(this)) & 15) == 0, "this=%p", this);
  G_ASSERTF((((intptr_t)(&dump)) & 15) == 0, "this=%p dump=%p", this, &dump);
  G_ASSERTF((sizeof(dump) & 15) == 0, "sizeof(dump)=%d", sizeof(dump));
  G_ASSERTF((((intptr_t)(&v_rtBBox)) & 15) == 0, "this=%p v_rtBBox=%p", this, &v_rtBBox);
  v_bbox3_init_empty(v_rtBBox);
}

template <typename FI>
StaticSceneRayTracerT<FI>::~StaticSceneRayTracerT()
{}

template <typename FI>
void StaticSceneRayTracerT<FI>::Node::destroy()
{
  if (sub0)
  {
    sub0->destroy();
    if (Node *n = (((BNode *)this)->sub1))
      n->destroy();
    delete ((BNode *)this);
  }
  else
  {
    delete ((LNode *)this);
  }
}

//--------------------------------------------------------------------------
template <typename FI>
inline int StaticSceneRayTracerT<FI>::tracerayLNode(const Point3 &p, const Point3 &dir, real &mint, const LNode *lnode,
  int fromFace) const
{
  //--------------------------------------------------------------------------

  int hit = -1;
  const FaceIndex *__restrict fIndices = lnode->faceIndexStart;
  const FaceIndex *__restrict fIndicesEnd = lnode->faceIndexEnd;
  for (; fIndices < fIndicesEnd; fIndices++)
  {
    int fIndex = *fIndices; //(int)lnode->fc[fi];
    unsigned faceFlag = get_u32(*fIndices);
    IF_CONSTEXPR (sizeof(FaceIndex) > 2)
      if ((faceFlag & skipFlags) || !(faceFlag & useFlags))
        continue;
    if (fIndex == fromFace)
      continue;

    // check culling
    faceFlag |= cullFlags;

    const RTface &f = faces(fIndex);

    const Point3_vec4 &vert0 = verts(f.v[0]);
    const Point3 edge1 = verts(f.v[1]) - vert0;
    const Point3 edge2 = verts(f.v[2]) - vert0;
    real u, v;
    if ((faceFlag & VAL_CULL_BOTH) == VAL_CULL_BOTH)
    {
      if (!traceRayToTriangleNoCull(p, dir, mint, vert0, edge1, edge2, u, v))
        continue;
    }
    else if (!traceRayToTriangleCullCCW(p, dir, mint, vert0, edge1, edge2, u, v))
      continue;
    lastU = u;
    lastV = v;
    hit = fIndex;
  }
  return hit;
}

//--------------------------------------------------------------------------
// Noinline because tracerayNodeVec called recursively

template <typename FI>
template <bool noCull>
DAGOR_NOINLINE int StaticSceneRayTracerT<FI>::tracerayLNodeVec(const vec3f &p, const vec3f &dir, float &t, const LNode *lnode,
  int fromFace) const
{
  const FaceIndex *__restrict fIndices = lnode->faceIndexStart;
  const FaceIndex *__restrict fIndicesEnd = lnode->faceIndexEnd;
  int fIndex[4];
  int hit = -1;

  const uint32_t batchSize = 4;
  uint32_t count = 0;

  for (; fIndices < fIndicesEnd; fIndices++)
  {
    IF_CONSTEXPR (sizeof(FaceIndex) > 2)
      if (get_u32(*fIndices) & skipFlags || !(get_u32(*fIndices) & useFlags))
        continue;
    if (int(*fIndices) == fromFace)
      continue;

    fIndex[count++] = int(*fIndices);
    if (count < batchSize)
      continue;
    count = 0;

    vec4f vert[batchSize][3];
    for (uint32_t i = 0; i < batchSize * 3; i++)
    {
      const RTface &f = faces(fIndex[i / 3]);
      vert[i / 3][i % 3] = v_ld(&verts(f.v[i % 3]).x);
    }

    int ret = traceray4Triangles(p, dir, t, vert, batchSize, noCull);
    if (ret >= 0)
      hit = fIndex[ret];
  }

  if (count)
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t i = 0; i < batchSize; i++)
    {
      if (i >= count) // unroll hint
        break;
      const RTface &f = faces(fIndex[i]);
      vert[i][0] = v_ld(&verts(f.v[0]).x);
      vert[i][1] = v_ld(&verts(f.v[1]).x);
      vert[i][2] = v_ld(&verts(f.v[2]).x);
    }

    int ret = traceray4Triangles(p, dir, t, vert, count, noCull);
    if (ret >= 0)
      hit = fIndex[ret];
  }

  return hit;
}

//--------------------------------------------------------------------------
template <typename FI>
template <bool noCull>
inline int VECTORCALL StaticSceneRayTracerT<FI>::rayhitLNodeIdx(vec3f p, vec3f dir, float t, const LNode *lnode) const
{
  const FaceIndex *__restrict fIndices = lnode->faceIndexStart;
  const FaceIndex *__restrict fIndicesEnd = lnode->faceIndexEnd;
  IF_CONSTEXPR (sizeof(FaceIndex) > 2)
    for (; fIndices < fIndicesEnd; fIndices++)
    {
      unsigned faceFlag = get_u32(*fIndices);
      if ((faceFlag & skipFlags) || !(faceFlag & useFlags))
        continue;
      break;
    }
  if (fIndices >= fIndicesEnd)
    return -1;

#if !_TARGET_SIMD_SSE
  vec4f ret = v_zero();
#endif
  int fIndex0 = *fIndices;
  int fIndex[4] = {fIndex0, fIndex0, fIndex0, fIndex0};
  int fIndexCnt = 0;
  for (; fIndices < fIndicesEnd;)
  {
    fIndexCnt = 0;
    for (; fIndexCnt < 4 && fIndices < fIndicesEnd; fIndices++)
    {
      IF_CONSTEXPR (sizeof(FaceIndex) > 2)
      {
        unsigned faceFlag = get_u32(*fIndices);
        if ((faceFlag & skipFlags) || !(faceFlag & useFlags))
          continue;
      }
      fIndex[fIndexCnt] = *fIndices;
      fIndexCnt++;
    }

    const RTface *f[4] = {&faces(fIndex[0]), &faces(fIndex[1]), &faces(fIndex[2]), &faces(fIndex[3])};

    // vec3f vert0 = v_ld(&verts(f.v[0]).x);
    mat43f p0, p1, p2;
    v_mat44_transpose_to_mat43(v_ld(&verts(f[0]->v[0]).x), v_ld(&verts(f[1]->v[0]).x), v_ld(&verts(f[2]->v[0]).x),
      v_ld(&verts(f[3]->v[0]).x), p0.row0, p0.row1, p0.row2);
    v_mat44_transpose_to_mat43(v_ld(&verts(f[0]->v[1]).x), v_ld(&verts(f[1]->v[1]).x), v_ld(&verts(f[2]->v[1]).x),
      v_ld(&verts(f[3]->v[1]).x), p1.row0, p1.row1, p1.row2);
    v_mat44_transpose_to_mat43(v_ld(&verts(f[0]->v[2]).x), v_ld(&verts(f[1]->v[2]).x), v_ld(&verts(f[2]->v[2]).x),
      v_ld(&verts(f[3]->v[2]).x), p2.row0, p2.row1, p2.row2);

    if (int hit = rayhit4Triangles(p, dir, t, p0, p1, p2, noCull))
      return fIndex[__bsf_unsafe(hit)];
  }
  return -1;
}


//--------------------------------------------------------------------------
template <typename FI>
int StaticSceneRayTracerT<FI>::tracerayNode(const Point3 &p, const Point3 &dir, real &mint, const Node *node, int fromFace) const
{
  if (!rayIntersectSphere(p, dir, node->bsc, node->bsr2, mint))
    return -1;
  if (node->sub0)
  {
    // branch node
    int hit = tracerayNode(p, dir, mint, node->sub0, fromFace);
    int hit2;
    if ((hit2 = tracerayNode(p, dir, mint, ((BNode *)node)->sub1, fromFace)) != -1)
      return hit2;
    return hit;
  }
  else
  {
    // leaf node
    return tracerayLNode(p, dir, mint, (LNode *)node, fromFace);
  }
}

template <typename FI>
template <bool noCull>
DAGOR_NOINLINE int StaticSceneRayTracerT<FI>::tracerayNodeVec(const vec3f &p, const vec3f &dir, float &t, const Node *node,
  int fromFace) const
{
  if (!v_test_ray_sphere_intersection(p, dir, v_splats(t), v_ldu(&node->bsc.x), v_set_x(node->bsr2)))
    return -1;
  if (node->sub0)
  {
    // branch node
    int hit0 = tracerayNodeVec<noCull>(p, dir, t, node->sub0, fromFace);
    int hit1 = tracerayNodeVec<noCull>(p, dir, t, ((BNode *)node)->sub1, fromFace);
    return hit1 != -1 ? hit1 : hit0;
  }
  else
  {
    // leaf node
    return tracerayLNodeVec<noCull>(p, dir, t, (LNode *)node, fromFace);
  }
}
//--------------------------------------------------------------------------

template <typename FI>
int StaticSceneRayTracerT<FI>::getHeightBelowLNode(const Point3 &p, real &ht, const LNode *lnode) const
{
  int ret = -1;
  const FaceIndex *__restrict fIndices = lnode->faceIndexStart;
  const FaceIndex *__restrict fIndicesEnd = lnode->faceIndexEnd;
  for (; fIndices < fIndicesEnd; fIndices++)
  {
    int fIndex = *fIndices; //(int)lnode->fc[fi];
    const RTface &f = faces(fIndex);

    const Point3 &vert0 = verts(f.v[0]);
    const Point3 edge1 = verts(f.v[1]) - vert0;
    const Point3 edge2 = verts(f.v[2]) - vert0;
    real triangleHt;
    if (!getTriangleHtCull<false>(p.x, p.z, triangleHt, vert0, edge1, edge2))
      continue;
    if (p.y >= triangleHt && ht <= triangleHt)
    {
      ret = fIndex;
      ht = triangleHt;
    }
  }
  return ret;
}

__forceinline static bool heightRayIntersectSphere(const Point3 &p, const Point3 &sphere_center, real r2, real cht)
{

  // reoder calculations for in-order cpus
  Point3 pc = sphere_center - p;
  real c = pc * pc - r2;
  if (c >= 0)
  {
    if (pc.y > 0)
      return false;

    real d = pc.y * pc.y - c; //*4.0f;
    if (d < 0)
      return false;

    real s = sqrtf(d);

    if (sphere_center.y - s > p.y)
      return false;
    if (sphere_center.y + s < cht)
      return false;
  }
  return true;
}

template <typename FI>
int StaticSceneRayTracerT<FI>::getHeightBelowNode(const Point3 &p, real &ht, const Node *node) const
{
  if (!heightRayIntersectSphere(p, node->bsc, node->bsr2, ht))
    return -1;
  if (node->sub0)
  {
    // branch node
    int hit = getHeightBelowNode(p, ht, node->sub0);
    int hit2 = getHeightBelowNode(p, ht, ((BNode *)node)->sub1);
    if (hit2 != -1)
      return hit2;
    return hit;
  }
  else
  {
    // leaf node
    return getHeightBelowLNode(p, ht, (LNode *)node);
  }
}

//--------------------------------------------------------------------------
template <typename FI>
template <bool noCull>
int VECTORCALL StaticSceneRayTracerT<FI>::rayhitNodeIdx(vec3f p, vec3f dir, float t, const Node *node) const
{
  if (!v_test_ray_sphere_intersection(p, dir, v_splats(t), v_ldu(&node->bsc.x), v_set_x(node->bsr2)))
    return -1;
  if (node->sub0)
  {
    // branch node
    int idx = rayhitNodeIdx<noCull>(p, dir, t, node->sub0);
    if (idx >= 0)
      return idx;
    return rayhitNodeIdx<noCull>(p, dir, t, ((BNode *)node)->sub1);
  }
  else
  {
    // leaf node
    return rayhitLNodeIdx<noCull>(p, dir, t, (LNode *)node);
  }
}

template <typename FI>
int StaticSceneRayTracerT<FI>::traceray(const Point3 &p, const Point3 &wdir2, real &mint2, int fromFace) const
{
  Point3 wdir = wdir2;
  real dirl = lengthSq(wdir);
  if (dirl == 0)
  {
    return -1;
  }
  dirl = sqrtf(dirl);
  wdir /= dirl;
  real mint = mint2 * dirl;
  int id = tracerayNormalized(p, wdir, mint, fromFace);
  if (id >= 0)
  {
    mint2 = mint / dirl;
  }
  return id;
}

template <typename FI>
int VECTORCALL StaticSceneRayTracerT<FI>::traceray(vec3f p, vec3f dir, real &mint, int fromFace) const
{
  vec3f lenSq = v_length3_sq(dir);
  if (v_extract_x(lenSq) == 0.f)
    return -1;
  vec3f len = v_sqrt4(lenSq);
  dir = v_div(dir, len);
  real t = mint * v_extract_x(len);
  int id = tracerayNormalized(p, dir, t, fromFace);
  if (id >= 0)
    mint = t / v_extract_x(len);
  return id;
}

template <typename FI>
int StaticSceneRayTracerT<FI>::tracerayNormalizedScalar(const Point3 &p1, const Point3 &wdir, real &mint2, int fromFace) const
{
  int ret = -1;

  if (mint2 <= 0)
    return -1;

  real shouldStartAt = 0;
  Point3 endPt = p1 + wdir * mint2;
  if (!(getBox() & p1))
  {
    if (!rayIntersectSphere(p1, wdir, getSphere().c, getSphere().r2, mint2))
      return -1;
    if (!does_line_intersect_box(getBox(), p1, endPt, shouldStartAt))
      return -1;
    else
      shouldStartAt *= mint2 * 0.9999f;
  }

  Ray ray(p1 + wdir * shouldStartAt, wdir, mint2 - shouldStartAt, shouldStartAt, getLeafSize());
  Point3 endCell(floor(Point3(endPt.x / getLeafSize().x, endPt.y / getLeafSize().y, endPt.z / getLeafSize().z)));
  IPoint3 diff = IPoint3(endCell) - ray.currentCell();
  int n = 4 * (abs(diff.x) + abs(diff.y) + abs(diff.z)) + 1;
  double t = 0;
  float rmint = ray.mint;
  for (; n; n--)
  {
    int leafSize = 0;
    if (getLeafLimits() & ray.currentCell())
    {
      auto getLeafRes = dump.grid->get_leaf(ray.currentCell().x, ray.currentCell().y, ray.currentCell().z);
      if (getLeafRes)
      {
        int ret2;
        if (*getLeafRes.leaf && (ret2 = tracerayNode(ray.p, ray.wdir, rmint, *getLeafRes.leaf, fromFace)) != -1)
        {
          ray.mint = rmint;
          ret = ret2;
          if (ray.mint < t)
            goto end;
        }
      }
      else
      {
        leafSize = getLeafRes.getl() - 1;
      }
    }
    IPoint3 oldpt = ray.currentCell();
    for (;;)
    {
      if (ray.mint < (t = ray.nextCell()))
        goto end;
      if (leafSize == 0 || (ray.currentCell().x >> leafSize) != (oldpt.x >> leafSize) ||
          (ray.currentCell().y >> leafSize) != (oldpt.y >> leafSize) || (ray.currentCell().z >> leafSize) != (oldpt.z >> leafSize))
        break;
    }
  }
end:;
  if (ret >= 0)
  {
    mint2 = ray.mint + ray.startAt;
  }
  return ret;
}

template <typename FI>
int StaticSceneRayTracerT<FI>::tracerayNormalized(const Point3 &p, const Point3 &dir, real &t, int fromFace) const
{
  return tracerayNormalized(v_ldu(&p.x), v_ldu(&dir.x), t, fromFace);
}

template <typename FI>
int VECTORCALL StaticSceneRayTracerT<FI>::tracerayNormalized(vec3f p, vec3f dir, real &mint, int fromFace) const
{
  int ret = -1;
  if (mint <= 0)
    return -1;

  real shouldStartAt = 0;
  vec3f endPt = v_madd(dir, v_splats(mint), p);
  bbox3f bbox = v_ldu_bbox3(getBox());
  if (!v_bbox3_test_pt_inside(bbox, p))
  {
    vec4f startT = v_set_x(mint);
    if (!v_ray_box_intersection(p, dir, startT, bbox))
      return -1;
    shouldStartAt = v_extract_x(startT) * 0.9999f;
  }

  Point3_vec4 rayStart, rayDir;
  IPoint4 endCell;
  v_stu(&rayStart.x, v_madd(dir, v_splats(shouldStartAt), p));
  v_stu(&rayDir.x, dir);
  v_stui(&endCell.x, v_cvt_floori(v_div(endPt, v_ldu(&getLeafSize().x))));
  Ray ray(rayStart, rayDir, mint - shouldStartAt, shouldStartAt, getLeafSize());
  IPoint3 diff = IPoint3::xyz(endCell) - ray.currentCell();
  int n = 4 * (abs(diff.x) + abs(diff.y) + abs(diff.z)) + 1;
  double t = 0;
  vec3f v_rayp = v_ldu(&ray.p.x);
  vec3f v_rayd = v_ldu(&ray.wdir.x);
  for (; n; n--)
  {
    int leafSize = 0;
    if (getLeafLimits() & ray.currentCell())
    {
      auto getLeafRes = dump.grid->get_leaf(ray.currentCell().x, ray.currentCell().y, ray.currentCell().z);
      if (getLeafRes)
      {
        int ret2;
        if (*getLeafRes.leaf)
        {
          // if ((cullFlags&VAL_CULL_BOTH) == VAL_CULL_BOTH)//that how it is supposed to be.
          ret2 = tracerayNodeVec<true>(v_rayp, v_rayd, ray.mint, *getLeafRes.leaf, fromFace);
          // else
          //   ret2 = tracerayNodeVec<false> (v_rayp, v_rayd, rmint, *getLeafRes.leaf, fromFace);

          if (ret2 != -1)
          {
            ret = ret2;
            if (ray.mint < t)
              goto end;
          }
        }
      }
      else
        leafSize = getLeafRes.getl() - 1;
    }
    IPoint3 oldpt = ray.currentCell();
    for (;;)
    {
      if (ray.mint < (t = ray.nextCell()))
        goto end;
      if (leafSize == 0 || (ray.currentCell().x >> leafSize) != (oldpt.x >> leafSize) ||
          (ray.currentCell().y >> leafSize) != (oldpt.y >> leafSize) || (ray.currentCell().z >> leafSize) != (oldpt.z >> leafSize))
        break;
    }
  }
end:;
  if (ret >= 0)
    mint = ray.mint + ray.startAt;
  return ret;
}

template <typename FI>
int VECTORCALL StaticSceneRayTracerT<FI>::rayhitNormalizedIdx(vec3f p, vec3f dir, real mint) const
{
  if (mint <= 0)
    return -1;

  real shouldStartAt = 0;
  vec3f endPt = v_madd(dir, v_splats(mint), p);
  bbox3f bbox = v_ldu_bbox3(getBox());
  if (!v_bbox3_test_pt_inside(bbox, p))
  {
    vec4f startT = v_set_x(mint);
    if (!v_ray_box_intersection(p, dir, startT, bbox))
      return -1;
    shouldStartAt = v_extract_x(startT) * 0.9999f;
  }

  Point3_vec4 rayStart, rayDir;
  IPoint4 endCell;
  v_stu(&rayStart.x, v_madd(dir, v_splats(shouldStartAt), p));
  v_stu(&rayDir.x, dir);
  v_stui(&endCell.x, v_cvt_floori(v_div(endPt, v_ldu(&getLeafSize().x))));
  Ray ray(rayStart, rayDir, mint - shouldStartAt, shouldStartAt, getLeafSize());

  vec3f v_rayp = v_ldu(&ray.p.x);
  vec3f v_rayd = v_ldu(&ray.wdir.x);

  IPoint3 diff = IPoint3::xyz(endCell) - ray.currentCell();
  int n = 4 * (abs(diff.x) + abs(diff.y) + abs(diff.z)) + 1;
  double t = 0;
  for (; n; n--)
  {
    int leafSize = 0;
    IPoint3 oldpt = ray.currentCell();
    if (getLeafLimits() & ray.currentCell())
    {
      auto getLeafRes = dump.grid->get_leaf(oldpt.x, oldpt.y, oldpt.z);
      if (getLeafRes)
      {
        if (*getLeafRes.leaf)
        {
          int hitIdx = rayhitNodeIdx<true>(v_rayp, v_rayd, ray.mint, *getLeafRes.leaf);
          if (hitIdx >= 0)
            return hitIdx;
        }
      }
      else
        leafSize = getLeafRes.getl() - 1;
    }
    for (;;)
    {
      if (ray.mint < (t = ray.nextCell()))
        goto end;
      if (leafSize == 0 || (ray.currentCell().x >> leafSize) != (oldpt.x >> leafSize) ||
          (ray.currentCell().y >> leafSize) != (oldpt.y >> leafSize) || (ray.currentCell().z >> leafSize) != (oldpt.z >> leafSize))
        break;
    }
  }
end:
  return -1;
}

template <typename FI>
int StaticSceneRayTracerT<FI>::getHeightBelow(const Point3 &pos1, float &ht) const
{
  if (!(BBox2(Point2::xz(getBox()[0]), Point2::xz(getBox()[1])) & Point2::xz(pos1)))
    return -1;
  if (getBox()[0].y > pos1.y)
    return -1;
  if (getBox()[1].y < ht)
    return -1;
  Point3 p1 = pos1;
  p1.y = p1.y < getBox()[1].y ? p1.y : getBox()[1].y;
  float endY = getBox()[0].y < ht ? ht : getBox()[0].y;
  IPoint3 cell = ipoint3(floor(div(p1, getLeafSize())));
  cell.y = min(cell.y, getLeafLimits()[1].y);                                    // avoid nans leading to infinite cycle
  int endcellY = max((int)floorf(endY / getLeafSize().y), getLeafLimits()[0].y); // avoid nans leading to infinite cycle
  float cht = endY;
  int ret = -1, ret2;
  float maxCellHt = (cell.y + 1) * getLeafSize().y;
  for (; cell.y >= endcellY;)
  {
    auto getLeafRes = dump.grid->get_leaf(cell.x, cell.y, cell.z);
    if (getLeafRes)
    {
      if (*getLeafRes.leaf && (ret2 = getHeightBelowNode(p1, cht, *getLeafRes.leaf)) >= 0)
        ret = ret2;
      maxCellHt -= getLeafSize().y;
      cell.y--;
    }
    else
    {
      int leafSize = getLeafRes.getl() - 1;
      cell.y &= ~((1 << leafSize) - 1);
      maxCellHt = cell.y * getLeafSize().y;
      cell.y--;
    }
    if (cht > maxCellHt)
      goto end;
  }
end:;
  if (ret >= 0)
  {
    ht = cht;
    return ret;
  }
  return -1;
}

template <typename FI>
int StaticSceneRayTracerT<FI>::traceDown(const Point3 &pos1, float &t) const
{
  float ht = pos1.y - t;
  int ret = getHeightBelow(pos1, ht);
  if (ret < 0)
    return -1;
  t = pos1.y - ht;
  return ret;
}


//--------------------------------------------------------------------------

template <typename FI>
inline bool StaticSceneRayTracerT<FI>::clipCapsuleFace(FaceIndex findex, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedir) const
{
  unsigned faceFlag = get_u32(findex);
  int fid = findex;
  IF_CONSTEXPR (sizeof(FaceIndex) > 2)
    if ((faceFlag & skipFlags) || !(faceFlag & useFlags))
      return false;

  const RTfaceBoundFlags &fb = facebounds(fid);
  if (!(faceFlag & VAL_CULL_CW) && fb.n * movedir > 0.001f)
    return false;
  else if (!(faceFlag & VAL_CULL_CCW) && fb.n * movedir < -0.001f)
    return false;

  Point3 cp1_f, cp2_f;
  real md_f = 0;
  const RTface &f = faces(fid);

  TriangleFace tf(verts(f.v[0]), verts(f.v[1]), verts(f.v[2]), fb.n);

  if (!clipCapsuleTriangle(c, cp1_f, cp2_f, md_f, tf))
    return false;

  /*if ( faceFlag & CULL_BOTH ) {
    tf.reverse();
    // check for collision with other side of face
    clipCapsuleTriangle (c, cp1_r, cp2_r, md_r, tf);

    // we take the closest value to 0 (i.e., min penetration depth)
    if ( md_r < 0 && md_r > md_f ) {
      // back side
      if ( md_r < md ) {
        md = md_r;
        cp1 = cp1_r;
        cp2 = cp2_r;
        return true;
      } else
        return false;
    }
  }*/

  if (md_f < md)
  {
    // front side
    md = md_f;
    cp1 = cp1_f;
    cp2 = cp2_f;
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------
template <typename FI>
inline int StaticSceneRayTracerT<FI>::clipCapsuleLNode(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized, const LNode *lnode) const
{
  //--------------------------------------------------------------------------
  int hit = -1;
  const FaceIndex *fIndices = lnode->faceIndexStart;
  const FaceIndex *fIndicesEnd = lnode->faceIndexEnd;
  for (; fIndices < fIndicesEnd; fIndices++)
  {
    int fIndex = *fIndices; //(int)lnode->fc[fi];
    if (clipCapsuleFace(*fIndices, c, cp1, cp2, md, movedirNormalized))
      hit = fIndex;
  }
  return hit;
}

//--------------------------------------------------------------------------
template <typename FI>
inline int StaticSceneRayTracerT<FI>::clipCapsuleNode(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized, const Node *node) const
{
  //--------------------------------------------------------------------------
  // if (!capsuleIntersectsSphere(c, node->bsc, node->bsr2))
  //  return -1;
  if (node->sub0)
  {
    // branch node
    int hit = clipCapsuleNode(c, cp1, cp2, md, movedirNormalized, node->sub0);
    int hit2;
    if ((hit2 = clipCapsuleNode(c, cp1, cp2, md, movedirNormalized, ((BNode *)node)->sub1)) != -1)
      return hit2;
    return hit;
  }
  else
  {
    return clipCapsuleLNode(c, cp1, cp2, md, movedirNormalized, (const LNode *)node);
  }
  return -1;
}

//! Tests for capsule clipping by scene; returns depth of penetration and normal to corresponding face
template <typename FI>
int StaticSceneRayTracerT<FI>::clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized) const
{
  bbox3f cBox = c.getBoundingBox();
  if (!v_bbox3_test_box_intersect(cBox, v_ldu_bbox3(getBox())))
  {
    return -1;
  }
  // debug ( "clipCapsule start: md=%.3f", md );
  BBox3 sBox;
  v_stu_bbox3(sBox, cBox);

  int id = -1;
  IPoint3 b0(floor(div(sBox[0], getLeafSize())));
  IPoint3 b1(floor(div(sBox[1], getLeafSize())));
  getLeafLimits().clip(b0.x, b0.y, b0.z, b1.x, b1.y, b1.z);
  if (b1.x >= b0.x && b1.y >= b0.y && b1.z >= b0.z)
    for (int z = b0.z; z <= b1.z; ++z)
      for (int y = b0.y; y <= b1.y; ++y)
        for (int x = b0.x; x <= b1.x; ++x)
        {
          auto getLeafRes = dump.grid->get_leaf(x, y, z);
          if (!getLeafRes || !*getLeafRes.leaf)
            continue;

          int new_id = clipCapsuleNode(c, cp1, cp2, md, movedirNormalized, *getLeafRes.leaf);
          if (new_id != -1)
            id = new_id;
        }
  // debug ( "->id=%d, md=%.3f", id, md );
  return id;
}


template <typename FI>
template <typename U>
__forceinline void VECTORCALL StaticSceneRayTracerT<FI>::getFacesLNode(vec3f bmin, vec3f bmax, Tab<int> &face, U &uniqueness,
  const LNode *node, const StaticSceneRayTracerT *rt)
{
  const FaceIndex *fIndices = node->faceIndexStart;
  const FaceIndex *fIndicesEnd = node->faceIndexEnd;
  bbox3f box;
  box.bmin = bmin;
  box.bmax = bmax;
  for (; fIndices < fIndicesEnd; ++fIndices)
  {
    const int fidx = *fIndices;

    IF_CONSTEXPR (sizeof(FaceIndex) > 2)
    {
      unsigned faceFlag = get_u32(*fIndices);
      if ((faceFlag & rt->getSkipFlags()) || !(faceFlag & rt->getUseFlags()) || !(faceFlag & VAL_CULL_BOTH))
        continue;
    }

    if (uniqueness[fidx])
      continue;

    const RTface &f = rt->faces(fidx);
    bbox3f facebox;
    facebox.bmin = facebox.bmax = v_ld(&rt->verts(f.v[0]).x);
    v_bbox3_add_pt(facebox, v_ld(&rt->verts(f.v[1]).x));
    v_bbox3_add_pt(facebox, v_ld(&rt->verts(f.v[2]).x));

    if (!v_bbox3_test_box_intersect(box, facebox))
      continue;

    face.push_back(fidx);
    uniqueness.set(fidx, true);
  }
}

template <typename FI>
template <typename U>
void VECTORCALL StaticSceneRayTracerT<FI>::getFacesNode(vec3f bmin, vec3f bmax, Tab<int> &face, U &uniqueness, const Node *node,
  const StaticSceneRayTracerT *rt)
{
  if (node->sub0)
  {
    getFacesNode(bmin, bmax, face, uniqueness, node->sub0, rt);
    getFacesNode(bmin, bmax, face, uniqueness, ((BNode *)node)->sub1, rt);
  }
  else
    getFacesLNode(bmin, bmax, face, uniqueness, (LNode *)node, rt);
}

template <typename FI>
int StaticSceneRayTracerT<FI>::getFaces(Tab<int> &face, const BBox3 &box) const
{
  const int startFc = face.size();

  IPoint3 b0(floor(::div(box.lim[0], dump.leafSize)));
  IPoint3 b1(floor(::div(box.lim[1], dump.leafSize)));

  getLeafLimits().clip(b0.x, b0.y, b0.z, b1.x, b1.y, b1.z);

  if (b1.x >= b0.x && b1.y >= b0.y && b1.z >= b0.z)
  {
    eastl::bitvector<framemem_allocator> uniqueness(getFacesCount());
    bbox3f bbox = v_ldu_bbox3(box);

    for (int z = b0.z; z <= b1.z; ++z)
      for (int y = b0.y; y <= b1.y; ++y)
        for (int x = b0.x; x <= b1.x; ++x)
        {
          auto getLeafRes = dump.grid->get_leaf(x, y, z);
          if (!getLeafRes || !*getLeafRes.leaf)
            continue;

          getFacesNode(bbox.bmin, bbox.bmax, face, uniqueness, *getLeafRes.leaf, this);
        }
  }

  return face.size() - startFc;
}

static StaticSceneRayTracerT<SceneRayI24F8>::GetFacesContext mtCtx32;
static StaticSceneRayTracerT<uint16_t>::GetFacesContext mtCtx16;

template <>
StaticSceneRayTracerT<SceneRayI24F8>::GetFacesContext *StaticSceneRayTracerT<SceneRayI24F8>::getMainThreadCtx() const
{
  return &mtCtx32;
}

template <>
StaticSceneRayTracerT<uint16_t>::GetFacesContext *StaticSceneRayTracerT<uint16_t>::getMainThreadCtx() const
{
  return &mtCtx16;
}

template <typename FI>
void StaticSceneRayTracerT<FI>::Node::patch(void *base, void *dump_base)
{
  if (sub0)
  {
    sub0.patch(base);
    sub0->patch(base, dump_base);
    ((BNode *)this)->patch(base, dump_base);
  }
  else
    ((LNode *)this)->patch(base, dump_base);
}

template <typename FI>
void StaticSceneRayTracerT<FI>::BNode::patch(void *base, void *dump_base)
{
  if (sub1)
  {
    sub1.patch(base);
    sub1->patch(base, dump_base);
  }
}

template <typename FI>
void StaticSceneRayTracerT<FI>::LNode::patch(void * /*base*/, void *dump_base)
{
  faceIndexStart.patch(dump_base);
  faceIndexEnd.patch(dump_base);
}

#include "serializableSceneRay.cpp"

template class StaticSceneRayTracerT<SceneRayI24F8>;
template class StaticSceneRayTracerT<uint16_t>;
