#include <sceneRay/dag_sceneRay.h>
#include <math/dag_math3d.h>
#include <math/dag_math3d.h>
#include <math/integer/dag_IPoint4.h>
#include <util/dag_bitArray.h>
#include <util/dag_stlqsort.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_boundingSphere.h>
#include <math/dag_mathUtils.h>
#include <math/dag_traceRayTriangle.h>
#include <debug/dag_log.h>

#define MAX_SCENE_LEAF_FACES 48

static inline void init_index_and_flags(SceneRayI24F8 &out, unsigned i, unsigned f) { out.index = i, out.flags = f; }
static inline void init_index_and_flags(uint16_t &out, unsigned i, unsigned) { out = i; }

template <typename FI>
BuildableStaticSceneRayTracerT<FI>::BuildableStaticSceneRayTracerT(const Point3 &lsz, int lev) :
  StaticSceneRayTracerT<FI>(),
  vertsTab(midmem),
  facesTab(midmem),
  faceboundsTab(midmem),
  faceIndicesTab(midmem),
  createdGrid(lev),
  faceFlagsTab(midmem)
{
  dump.leafSize = lsz;

  dump.grid = &createdGrid;
}

template <typename FI>
BuildableStaticSceneRayTracerT<FI>::~BuildableStaticSceneRayTracerT()
{
  clear_and_shrink(nodesMemory);
  replaceAllPointers();
}
///============BUILD=================================

template <typename FI>
__forceinline bool BuildableStaticSceneRayTracerT<FI>::__add_face(const RTface &f, int ri)
{
  vec3f v0 = v_ld(&verts(f.v[0]).x);
  vec3f v1 = v_ld(&verts(f.v[1]).x);
  vec3f v2 = v_ld(&verts(f.v[2]).x);
  vec3f sc = v_triangle_bounding_sphere_center(v0, v1, v2);

  // ensure that sc is valid via NaN check
  bool valid = (v_signmask(v_cmp_eq(v_remove_nan(sc), sc)) & 0x7) == 0x7;
  v_stu_p3(&faceboundsTab[ri].sc.x, valid ? sc : v0);
  return valid;
}

template <typename FI>
bool BuildableStaticSceneRayTracerT<FI>::reserve(int fcount, int vcount)
{
  vertsTab.reserve(vcount);
  facesTab.reserve(fcount);
  faceboundsTab.reserve(fcount + 1); // to prevent 128-bit reading past the buffer
  dump.vertsPtr = vertsTab.data();
  dump.vertsCount = vertsTab.size();
  dump.facesPtr = facesTab.data();
  dump.facesCount = facesTab.size();
  return true;
}

template <>
bool BuildableStaticSceneRayTracerT<SceneRayI24F8>::addmesh(const Point3 *vert, int vcount, const unsigned *face, unsigned stride,
  int fcount, const unsigned *flags, bool rebuild_now)
{
  return addmesh((const uint8_t *)vert, sizeof(Point3), vcount, face, stride, fcount, flags, rebuild_now);
}
template <>
bool BuildableStaticSceneRayTracerT<uint16_t>::addmesh(const Point3 *vert, int vcount, const unsigned *face, unsigned stride,
  int fcount, const unsigned *flags, bool rebuild_now)
{
  G_ASSERTF(0, "addMesh(Point3 *vert, unsigned *face) n/a for FaceIndex=uint16_t");
  return false;
}

template <typename FI>
bool BuildableStaticSceneRayTracerT<FI>::addmesh(const uint8_t *vert, unsigned v_stride, int vcount, const index_t *face,
  unsigned stride, int fcount, const unsigned *flags, bool rebuild_now)
{
  int ff = facesTab.size(), ri = ff, fc = fcount;
  int vn = vertsTab.size();
  G_ASSERTF_RETURN(sizeof(index_t) > 2 || vn + vcount < 0x10000, false, "sizeof(index_t)=%d vertsTab.size()=%d vcount=%d",
    sizeof(index_t), vn, vcount);

  append_items(vertsTab, vcount);
  for (int i = 0; i < vcount; ++i, vert += v_stride)
  {
    vertsTab[i + vn] = *(Point3 *)vert;
    vertsTab[i + vn].resv = 0;
  }
  append_items(facesTab, fc);

  dump.vertsPtr = vertsTab.data();
  dump.vertsCount = vertsTab.size();
  dump.facesPtr = facesTab.data();
  dump.facesCount = facesTab.size();
  dump.faceIndicesCount = 0;

  faceIndicesTab.clear();

  G_ASSERTF(!flags || sizeof(index_t) > 2, "sizeof(index_t)=%d flags=%p", sizeof(index_t), flags);
  if (flags)
    append_items(faceFlagsTab, fc, flags);
  else if (sizeof(index_t) > 2)
  {
    int at = append_items(faceFlagsTab, fc);
    for (int i = at; i < faceFlagsTab.size(); ++i)
      faceFlagsTab[i] = CULL_CCW;
  }

  for (int i = 0; i < fc; ++i, ++ri)
  {
    // i - iterates through source m.faces
    // ri - positioned to current added faces int this.faces
    RTface &f = faces(ri);
    f.v[0] = face[0] + vn;
    f.v[1] = face[1] + vn;
    f.v[2] = face[2] + vn;

    face = (index_t *)(((char *)face) + stride);
  }

  if (rebuild_now)
    rebuild();

  return true;
  //  debug ( "assured about faces %Xh..%Xh for %p", ff, faces.size()-1, this );
}

template <typename FI>
void BuildableStaticSceneRayTracerT<FI>::oneMeshInplace(const Point3_vec4 *vert, int vcount, bool use_external_verts,
  const index_t *face, int fn, bool use_external_faces, const unsigned *flags)
{

  if (use_external_verts)
  {
    dump.vertsPtr = vert;
    dump.vertsCount = vcount;
  }
  else
  {
    vertsTab = make_span_const(vert, vcount);
    dump.vertsPtr = vertsTab.data();
    dump.vertsCount = vertsTab.size();
  }

  if (use_external_faces)
  {
    dump.facesPtr = (RTface *)face;
    dump.facesCount = fn;
  }
  else
  {
    facesTab = make_span_const((RTface *)face, fn);
    dump.facesPtr = facesTab.data();
    dump.facesCount = facesTab.size();
  }

  dump.faceIndicesCount = 0;

  faceIndicesTab.clear();

  G_ASSERTF(!flags || sizeof(index_t) > 2, "sizeof(index_t)=%d flags=%p", sizeof(index_t), flags);
  if (flags)
    append_items(faceFlagsTab, fn, flags);
  else if (sizeof(index_t) > 2)
  {
    int at = append_items(faceFlagsTab, fn);
    for (int i = at; i < faceFlagsTab.size(); ++i)
      faceFlagsTab[i] = CULL_CCW;
  }
  rebuild();
}

class SceneRayBuildContext
{
public:
  Bitarray vertsUsed;
  Tab<Point3> usedVerts;
  bool fastBuild = false;
};

template <typename FI>
void BuildableStaticSceneRayTracerT<FI>::buildLNode(LNode *node, FaceIndex *f, int numf, SceneRayBuildContext &)
{
  node->faceIndexStart = f;
  node->faceIndexEnd = node->faceIndexStart + numf;
}

struct DECLSPEC_ALIGN(16) BBox3_vec4 : public BBox3
{
  float _resv;
} ATTRIBUTE_ALIGN(16);

template <typename FI>
struct AxisSeparator
{
  const Point3 *axis;
  AxisSeparator(const Point3 *a) : axis(a) {}
  bool operator()(const FI a, const FI b) const { return axis[(int)a].x < axis[(int)b].x; }
};


extern vec4f mesh_fast_bounding_sphere(const Point3 *point, unsigned point_count, bbox3f_cref box);
static constexpr int BNBIAS = 1;

template <typename FI>
uintptr_t BuildableStaticSceneRayTracerT<FI>::build_node(FaceIndex *fc, int numf, SceneRayBuildContext &ctx)
{
  G_STATIC_ASSERT(BNBIAS < sizeof(Node));
  int i;
  if (numf <= 0)
  {
    G_ASSERT(0);
    return 0;
  }
  BBox3_vec4 box;

  ctx.vertsUsed.resize(getVertsCount());
  ctx.vertsUsed.reset();

  // we need to reserve enough memory so when we read our last vertex it'll still fit in the memory if
  // we read 16 bytes (vec4), instead of 12 bytes (Point3), as this is what happens in mesh_bounding_sphere
  // when we v_ldu these vertices and usedVertsCnt == getVertsCount()
  uint32_t numVertsInArray = min(numf * 3, getVertsCount());
  ctx.usedVerts.resize(numVertsInArray + 1);

  int usedVertsCnt = 0;

  vec3f fb_median = v_zero();
  bbox3f vBox;
  v_bbox3_init_empty(vBox);
  for (i = 0; i < numf; ++i)
  {
    int fid = (int)fc[i];
    const RTface &f = faces(fid);
#define USE_VERT(fvi)                                      \
  if (!ctx.vertsUsed.get(f.v[fvi]))                        \
  {                                                        \
    ctx.vertsUsed.set(f.v[fvi]);                           \
    ctx.usedVerts[usedVertsCnt++] = this->verts(f.v[fvi]); \
  }
    USE_VERT(0)
    USE_VERT(1)
    USE_VERT(2)
#undef USE_VERT
    // fb_median = v_add(fb_median, v_add(fboxes[fid].bmin, fboxes[fid].bmax));
    fb_median = v_add(fb_median, v_ldu(&faceboundsTab[fid].sc.x));
    v_bbox3_add_box(vBox, fboxes[fid]);
  }
  fb_median = v_div(fb_median, v_splats(numf));
  // fb_median = v_div(fb_median, v_splats(2.0f*numf));
  G_ASSERT((((intptr_t)&box[0].x) & 15) == 0);
  v_st(&box[0].x, vBox.bmin);
  v_stu(&box[1].x, vBox.bmax);
  alignas(16) Point4 nodeSphR2;
  if (ctx.fastBuild)
  {
    v_st(&nodeSphR2.x, mesh_fast_bounding_sphere(&ctx.usedVerts[0], usedVertsCnt, vBox));
  }
  else
  {
    BSphere3 sp = ::mesh_bounding_sphere(&ctx.usedVerts[0], usedVertsCnt);
    nodeSphR2 = Point4::xyzV(sp.c, sp.r2);
  }

  Point3 wd = box.width();
  uintptr_t allocatedAt = nodesMemory.size();
  if (numf <= MAX_SCENE_LEAF_FACES)
  {
    // build leaf node
    append_items(nodesMemory, sizeof(LNode));
    LNode *n = new (nodesMemory.data() + allocatedAt, _NEW_INPLACE) LNode;
    n->bsc = Point3::xyz(nodeSphR2);
    n->bsr2 = nodeSphR2.w;
    G_ASSERT(!n->sub0); // FIXME: this assertion is actually a check for VC2010 compiler bug with /arch:SSE2 (fixed in VC2010sp1)
    buildLNode(n, fc, numf, ctx);
  }
  else
  {
    // build branch
    append_items(nodesMemory, sizeof(BNode));
    int df = numf >> 1;
    df = (df + 3) & ~3; // try to keep number faces in nodes as multiple of 4
    int md = 0;
    float ms = wd[0];
    for (i = 1; i < 3; ++i)
      if (wd[i] > ms)
        ms = wd[md = i];

    AxisSeparator<FI> axis((Point3 *)&faceboundsTab[0].sc[md]);
    stlsort::nth_element(fc, fc + df, fc + numf, axis);

    const uintptr_t left = build_node(fc, df, ctx);
    const uintptr_t right = build_node(fc + df, numf - df, ctx);
    BNode *n = new (nodesMemory.data() + allocatedAt, _NEW_INPLACE) BNode;
    n->bsc = Point3::xyz(nodeSphR2);
    n->bsr2 = nodeSphR2.w;
    n->sub0 = (Node *)left;
    n->sub1 = (Node *)right;
  }
  return allocatedAt + BNBIAS;
}

template <typename FI>
struct FaceIndexContainer
{
  Tab<FI> faces;
  int face, count;
  FaceIndexContainer() : faces(tmpmem) {}
};

template <typename FI>
void BuildableStaticSceneRayTracerT<FI>::buildLeaf(Leaf **leaf, SceneRayBuildContext &ctx)
{
  FaceIndexContainer<FI> *fc1 = (FaceIndexContainer<FI> *)*leaf;
  *leaf = (Node *)build_node(&faceIndices(0) + fc1->face, fc1->count, ctx);
  delete fc1;
}

extern int tri_box_overlap(const Point3 &boxcenter, const Point3 &boxhalfsize, const Point3 *triverts);

template <typename FI>
bool BuildableStaticSceneRayTracerT<FI>::rebuild(bool fast)
{
  faceboundsTab.resize(getFacesCount() + 1); // to prevent 128-bit reading past the buffer
  faceboundsTab.resize(getFacesCount());
  clear_and_shrink(nodesMemory);
  replaceAllPointers();
  nodesMemory.reserve(1); // can't be less than one node
  createdGrid.clear();
  for (int i = 0; i < getFacesCount(); ++i)
  {
    const RTface &f = faces(i);
    if (!__add_face(f, i) && sizeof(index_t) > 2)
      faceFlagsTab[i] = 0;
  }

  Tab<IPoint3> fcell(tmpmem);
  fboxes.resize(getFacesCount());
  G_ASSERTF((((intptr_t)(&dump)) & 15) == 0, "this=%p dump=%p", this, &dump);
  G_ASSERTF((((intptr_t)(&v_rtBBox)) & 15) == 0, "this=%p v_rtBBox=%p", this, &v_rtBBox);
  G_ASSERT((((intptr_t)fboxes.data()) & 15) == 0);
  G_ASSERT((((intptr_t)(&verts(0))) & 15) == 0);
  v_bbox3_init_empty(v_rtBBox);
  for (int i = 0; i < getFacesCount(); ++i)
  {
    const RTface &f = faces(i);
    v_bbox3_init(fboxes[i], v_ld(&verts(f.v[0]).x));
    v_bbox3_add_pt(fboxes[i], v_ld(&verts(f.v[1]).x));
    v_bbox3_add_pt(fboxes[i], v_ld(&verts(f.v[2]).x));
    if (sizeof(index_t) <= 2 || faceFlagsTab[i] & CULL_BOTH) //
      v_bbox3_add_box(v_rtBBox, fboxes[i]);
  }

  v_stu(&dump.rtBox[0].x, v_rtBBox.bmin);
  v_stu_p3(&dump.rtBox[1].x, v_rtBBox.bmax);
  {
    vec4f theRad = v_zero(), theCenter = v_bbox3_center(v_rtBBox);
    for (int i = 0; i < getVertsCount(); ++i)
      theRad = v_max(theRad, v_length3_sq_x(v_sub(theCenter, v_ld(&verts(i).x))));
    float maxRad2 = v_extract_x(theRad);
    dump.rtSphere.c = dump.rtBox.center();
    dump.rtSphere.r2 = maxRad2;
    dump.rtSphere.r = sqrtf(maxRad2);
  }

  // Tab<int> rmap(tmpmem);
  clear_leaf_bounds();
  int totalInds = 0;
  Tab<FaceIndexContainer<FI> *> containers(tmpmem);
  Point3 halfLeafSize = getLeafSize() * 0.5f;
  Point3 halfLeafSizeScaled = getLeafSize() * 0.50001f;
  vec4f vleafSize = v_ldu(&getLeafSize().x);
  alignas(16) IPoint4 l0;
  alignas(16) IPoint4 l1;
  G_ASSERT((((intptr_t)&l0) & 15) == 0);
  G_ASSERT((((intptr_t)&l1) & 15) == 0);
  for (int i = 0; i < getFacesCount(); ++i)
  {
    if (sizeof(index_t) > 2 && !(faceFlagsTab[i] & CULL_BOTH))
      continue;
    // RTfaceBoundFlags &fb = facebounds(i);
    vec4i vl0 = v_cvt_floori(v_div(fboxes[i].bmin, vleafSize));
    vec4i vl1 = v_cvt_floori(v_div(fboxes[i].bmax, vleafSize));
    v_sti(&l0, vl0);
    v_sti(&l1, vl1);
    const RTface &f = faces(i);
    Point3 triverts[3] = {
      verts(f.v[0]), // faces[fc[i]].vp[0];
      verts(f.v[1]), // faces[fc[i]].vp[0]+faces[fc[i]].edge[0];
      verts(f.v[2])  // faces[fc[i]].vp[0]+faces[fc[i]].edge[1];
    };
    for (int z = l0.z; z <= l1.z; ++z)
      for (int y = l0.y; y <= l1.y; ++y)
        for (int x = l0.x; x <= l1.x; ++x)
        {
          Point3 leafcenter = Point3(x * getLeafSize().x, y * getLeafSize().y, z * getLeafSize().z) + halfLeafSize;
          // could check whether the faces really crosses this cell
          if (!tri_box_overlap(leafcenter, halfLeafSizeScaled, triverts))
            continue;

          auto l = createdGrid.get_leaf(x, y, z);
          if (!l || !*l.leaf)
          {
            l.leaf = createdGrid.create_leaf(x, y, z);
            delete *l.leaf;
            *l.leaf = (Leaf *)new (tmpmem) FaceIndexContainer<FI>();
            containers.push_back(((FaceIndexContainer<FI> *)*l.leaf));
          }
          FaceIndex index;
          init_index_and_flags(index, i, sizeof(index_t) > 2 ? faceFlagsTab[i] : 0);
          ((FaceIndexContainer<FI> *)*l.leaf)->faces.push_back(index);
          totalInds++;
          notify_leaf_added(x, y, z);
        }
  }

  faceIndicesTab.reserve(totalInds);
  for (int ci = 0; ci < containers.size(); ++ci)
  {
    containers[ci]->face = append_items(faceIndicesTab, containers[ci]->faces.size(), containers[ci]->faces.data());
    containers[ci]->count = containers[ci]->faces.size();
  }

  dump.faceIndicesPtr = faceIndicesTab.data();
  dump.faceIndicesCount = faceIndicesTab.size();
  // nomem(fgrp2.resize(faces.size()));
  struct MyCB : public RTHierGrid3LeafCB<Leaf>
  {
    BuildableStaticSceneRayTracerT<FI> *rt;
    SceneRayBuildContext ctx;
    MyCB(BuildableStaticSceneRayTracerT<FI> *r, bool fast) : rt(r) { ctx.fastBuild = fast; }
    void leaf_callback(Leaf **l, int, int, int) { rt->buildLeaf(l, ctx); }
  } cb(this, fast);
  createdGrid.enum_all_leaves(cb);

  nodesMemory.shrink_to_fit();
  replaceAllPointers();
  // printf("missing %d total=%d\n", ::missing, ::total);

  clear_and_shrink(faceboundsTab);
  clear_and_shrink(faceFlagsTab);
  clear_and_shrink(fboxes);
  return true;
}

BuildableStaticSceneRayTracer *create_buildable_staticmeshscene_raytracer(const Point3 &lsz, int lev)
{
  return new BuildableStaticSceneRayTracer(lsz, lev);
}

template <typename FI>
void BuildableStaticSceneRayTracerT<FI>::replacePointers(Node *n_)
{
  if (!n_ || !n_->sub0) // leaf
    return;
  BNode *n = (BNode *)n_;
  n->sub0 = uintptr_t(n->sub0.get()) < nodesMemory.size() ? (Node *)(nodesMemory.data() + uintptr_t(n->sub0.get()) - BNBIAS) : nullptr;
  n->sub1 = uintptr_t(n->sub1.get()) < nodesMemory.size() ? (Node *)(nodesMemory.data() + uintptr_t(n->sub1.get()) - BNBIAS) : nullptr;
  replacePointers(n->sub0);
  replacePointers(n->sub1);
}

template <typename FI>
void BuildableStaticSceneRayTracerT<FI>::replaceAllPointers()
{
  struct ReplacePointersCB : public RTHierGrid3LeafCB<Leaf>
  {
    BuildableStaticSceneRayTracerT<FI> *rt;
    Tab<uint8_t> &nodesMemory;
    ReplacePointersCB(BuildableStaticSceneRayTracerT<FI> *r, Tab<uint8_t> &n) : rt(r), nodesMemory(n) {}
    void leaf_callback(Leaf **l, int, int, int)
    {
      if (!l)
        return;
      const uintptr_t pointAt = uintptr_t(*l) - BNBIAS;
      *l = (pointAt < nodesMemory.size()) ? (Node *)(nodesMemory.data() + pointAt) : nullptr;
      rt->replacePointers(*l);
    }
  } cb(this, nodesMemory);
  createdGrid.enum_all_leaves(cb);
}

template class BuildableStaticSceneRayTracerT<SceneRayI24F8>;
template class BuildableStaticSceneRayTracerT<uint16_t>;
