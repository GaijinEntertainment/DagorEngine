// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/dag_triangleBoxIntersection.h>
#include <math/dag_geomTree.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_plane3.h>
#include <debug/dag_debug.h>
#include <generic/dag_sort.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_triangleTriangleIntersection.h>
#include <math/dag_mathUtils.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_capsuleTriangle.h>
#include <osApiWrappers/dag_localConv.h>
#include <supp/dag_prefetch.h>
#include <memory/dag_framemem.h>
#include <math/dag_math3d.h>
#include <sceneRay/dag_sceneRayBuildable.h>
#include <scene/dag_physMat.h>
#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_cpuFeatures.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_btagCompr.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_finally.h>

#if (_TARGET_PC && !_TARGET_STATIC_LIB)
// Enable FRT for all objects in daEditor for capsule clipping
#define MIN_FACES_TO_CREATE_GRID 0
#define MIN_WIDTH_TO_CREATE_GRID 0
#else
#define MIN_FACES_TO_CREATE_GRID 64
#define MIN_WIDTH_TO_CREATE_GRID 5
#endif
#define USE_TRACE_GRID 1
// #define VERIFY_TRACE_RESULTS 1 // May affect performance

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define IF_CONSTEXPR if constexpr
#else
#define IF_CONSTEXPR if
#endif

// CollisionNode assignment operators
#define COLLNODE_COPY_BASIC_MEMBERS() \
  ASSIGN_OP(behaviorFlags);           \
  ASSIGN_OP(flags);                   \
  ASSIGN_OP(type);                    \
  ASSIGN_OP(geomNodeId);              \
  ASSIGN_OP(physMatId);               \
  ASSIGN_OP(nextNode);                \
  ASSIGN_OP(tm);                      \
  ASSIGN_OP(modelBBox);               \
  ASSIGN_OP(boundingSphere);          \
  ASSIGN_OP(cachedMaxTmScale);        \
  ASSIGN_OP(convexPlanes);            \
  ASSIGN_OP(capsule);                 \
  ASSIGN_OP(nodeIndex);               \
  ASSIGN_OP(insideOfNode);            \
  ASSIGN_OP(name);

CollisionNode &CollisionNode::operator=(const CollisionNode &n)
{
#define ASSIGN_OP(X) X = n.X
  COLLNODE_COPY_BASIC_MEMBERS()
#undef ASSIGN_OP

  resetVertices(n.vertices);
  if (!(flags & FLAG_VERTICES_ARE_REFS) && vertices.size())
  {
    vertices.set(memalloc_typed<Point3_vec4>(vertices.size(), midmem), vertices.size());
    mem_copy_from(vertices, n.vertices.data());
  }

  resetIndices(n.indices);
  if (!(flags & FLAG_INDICES_ARE_REFS) && indices.size())
  {
    indices.set(memalloc_typed<uint16_t>(indices.size(), midmem), indices.size());
    mem_copy_from(indices, n.indices.data());
  }
  return *this;
}
CollisionNode &CollisionNode::operator=(CollisionNode &&n)
{
#define ASSIGN_OP(X) X = eastl::move(n.X)
  COLLNODE_COPY_BASIC_MEMBERS()
#undef ASSIGN_OP

  resetVertices(n.vertices);
  n.vertices.reset();

  resetIndices(n.indices);
  n.indices.reset();
  return *this;
}
#undef COLLNODE_COPY_BASIC_MEMBERS

struct ProfileStats
{
  unsigned meshNodesNum = 0;
  unsigned meshNodesSphCheckPassed = 0;
  unsigned meshNodesBoxCheckPassed = 0;
  unsigned meshTrianglesTraced = 0;
  unsigned meshTrianglesHits = 0;
};

template <typename T>
static inline void sort_collres_intersections(T &intersected_nodes_list)
{
  if (intersected_nodes_list.size() > 1)
  {
    stlsort::sort_branchless(intersected_nodes_list.begin(), intersected_nodes_list.end());
#if _TARGET_PC_WIN
    G_ASSERT(eastl::is_sorted(intersected_nodes_list.begin(), intersected_nodes_list.end(),
      [](const auto &a, const auto &b) { return a.verifyIsLess(b); }));
#elif _DEBUG_TAB_
    G_ASSERTF(!intersected_nodes_list.begin()[1].verifyIsLess(intersected_nodes_list.front()), "%.9f > %.9f",
      intersected_nodes_list.front().intersectionT, intersected_nodes_list.begin()[1].intersectionT);
#else
    G_FAST_ASSERT(!intersected_nodes_list.begin()[1].verifyIsLess(intersected_nodes_list.front()));
#endif
  }
}

CollisionResource *CollisionResource::loadResource(IGenLoad &crd, int res_id)
{
  CollisionResource *resource = new CollisionResource;
  resource->load(crd, res_id);
  return resource;
}

static int sort_by_bbox_size(const CollisionNode *left, const CollisionNode *right)
{
  float szL = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(left->modelBBox))));
  float szR = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(right->modelBBox))));
  if (fabsf(szL - szR) < 1e-3)
    return strcmp(left->name, right->name);
  return (szL < szR) ? 1 : -1;
}

template <typename T>
static inline void readTab(IGenLoad &cb, T &tab)
{
  int s = cb.readInt();
  tab.resize(0);
  if (s)
  {
    reserve_and_resize(tab, s);
    cb.read(tab.data(), data_size(tab));
  }
}

static DeserializedStaticSceneRayTracerT<uint16_t> *load_frt16(IGenLoad &cb)
{
  auto *rt = new DeserializedStaticSceneRayTracerT<uint16_t>();
  if (!rt->serializedLoad(cb))
  {
    delete rt;
    rt = NULL;
  }
  return rt;
}

void CollisionResource::load(IGenLoad &_cb, int res_id)
{
  collisionFlags = 0;
  gridForTraceable.tracer.reset(nullptr);
  gridForCollidable.tracer.reset(nullptr);
  allNodesList.clear();

  unsigned label = _cb.readInt();
  G_ASSERTF_RETURN((label & 0xFFFF0000) == 0xACE50000, , "Invalid collision resource: 0x%8X", label);

  int version = (label & 0xFFFF);
  if (version == 0)
    return loadLegacyRawFormat(_cb, res_id);

  unsigned btag = 0;
  const unsigned compr_data_sz = _cb.beginBlock(&btag);
  uint8_t zcrdStorage[max(sizeof(ZstdLoadCB), sizeof(OodleLoadCB))];
  String tmp_str;

  IGenLoad *zcrd = nullptr;
  if (btag == btag_compr::ZSTD)
    zcrd = new (zcrdStorage, _NEW_INPLACE) ZstdLoadCB(_cb, compr_data_sz);
  else if (btag == btag_compr::OODLE)
    zcrd = new (zcrdStorage, _NEW_INPLACE) OodleLoadCB(_cb, compr_data_sz - 4, _cb.readInt());
  else
    zcrd = &_cb;

  zcrd->read(&vFullBBox, sizeof(vFullBBox));
  zcrd->read(&vBoundingSphere, sizeof(vBoundingSphere));
  zcrd->read(&boundingBox, sizeof(boundingBox));
  zcrd->readReal(boundingSphereRad);
  collisionFlags = zcrd->readInt();

  if (collisionFlags & COLLISION_RES_FLAG_HAS_TRACE_FRT)
    gridForTraceable.tracer.reset(load_frt16(*zcrd));
  if ((collisionFlags & COLLISION_RES_FLAG_HAS_COLL_FRT) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
    gridForCollidable.tracer.reset(load_frt16(*zcrd));

  reserve_and_resize(allNodesList, zcrd->readInt());
  for (auto &n : allNodesList)
  {
    zcrd->readString(n.name);
    zcrd->readString(tmp_str);
    n.physMatId = PhysMat::getMaterialId(tmp_str.str());

    zcrd->read(&n.modelBBox, sizeof(n.modelBBox));
    zcrd->read(&n.boundingSphere, sizeof(n.boundingSphere));
    n.behaviorFlags = zcrd->readIntP<2>();
    n.flags = zcrd->readIntP<1>();
    n.type = zcrd->readIntP<1>();
    zcrd->readReal(n.cachedMaxTmScale);
    zcrd->read(&n.tm, sizeof(n.tm));
    n.insideOfNode = zcrd->readIntP<2>();

    reserve_and_resize(n.convexPlanes, zcrd->readIntP<2>());
    zcrd->readTabData(n.convexPlanes);

    if (int cnt = zcrd->readInt())
    {
      if (n.flags & n.FLAG_VERTICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        n.resetVertices({&((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer->verts(ofs & 0xFFFFFF), cnt});
      }
      else
      {
        n.resetVertices({memalloc_typed<Point3_vec4>(cnt, midmem), cnt});
        zcrd->read(n.vertices.data(), data_size(n.vertices));
      }
    }

    if (int cnt = zcrd->readInt())
    {
      if (n.flags & n.FLAG_INDICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        n.resetIndices({((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer->faces(0).v + (ofs & 0xFFFFFF), cnt});
      }
      else
      {
        n.resetIndices({memalloc_typed<uint16_t>(cnt, midmem), cnt});
        zcrd->read(n.indices.data(), data_size(n.indices));
      }
    }
    if (n.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      n.capsule.set(n.modelBBox);
      n.capsule.transform(n.tm);
    }

    n.nodeIndex = &n - allNodesList.data();
  }
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
  {
    reserve_and_resize(relGeomNodeTms, allNodesList.size());
    zcrd->read(relGeomNodeTms.data(), data_size(relGeomNodeTms));
  }
  if (zcrd != &_cb)
    zcrd->~IGenLoad();

  rebuildNodesLL();
}

void CollisionResource::loadLegacyRawFormat(IGenLoad &_cb, int res_id, int (*resolve_phmat)(const char *))
{
  int version = _cb.readInt();
  bool hasMaterialData = version >= 0x20150115;
  bool hasCollisionFlags = version >= 0x20180510;
  if (version != 0x20200300 && version != 0x20180510 && version != 0x20160120 && version != 0x20150115 && version != 0x20080925)
    DAG_FATAL("Invalid collision resource version %#08X", version);

  BSphere3 boundingSphere;
  _cb.beginBlock();
  _cb.read(&boundingSphere, sizeof(BSphere3));
  _cb.endBlock();
  vBoundingSphere = v_perm_xyzd(v_ldu(&boundingSphere.c.x), v_splats(boundingSphere.r2));
  boundingSphereRad = boundingSphere.r;

  unsigned int blockFlags = 0;
  const int blockSize = _cb.beginBlock(&blockFlags);
  struct MemoryChainedData *unpacked_data = nullptr;

  if (blockFlags == btag_compr::ZSTD)
  {
    MemorySaveCB cwrUnpack(clamp((blockSize * 4 + 0xFFF) & ~0xFFF, 2 << 10, 64 << 10));
    zstd_decompress_data(cwrUnpack, _cb, blockSize);
    unpacked_data = cwrUnpack.takeMem();
  }
  else if (blockFlags == btag_compr::OODLE)
  {
    int decompSize = _cb.readInt();
    MemorySaveCB cwrUnpack(eastl::min(decompSize, 64 << 10));
    oodle_decompress_data(cwrUnpack, _cb, blockSize - sizeof(int), decompSize);
    unpacked_data = cwrUnpack.takeMem();
  }

  MemoryLoadCB crd(unpacked_data, true);
  IGenLoad &cb = blockFlags != btag_compr::NONE ? static_cast<IGenLoad &>(crd) : _cb;

  if (hasCollisionFlags)
    collisionFlags = cb.readInt();
  unsigned int numNodes = cb.readInt();
  reserve_and_resize(allNodesList, numNodes);
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    reserve_and_resize(relGeomNodeTms, numNodes);

  dag::RelocatableFixedVector<int, 4096> tempIndices;
  dag::RelocatableFixedVector<Point3, 1024> tempVertices;
  SmallTab<Plane3, TmpmemAlloc> tempConvexPlanes;
  bbox3f totalBBox;
  v_bbox3_init_empty(totalBBox); // recalc bbox from nodes
  for (size_t nodeNo = 0; nodeNo < numNodes; nodeNo++)
  {
    CollisionNode &node = allNodesList[nodeNo];

    node.nodeIndex = nodeNo;

    cb.readString(node.name);
    if (hasMaterialData)
    {
      SimpleString matName;
      cb.readString(matName);
      node.physMatId = resolve_phmat ? resolve_phmat(matName) : PhysMat::getMaterialId(matName.str());
    }
    else
      node.physMatId = PHYSMAT_INVALID;

    uint32_t typeAndBehFlags = cb.readInt();
    node.type = typeAndBehFlags & 0xFF;
    if ((collisionFlags & COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS) == COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS)
    {
      cb.read(&node.behaviorFlags, 1);
      node.behaviorFlags = (typeAndBehFlags & 0xFF00) | (node.behaviorFlags & 0x00FF);
    }

    cb.read(&node.tm, sizeof(TMatrix));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
      cb.read(&relGeomNodeTms[nodeNo], sizeof(TMatrix));
    node.flags = 0;
    mat44f vNodeTm;
    v_mat44_make_from_43cu_unsafe(vNodeTm, node.tm.array);
    float dot01 = v_extract_x(v_dot3_x(vNodeTm.col0, vNodeTm.col1));
    float dot02 = v_extract_x(v_dot3_x(vNodeTm.col0, vNodeTm.col2));
    float dot12 = v_extract_x(v_dot3_x(vNodeTm.col1, vNodeTm.col2));
    float len0sq = v_extract_x(v_length3_sq_x(vNodeTm.col0));
    float len1sq = v_extract_x(v_length3_sq_x(vNodeTm.col1));
    float len2sq = v_extract_x(v_length3_sq_x(vNodeTm.col2));
    float len3sq = v_extract_x(v_length3_sq_x(vNodeTm.col3));
    const float eps = 1e-3;
    if (fabs(dot01) < eps && fabs(dot02) < eps && fabs(dot12) < eps && fabsf(len0sq - len1sq) < eps && fabsf(len0sq - len2sq) < eps)
    {
      if (fabs(len0sq - 1.f) < eps && fabs(len1sq - 1.f) < eps && fabs(len2sq - 1.f) < eps)
      {
        node.flags = node.ORTHONORMALIZED;
        if (node.tm.getcol(0).x == 1.f && node.tm.getcol(1).y == 1.f && node.tm.getcol(2).z == 1.f)
        {
          if (len3sq < eps)
            node.flags |= node.IDENT;
          else
            node.flags |= node.TRANSLATE;
        }
      }
      else
        node.flags = node.ORTHOUNIFORM;
    }
    node.cachedMaxTmScale = sqrtf(max(len0sq, max(len1sq, len2sq)));

    cb.read(&node.boundingSphere, sizeof(BSphere3));
    cb.read(&node.modelBBox, sizeof(BBox3));
    if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      node.capsule.set(node.modelBBox);
      node.capsule.transform(node.tm);
    }
    if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      readTab(cb, tempConvexPlanes);
      reserve_and_resize(node.convexPlanes, tempConvexPlanes.size());
      for (int i = 0; i < tempConvexPlanes.size(); ++i)
      {
        tempConvexPlanes[i].normalize();
        node.convexPlanes[i] = v_ldu(&tempConvexPlanes[i].n.x);
      }
    }
    readTab(cb, tempVertices);
    if (!tempVertices.empty())
    {
      int verticesLimit = eastl::numeric_limits<decltype(node.indices)::value_type>::max();
      if (tempVertices.size() > verticesLimit)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Mesh vertexes count %i > %i in node <%s> of res <%s>", tempVertices.size(), verticesLimit, node.name.c_str(),
          resName.c_str());
      }
      node.vertices.set(memalloc_typed<Point3_vec4>(tempVertices.size(), midmem), tempVertices.size());
      for (int i = 0; i < tempVertices.size(); ++i)
      {
        node.vertices[i] = tempVertices[i];
        node.vertices[i].resv = 1.0f;
      }
    }

    readTab(cb, tempIndices);
    bbox3f nodeBBox = v_ldu_bbox3(node.modelBBox);
    if (!tempIndices.empty() && !node.vertices.empty())
    {
      v_bbox3_init_empty(nodeBBox);
      for (int i = 0; i < node.vertices.size(); i++)
      {
        vec3f vert = v_ldu(&node.vertices[i].x);
        v_bbox3_add_pt(nodeBBox, vert);
      }

      v_stu_bbox3(node.modelBBox, nodeBBox);
      node.indices.set(memalloc_typed<uint16_t>(tempIndices.size(), midmem), tempIndices.size());
      for (int i = 0; i < tempIndices.size(); i += 3) // rotate (0,1,2)->(0,2,1) and narrow-convert int -> uint16_t
      {
        node.indices[i + 0] = (uint16_t)tempIndices[i + 0];
        node.indices[i + 2] = (uint16_t)tempIndices[i + 1];
        node.indices[i + 1] = (uint16_t)tempIndices[i + 2];
      }
    }

    if (!v_bbox3_is_empty(nodeBBox))
    {
      if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX || node.type == COLLISION_NODE_TYPE_CAPSULE)
        v_bbox3_init(nodeBBox, vNodeTm, nodeBBox);
      v_bbox3_add_box(totalBBox, nodeBBox);
    }

    if (node.modelBBox.lim[0].x > node.modelBBox.lim[1].x) // Particle View 01, isempty() throws float exception.
    {
      nodeNo--;
      numNodes--;
      allNodesList.resize(numNodes);
    }
  }

  vFullBBox = totalBBox;
  v_stu_bbox3(boundingBox, totalBBox);

  sortNodesList();
  rebuildNodesLL();

  _cb.endBlock();
}

void CollisionResource::sortNodesList()
{
  sort(allNodesList, &sort_by_bbox_size);

  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
  {
    // We must rearrange relGeomNodeTms according to new order after sort
    Tab<TMatrix> relTms;
    reserve_and_resize(relTms, allNodesList.size());
    for (size_t nodeNo = 0; nodeNo < allNodesList.size(); nodeNo++)
    {
      const uint16_t nodeIdx = allNodesList[nodeNo].nodeIndex;
      G_ASSERT(nodeIdx < relGeomNodeTms.size());
      relTms[nodeNo] = relGeomNodeTms[nodeIdx];
    }
    relGeomNodeTms.swap(relTms);
  }

  for (size_t nodeNo1 = 0; nodeNo1 < allNodesList.size(); nodeNo1++)
  {
    for (size_t nodeNo2 = nodeNo1 + 1; nodeNo2 < allNodesList.size(); nodeNo2++)
    {
      bbox3f bbox1 = v_ldu_bbox3(allNodesList[nodeNo1].modelBBox);
      bbox3f bbox2 = v_ldu_bbox3(allNodesList[nodeNo2].modelBBox);
      if (v_bbox3_test_box_inside(bbox1, bbox2)) // bbox2 inside of bbox1.
      {
        float bboxLenSq = v_extract_x(v_length3_sq_x(v_bbox3_size(bbox1)));
        uint16_t n2ins = allNodesList[nodeNo2].insideOfNode;
        if (allNodesList[nodeNo2].insideOfNode == 0xffff ||
            bboxLenSq < v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(allNodesList[n2ins].modelBBox)))))
        {
          allNodesList[nodeNo2].insideOfNode = nodeNo1;
        }
      }
    }
  }
}
void CollisionResource::rebuildNodesLL()
{
  meshNodesHead = boxNodesHead = sphereNodesHead = capsuleNodesHead = NULL;
  numMeshNodes = numBoxNodes = 0;
  CollisionNode *meshNodesTail = NULL, *boxNodesTail = NULL, *sphereNodesTail = NULL, *capsuleNodesTail = NULL;
  for (size_t nodeNo = 0; nodeNo < allNodesList.size(); nodeNo++)
  {
    CollisionNode &node = allNodesList[nodeNo];
    node.nextNode = NULL;

    node.nodeIndex = nodeNo;

    if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      if (meshNodesHead)
      {
        G_ASSERT(node.vertices.size() && node.indices.size());
        meshNodesTail->nextNode = &node;
        meshNodesTail = &node;
      }
      else
        meshNodesHead = meshNodesTail = &node;
      ++numMeshNodes;
    }
    else if (node.type == COLLISION_NODE_TYPE_BOX)
    {
      if (boxNodesHead)
      {
        boxNodesTail->nextNode = &node;
        boxNodesTail = &node;
      }
      else
        boxNodesHead = boxNodesTail = &node;
      ++numBoxNodes;
    }
    else if (node.type == COLLISION_NODE_TYPE_SPHERE)
    {
      if (sphereNodesHead)
      {
        sphereNodesTail->nextNode = &node;
        sphereNodesTail = &node;
      }
      else
        sphereNodesHead = sphereNodesTail = &node;
    }
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      if (capsuleNodesHead)
      {
        capsuleNodesTail->nextNode = &node;
        capsuleNodesTail = &node;
      }
      else
        capsuleNodesHead = capsuleNodesTail = &node;
    }
  }
}

const CollisionNode *CollisionResource::getNode(uint32_t index) const
{
  return index < allNodesList.size() ? allNodesList.data() + index : nullptr;
}

CollisionNode *CollisionResource::getNode(uint32_t index)
{
  return index < allNodesList.size() ? allNodesList.data() + index : nullptr;
}

int CollisionResource::getNodeIndexByName(const char *name) const
{
  if (name && *name)
    for (int i = 0; i < allNodesList.size(); i++)
      if (strcmp(allNodesList[i].name, name) == 0)
        return i;
  return -1;
}

CollisionNode *CollisionResource::getNodeByName(const char *name) { return getNode(getNodeIndexByName(name)); }

const CollisionNode *CollisionResource::getNodeByName(const char *name) const { return getNode(getNodeIndexByName(name)); }

template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, typename filter_t,
  typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir,
  float len, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
  TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  CollisionTrace in;
  in.vFrom = from;
  in.vDir = dir;
  in.vTo = v_madd(dir, v_splats(len), from);
  in.t = len;
  in.isectBounding = false;

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<trace_mode, trace_type, true /*is_single_ray*/>(tm, geom_node_tree, traces, calc_normal, bsphere_scale,
    behavior_filter, filter, callback, out_stats, force_no_cull);
}

// Heavy all-in-one node iterator, calculations unused by caller and deadcode will be automatically stripped by compiler
template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool is_single_ray,
  typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f original_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  TIME_PROFILE_DEV(collres_trace);

  // Move instance_tm to zero for better precision
  // Line below needed to bypass 32-bit compiler bug when original instance tm passed by constant pointer
  // was modified by "tm.col3 = v_zero()" instead of it's copy in arguments of that forceinline function
  const mat44f tm = {original_tm.col0, original_tm.col1, original_tm.col2, v_zero()};
  vec3f woffset = original_tm.col3;

  for (CollisionTrace &trace : traces)
  {
    // Move all traces following instance_tm
    trace.vFrom = v_sub(trace.vFrom, woffset);
    trace.vTo = v_sub(trace.vTo, woffset);
  }

  vec4f maxScaleSq = v_mat44_max_scale43_sq(tm);
  vec3f xyzzScaleSq = v_mat44_scale43_sq(tm);

  // Check bounding
  bool anyTraceIntersectsBounding = false;
  {
    vec3f vBsphCenter;
    if (geom_node_tree && bsphereCenterNode)
      vBsphCenter = v_add(geom_node_tree->getNodeWposRel(bsphereCenterNode), v_sub(geom_node_tree->getWtmOfs(), woffset));
    else
      vBsphCenter = v_mat44_mul_vec3p(tm, vBoundingSphere);
    vec4f vBsphR2 = v_mul_x(v_mul_x(maxScaleSq, v_splat_w(vBoundingSphere)), v_set_x(bsphere_scale * bsphere_scale));
    for (CollisionTrace &trace : traces)
    {
      vec4f vExtBsphR2 = vBsphR2;
      if (trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT)
      {
        vec4f vExtBsphR = v_add_x(v_sqrt_x(vBsphR2), v_set_x(trace.capsuleRadius));
        vExtBsphR2 = v_mul_x(vExtBsphR, vExtBsphR);
      }
      trace.isectBounding = DAGOR_LIKELY(trace.t > VERY_SMALL_NUMBER) &&
                            v_test_ray_sphere_intersection(trace.vFrom, trace.vDir, v_splats(trace.t), vBsphCenter, vExtBsphR2);

#if defined(_WIN32) && DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0
      G_ASSERT(!check_nan(v_extract_x(trace.vDir)) && !check_nan(v_extract_y(trace.vDir)) && !check_nan(v_extract_z(trace.vDir)) &&
               !check_nan(trace.t));
#endif

      float dirLenSq = v_extract_x(v_length3_sq_x(trace.vDir));
      if (DAGOR_UNLIKELY(fabsf(dirLenSq - 1.f) > 0.01f))
      {
        if (dirLenSq > VERY_SMALL_NUMBER)
          logerr("Not normalized dir " FMT_P3 " lenSq=%f used in collres.traceRay (tm scale=%f)", V3D(trace.vDir), dirLenSq,
            sqrtf(v_extract_x(maxScaleSq)));
        trace.isectBounding &= dirLenSq > VERY_SMALL_NUMBER;
      }

      anyTraceIntersectsBounding |= trace.isectBounding;
    }
    if (is_single_ray) // hint for optimizer, helps to remove !trace.isectBounding checks for each node
      anyTraceIntersectsBounding = traces.front().isectBounding;
  }

  bool res = false;
  if (anyTraceIntersectsBounding)
  {
    // 0.05m of absolute error on average https://www.desmos.com/calculator/cdw6uasxlo
    const float eps = eastl::min(0.008f, 0.1f / getBoundingSphereRad());
    // Scaled tm requires more complicated norm calculation code and additional T conversion from world to local basis and back
    vec3f otmMask = v_and(v_cmp_gt(xyzzScaleSq, v_splats(1.f - eps)), v_cmp_lt(xyzzScaleSq, v_splats(1.f + eps)));
    bool bIsOrthonormalizedTm = v_check_xyzw_all_true(otmMask);

#if VERIFY_TRACE_RESULTS
    dag::Vector<CollisionTrace> initialTraces(traces.begin(), traces.end());
#endif

    auto cb_wrapper = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
      pos = v_add(pos, woffset); // Fix isect position for callback. Warning: trace.vFrom and trace.vTo isn't fixed!

#if VERIFY_TRACE_RESULTS
      bool verified = true;
      float normLen = 0.f;
      Point3_vec4 n, p, from, to;
      v_st(&n.x, normal);
      v_st(&p.x, pos);
      v_st(&from.x, initialTraces[trace_id].vFrom);
      v_st(&to.x, initialTraces[trace_id].vTo);
      verified &= !check_nan(t);
      verified &= t >= 0 && t <= initialTraces[trace_id].t;
      if (calc_normal)
      {
        normLen = v_extract_x(v_length3_x(normal));
        verified &= are_approximately_equal(normLen, 1.f, 0.005f);
        verified &= !check_nan(n);
      }
      verified &= !check_nan(p);
      if (!verified)
      {
        logerr("Trace %i intersection verification failed: from " FMT_P3 " to " FMT_P3 " max_t %f calc_norm %i", trace_id, P3D(from),
          P3D(to), initialTraces[trace_id].t, calc_normal);
        logerr("Results: t %f, norm " FMT_P3 " norm_len %.3f isect_pos " FMT_P3 " node_type %i", t, P3D(n), normLen, P3D(p),
          node->type);
        if (calc_normal && (trace_type == CollisionTraceType::RAY_HIT || trace_type == CollisionTraceType::CAPSULE_HIT))
          logerr("Normal calculation is not supported in ray hit traces");
      }
#endif
      return callback(trace_id, node, t, normal, pos);
    };

    if (DAGOR_LIKELY(bIsOrthonormalizedTm))
      res = forEachIntersectedNode<true, trace_mode, trace_type, is_single_ray>(tm, 1.f /* max_tm_scale_sq */, woffset, geom_node_tree,
        traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull);
    else
      res = forEachIntersectedNode<false, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, geom_node_tree,
        traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull);
  }

#if DAGOR_DBGLEVEL > 0
  addTracesProfileTag(traces);
#endif

  // Fix traces to true world coords back
  for (CollisionTrace &trace : traces)
  {
    trace.vFrom = v_add(trace.vFrom, woffset);
    trace.vTo = v_add(trace.vTo, woffset);
  }

  return res;
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4701) // potentially uninitialized local variable 'XXX' used
#endif

template <bool orthonormalized_instance_tm, CollisionResource::IterationMode trace_mode,
  CollisionResource::CollisionTraceType trace_type, bool is_single_ray, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(const mat44f tm, float max_tm_scale_sq, vec3f woffset,
  const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter,
  const filter_t &filter, const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  bool hasCollision = false;
  bool isTraceByCapsule = trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT;
  calc_normal &= trace_type == CollisionTraceType::TRACE_RAY || trace_type == CollisionTraceType::TRACE_CAPSULE;
  const auto *tracer = getTracer(behavior_filter);
  if (!tracer || isTraceByCapsule)
  {
    TIME_PROFILE_DEV(collres_trace_mesh_no_grid);
    ProfileStats profileStats;
    profileStats.meshNodesNum = numMeshNodes;
#if DAGOR_DBGLEVEL > 0
    Finally addTag([&profileStats] { addMeshNodesProfileTag(profileStats); });
#endif

    bbox3f traceBox;
    if (!is_single_ray)
    {
      v_bbox3_init_empty(traceBox);
      for (const CollisionTrace &trace : traces)
      {
        if (!trace.isectBounding)
          continue;
        vec3f rMin = v_min(trace.vFrom, trace.vTo);
        vec3f rMax = v_max(trace.vFrom, trace.vTo);
        if (isTraceByCapsule)
        {
          rMin = v_sub(rMin, v_splats(trace.capsuleRadius));
          rMax = v_add(rMax, v_splats(trace.capsuleRadius));
        }
        v_bbox3_add_pt(traceBox, rMin);
        v_bbox3_add_pt(traceBox, rMax);
      }
    }
    for (const CollisionNode *meshNode = meshNodesHead; DAGOR_LIKELY(meshNode); meshNode = meshNode->nextNode)
    {
      if (!filter(meshNode))
        continue;

      alignas(EA_CACHE_LINE_SIZE) mat44f nodeTm, nodeItm;
      nodeTm = getMeshNodeTmInline(meshNode, tm, woffset, geom_node_tree);
      vec3f bsphCenter = v_mat44_mul_vec3p(nodeTm, v_ldu(&meshNode->boundingSphere.c.x));
      float bsphR2 = meshNode->boundingSphere.r2 * max_tm_scale_sq * sqr(meshNode->cachedMaxTmScale);
      if (is_single_ray)
      {
        vec3f vFrom = traces.front().vFrom;
        vec3f vDir = traces.front().vDir;
        float r2 = isTraceByCapsule ? sqr(sqrtf(bsphR2) + traces.front().capsuleRadius) : bsphR2;
        if (!v_test_ray_sphere_intersection(vFrom, vDir, v_splats(traces.front().t), bsphCenter, v_set_x(r2)))
          continue;
      }
      else
      {
        if (!v_bbox3_test_sph_intersect(traceBox, bsphCenter, v_set_x(bsphR2)))
          continue;
      }
      profileStats.meshNodesSphCheckPassed++;

      // both instance and node tms
      bool isOrthonormalizedTm = orthonormalized_instance_tm && (meshNode->flags & CollisionNode::ORTHONORMALIZED);
      bool isOrthouniformTm =
        orthonormalized_instance_tm && (meshNode->flags & (CollisionNode::ORTHOUNIFORM | CollisionNode::ORTHONORMALIZED));

      if (DAGOR_LIKELY(isOrthouniformTm))
      {
        v_mat44_orthonormal_inverse43(nodeItm, nodeTm);
        if (DAGOR_UNLIKELY(!isOrthonormalizedTm))
        {
          vec4f scale = v_splat_x(v_rcp_x(v_set_x(meshNode->cachedMaxTmScale)));
          nodeItm.col0 = v_mul(nodeItm.col0, scale);
          nodeItm.col1 = v_mul(nodeItm.col1, scale);
          nodeItm.col2 = v_mul(nodeItm.col2, scale);
        }
      }
      else
        v_mat44_inverse43(nodeItm, nodeTm);

      for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
      {
        CollisionTrace &trace = traces[traceId];
        if (!trace.isectBounding)
          continue;

        vec3f vNodeLocalFrom = v_mat44_mul_vec3p(nodeItm, trace.vFrom);
        vec3f vNodeLocalDir;
        float localT;

        if (isOrthonormalizedTm)
        {
          vNodeLocalDir = v_mat44_mul_vec3v(nodeItm, trace.vDir);
          localT = trace.t;
        }
        else
        {
          vec3f vNodeLocalTo = v_mat44_mul_vec3p(nodeItm, trace.vTo);
          vec3f vNodeLocalFullDir = v_sub(vNodeLocalTo, vNodeLocalFrom);
          vec4f vNodeLocalT = v_length3(vNodeLocalFullDir);
          vNodeLocalDir = v_div(vNodeLocalFullDir, vNodeLocalT);
          localT = v_extract_x(vNodeLocalT);
          if (DAGOR_UNLIKELY(localT < VERY_SMALL_NUMBER))
            continue;
        }
        float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * (localT / trace.t) : 0.f;

        // check mesh node bounding
        bbox3f bbox = v_ldu_bbox3(meshNode->modelBBox);
        if (isTraceByCapsule)
        {
          // For capsule trace we do simple bbox extension by capsule radius and test ray intersection.
          // It's very fast and gives very low amount of extra false intersections on small radiuses (<<box size).
          v_bbox3_extend(bbox, v_splats(localCapsuleRadius));
        }
        if (!v_test_ray_box_intersection_unsafe(vNodeLocalFrom, vNodeLocalDir, v_set_x(localT), bbox)) // slower than 'ray vs sphere',
                                                                                                       // but more precise for big
                                                                                                       // objects like vehicles
          continue;
        profileStats.meshNodesBoxCheckPassed++;
        profileStats.meshTrianglesTraced += meshNode->indices.size() / 3;

        IF_CONSTEXPR (trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT ||
                      trace_type == CollisionTraceType::CAPSULE_HIT)
        {
          if (out_stats && meshNode->nodeIndex < out_stats->size())
            (*out_stats)[meshNode->nodeIndex]++;

          float inOutLocalT = localT;
          vec3f vNodeLocalNorm, vNodeLocalCapsuleHitPos;
          vec3f *normPtr = calc_normal ? &vNodeLocalNorm : nullptr;

          bool isHit = false;
          bool isLightNode = meshNode->indices.size() < traceMeshNodeLocalApi.threshold;
          switch (trace_type)
          {
            case CollisionTraceType::TRACE_RAY:
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalCullCCW)(*meshNode, vNodeLocalFrom,
                vNodeLocalDir, inOutLocalT, normPtr);
              break;
            case CollisionTraceType::TRACE_CAPSULE:
              isHit = traceCapsuleMeshNodeLocalCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius,
                vNodeLocalNorm, vNodeLocalCapsuleHitPos);
              break;
            case CollisionTraceType::RAY_HIT:
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnRayHitMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnRayHitMeshNodeLocalCullCCW)(*meshNode, vNodeLocalFrom,
                vNodeLocalDir, inOutLocalT);
              break;
            case CollisionTraceType::CAPSULE_HIT:
              isHit = capsuleHitMeshNodeLocalCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius);
              break;
            default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
          }
          if (isHit)
          {
            hasCollision = true;
            float intersectionT = trace.t * (inOutLocalT / localT);
            vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
            vec3f vIntersectionNorm = v_zero();
            if (trace_mode == FIND_BEST_INTERSECTION)
            {
              trace.t = intersectionT;
              trace.vTo = vIntersectionPos;
            }
            if (calc_normal)
            {
              if (DAGOR_LIKELY(isOrthouniformTm))
                vIntersectionNorm = v_mat44_mul_vec3v(nodeTm, vNodeLocalNorm);
              else
              {
                mat33f itm33, titm33;
                v_mat33_from_mat44(itm33, nodeItm);
                v_mat33_transpose(titm33, itm33);
                vIntersectionNorm = v_mat33_mul_vec3(titm33, vNodeLocalNorm);
              }
              vIntersectionNorm = v_norm3(vIntersectionNorm);
            }
            if (trace_type == CollisionTraceType::TRACE_CAPSULE)
            {
              // For capsule trace we have custom intersection pos output, which often lie not on a ray
              vIntersectionPos = v_mat44_mul_vec3p(nodeTm, vNodeLocalCapsuleHitPos);
            }
            profileStats.meshTrianglesHits++;
            callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos);
            if (trace_mode == ANY_ONE_INTERSECTION)
              return hasCollision;
            if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
            {
              trace.isectBounding = false;
              if (is_single_ray)
                return hasCollision;
            }
          }
        }
        else IF_CONSTEXPR (trace_mode == ALL_INTERSECTIONS)
        {
          if (trace_type == CollisionTraceType::TRACE_CAPSULE)
          {
            G_ASSERT(false); //-V1037 not implemented yet. It's simple, but will generate too much intersections
                             // and need to be redesigned. Maybe return only the best node IN and OUT positions?
            continue;
          }

          if (out_stats && meshNode->nodeIndex < out_stats->size())
            (*out_stats)[meshNode->nodeIndex]++;

          all_nodes_ret_t ret;
          bool isLightNode = meshNode->indices.size() < traceMeshNodeLocalApi.threshold;
          bool isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                                    : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(*meshNode, vNodeLocalFrom,
            vNodeLocalDir, localT, calc_normal, force_no_cull, ret);
          if (isHit)
          {
            hasCollision = true;
            mat33f titm33;
            if (!isOrthouniformTm && calc_normal)
            {
              mat33f itm33;
              v_mat33_from_mat44(itm33, nodeItm);
              v_mat33_transpose(titm33, itm33);
            }
            for (auto n_t : ret)
            {
              float intersectionT = trace.t * (v_extract_w(n_t) / localT);
              vec3f vIntersectionNorm = v_zero();
              vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
              if (calc_normal)
              {
                mat33f normTm;
                v_mat33_from_mat44(normTm, nodeTm);
                if (DAGOR_UNLIKELY(!isOrthouniformTm))
                  normTm = titm33;
                vIntersectionNorm = v_norm3(v_mat33_mul_vec3(normTm, n_t));
              }
              callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos);
            }
            profileStats.meshTrianglesHits += ret.size();
          }
        } // allow multiple intersection of one node or not
      }   // traces loop
    }     // mesh nodes loop

    if (DAGOR_LIKELY(!boxNodesHead && !sphereNodesHead && !capsuleNodesHead))
      return hasCollision;
  } // if (!tracer)

  alignas(EA_CACHE_LINE_SIZE) mat44f itm;
  if (orthonormalized_instance_tm)
    v_mat44_orthonormal_inverse43(itm, tm);
  else
    v_mat44_inverse43(itm, tm);

  // for normals
  mat33f titm33;
  bool titmCalculated = false;

  for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
  {
    CollisionTrace &trace = traces[traceId];
    if (!trace.isectBounding)
      continue;

    vec3f vLocalFrom = v_mat44_mul_vec3p(itm, trace.vFrom);
    vec3f vLocalTo, vLocalDir;
    typename eastl::type_select<orthonormalized_instance_tm, float &, float>::type localT = trace.t;

    if (orthonormalized_instance_tm)
    {
      vLocalDir = v_mat44_mul_vec3v(itm, trace.vDir);
      vLocalTo = v_madd(vLocalDir, v_splats(trace.t), vLocalFrom);
      localT = trace.t;
    }
    else
    {
      vLocalTo = v_mat44_mul_vec3p(itm, trace.vTo);
      vLocalDir = v_sub(vLocalTo, vLocalFrom);
      vec4f vLocalT = v_length3(vLocalDir);
      vLocalDir = v_div(vLocalDir, vLocalT);
      localT = v_extract_x(vLocalT);
      if (DAGOR_UNLIKELY(localT < VERY_SMALL_NUMBER))
        continue;
    }
    float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * (localT / trace.t) : 0.f;

    if (tracer && !isTraceByCapsule)
    {
      TIME_PROFILE_DEV(collres_trace_mesh_grid);
#if DA_PROFILER_ENABLED
      int tris = getTrianglesCount(behavior_filter);
      DA_PROFILE_TAG(tris, ": %u", tris);
#endif

      // moved it in (tracer) block for rendinsts only, because in most cases we have not more than one not-mesh node
      // don't forget to extend vFullBBox by localCapsuleRadius if you want to change it
      if (!v_test_segment_box_intersection(vLocalFrom, vLocalTo, vFullBBox))
        continue;

      IF_CONSTEXPR (trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT)
      {
        vec3f vGridLocalFrom = vLocalFrom;
        float gridLocalT = localT;
        float incT = 0.f;
        int lastFaceId = -1;
        for (;;)
        {
          int faceId = -1;
          switch (trace_type)
          {
            case CollisionTraceType::TRACE_RAY:
              faceId = tracer->tracerayNormalized(vGridLocalFrom, vLocalDir, gridLocalT, lastFaceId);
              break;
            case CollisionTraceType::RAY_HIT:
              faceId = tracer->rayhitNormalizedIdx(vGridLocalFrom, vLocalDir, gridLocalT, lastFaceId);
              break;
            default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
          }
          if (faceId < 0)
            break;
          lastFaceId = faceId;

          int nodeIndex = getNodeIndexByFaceId(faceId, behavior_filter);
          if (DAGOR_LIKELY(nodeIndex >= 0))
          {
            const CollisionNode *meshNode = &allNodesList[nodeIndex];
            if (filter(meshNode))
            {
              hasCollision = true;
              float localIntersectionT = gridLocalT + incT;
              float intersectionT = trace.t * (localIntersectionT / localT);
              vec3f vIntersectionNorm = v_zero();
              vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
              if (trace_mode == FIND_BEST_INTERSECTION)
              {
                trace.t = intersectionT;
                localT = localIntersectionT;
                vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
              }
              if (calc_normal)
              {
                vec3f vLocalNorm = tracer->getNormal(faceId);
                if (orthonormalized_instance_tm)
                  vIntersectionNorm = v_mat44_mul_vec3v(tm, vLocalNorm);
                else
                {
                  if (!titmCalculated)
                  {
                    mat33f itm33;
                    v_mat33_from_mat44(itm33, itm);
                    v_mat33_transpose(titm33, itm33);
                    titmCalculated = true;
                  }
                  vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vLocalNorm));
                }
              }
              callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos);
              if (trace_mode == ANY_ONE_INTERSECTION)
                return hasCollision;
              if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
                goto next_trace;
              break; // we iterate by grid, next cell will be definitely worst than current
            }
          }

          // continue from intersection pos
          const float threshold = 0.001f;
          float tt = gridLocalT + threshold;
          vGridLocalFrom = v_madd(vLocalDir, v_splats(tt), vGridLocalFrom);
          incT += tt;
          gridLocalT = localT - incT;
        } // intersected faces loop
      }
      else IF_CONSTEXPR (trace_mode == ALL_INTERSECTIONS)
      {
        G_ASSERT(trace_type == CollisionTraceType::TRACE_RAY); // capsule is not supported
        all_faces_ret_t hits;
        bool isHit = tracer->tracerayNormalized(vLocalFrom, vLocalDir, localT, hits);
        if (isHit)
        {
          hasCollision = true;
          if (!orthonormalized_instance_tm && !titmCalculated)
          {
            mat33f itm33;
            v_mat33_from_mat44(itm33, itm);
            v_mat33_transpose(titm33, itm33);
            titmCalculated = true;
          }
          for (auto hit : hits)
          {
            int nodeIndex = getNodeIndexByFaceId(hit.faceId, behavior_filter);
            if (DAGOR_LIKELY(nodeIndex >= 0))
            {
              const CollisionNode *meshNode = &allNodesList[nodeIndex];
              if (filter(meshNode))
              {
                float intersectionT = trace.t * (hit.t / localT);
                vec3f vIntersectionNorm = v_zero();
                vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
                if (calc_normal)
                {
                  vec3f vLocalNorm = tracer->getNormal(hit.faceId);
                  mat33f normTm;
                  v_mat33_from_mat44(normTm, tm);
                  if (!orthonormalized_instance_tm)
                    normTm = titm33;
                  vIntersectionNorm = v_norm3(v_mat33_mul_vec3(normTm, vLocalNorm));
                }
                callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos);
              }
            }
          }
        }
      } // one hit or all intersections
    }   // if (tracer)

    for (const CollisionNode *boxNode = boxNodesHead; boxNode; boxNode = boxNode->nextNode)
    {
      if (!filter(boxNode))
        continue;
      float atMin = 1.f, atMax = 1.f; // [0; 1]
      int side = 0;
      bool isHit = false;
      bbox3f modelBBox = v_ldu_bbox3(boxNode->modelBBox);
      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          side = v_segment_box_intersection_side(vLocalFrom, vLocalTo, modelBBox, atMin, atMax);
          isHit = side != -1;
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          G_ASSERTF(false, "CollisionResource trace failed: capsule trace for box nodes is not implemented yet"); //-V1037
          // Very complex when done correct for all theoretically possible cases
          // Try to use traceRay for collres with box nodes
          break;
        case CollisionTraceType::RAY_HIT: isHit = v_test_segment_box_intersection(vLocalFrom, vLocalTo, modelBBox); break;
        case CollisionTraceType::CAPSULE_HIT:
          G_ASSERTF(false, "CollisionResource trace failed: capsule rayhit for box nodes is not implemented yet"); //-V1037
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * atMin;
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          localT *= atMin; // warning: localT is ref to trace.t if (orthonormalized_instance_tm==true)
          trace.t = intersectionT;
          vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
        }
        if (calc_normal)
        {
          switch (side % 3) // better than index
          {
            case 0: vIntersectionNorm = tm.col0; break;
            case 1: vIntersectionNorm = tm.col1; break;
            case 2: vIntersectionNorm = tm.col2; break;
          }
          if (!orthonormalized_instance_tm)
            vIntersectionNorm = v_norm3(vIntersectionNorm);
          if (side < 3)
            vIntersectionNorm = v_neg(vIntersectionNorm);
        }
        callback(traceId, boxNode, intersectionT, vIntersectionNorm, vIntersectionPos);
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // box nodes loop

    for (const CollisionNode *sphereNode = sphereNodesHead; sphereNode; sphereNode = sphereNode->nextNode)
    {
      if (!filter(sphereNode))
        continue;
      bool isHit = false;
      vec3f bsphPos = v_ldu(&sphereNode->boundingSphere.c.x);
      vec4f bsphR2 = v_set_x(sphereNode->boundingSphere.r2);
      vec4f bsphCapsuleR = v_set_x(sphereNode->boundingSphere.r + localCapsuleRadius);
      vec4f bsphCapsuleR2 = v_mul(bsphCapsuleR, bsphCapsuleR);
      vec4f inOutLocalT = v_set_x(localT);

      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          isHit = v_ray_sphere_intersection(vLocalFrom, vLocalDir, inOutLocalT, bsphPos, bsphR2);
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = v_ray_sphere_intersection(vLocalFrom, vLocalDir, inOutLocalT, bsphPos, bsphCapsuleR2);
          break;
        case CollisionTraceType::RAY_HIT:
          isHit = v_test_ray_sphere_intersection(vLocalFrom, vLocalDir, v_splat_x(inOutLocalT), bsphPos, bsphR2);
          break;
        case CollisionTraceType::CAPSULE_HIT:
          isHit = v_test_ray_sphere_intersection(vLocalFrom, vLocalDir, v_splat_x(inOutLocalT), bsphPos, bsphCapsuleR2);
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * (v_extract_x(inOutLocalT) / localT);
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          trace.t = intersectionT;
          localT = v_extract_x(inOutLocalT);
        }
        if (calc_normal || trace_type == CollisionTraceType::TRACE_CAPSULE)
        {
          vec3f vLocalHitPos = v_madd(vLocalDir, v_splat_x(inOutLocalT), vLocalFrom);
          vec3f vLocalNorm = v_sub(vLocalHitPos, v_ldu(&sphereNode->boundingSphere.c.x));

          if (calc_normal)
          {
            if (orthonormalized_instance_tm)
              vIntersectionNorm = v_norm3(v_mat44_mul_vec3v(tm, vLocalNorm));
            else
            {
              if (!titmCalculated)
              {
                mat33f itm33;
                v_mat33_from_mat44(itm33, itm);
                v_mat33_transpose(titm33, itm33);
                titmCalculated = true;
              }
              vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vLocalNorm));
            }
          }
          if (trace_type == CollisionTraceType::TRACE_CAPSULE)
          {
            vec3f vLocalPos = v_madd(vLocalNorm, v_splats(sphereNode->boundingSphere.r), v_ldu(&sphereNode->boundingSphere.c.x));
            vIntersectionPos = v_mat44_mul_vec3p(tm, vLocalPos);
          }
        }
        callback(traceId, sphereNode, intersectionT, vIntersectionNorm, vIntersectionPos);
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // sphere nodes loop

    for (const CollisionNode *capsuleNode = capsuleNodesHead; capsuleNode; capsuleNode = capsuleNode->nextNode)
    {
      if (!filter(capsuleNode))
        continue;
      float inOutLocalT = localT;
      bool isHit = false;
      vec3f vOutLocalNorm = v_zero();
      vec3f vOutLocalPos = v_zero();

      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          isHit = capsuleNode->capsule.traceRay(vLocalFrom, vLocalDir, inOutLocalT, vOutLocalNorm);
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit =
            capsuleNode->capsule.traceCapsule(vLocalFrom, vLocalDir, inOutLocalT, localCapsuleRadius, vOutLocalNorm, vOutLocalPos);
          break;
        case CollisionTraceType::RAY_HIT: isHit = capsuleNode->capsule.rayHit(vLocalFrom, vLocalDir, localT); break;
        case CollisionTraceType::CAPSULE_HIT:
          isHit = capsuleNode->capsule.capsuleHit(vLocalFrom, vLocalDir, localT, localCapsuleRadius);
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * (inOutLocalT / localT);
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          trace.t = intersectionT;
          localT = inOutLocalT;
        }
        if (calc_normal)
        {
          if (orthonormalized_instance_tm)
            vIntersectionNorm = v_mat44_mul_vec3v(tm, vOutLocalNorm);
          else
          {
            if (!titmCalculated)
            {
              mat33f itm33;
              v_mat33_from_mat44(itm33, itm);
              v_mat33_transpose(titm33, itm33);
              titmCalculated = true;
            }
            vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vOutLocalNorm));
          }
        }
        if (trace_type == CollisionTraceType::TRACE_CAPSULE)
        {
          vIntersectionPos = v_mat44_mul_vec3p(tm, vOutLocalPos);
        }
        callback(traceId, capsuleNode, intersectionT, vIntersectionNorm, vIntersectionPos);
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // capsule nodes loop
  next_trace:;
  } // traces loop

  return hasCollision;
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
  float &in_out_t, Point3 *out_normal, int &out_mat_id, int &out_node_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    out_mat_id = node->physMatId;
    out_node_id = node->nodeIndex;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
  float &in_out_t, Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter, int ray_mat_id,
  uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!filter || filter(node->nodeIndex)) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    if (out_mat_id)
      out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

bool CollisionResource::traceRay(const mat44f &tm, const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal,
  int &out_mat_id, int ray_mat_id, uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, nullptr /*geom_node_tree*/, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, bool force_no_cull) const
{
  intersected_nodes_list.clear();

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->collisionNodeId = node->nodeIndex;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    true /*calc_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, force_no_cull);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale,
  const CollisionNodeFilter &filter, TraceCollisionResourceStats *out_stats, const CollisionNodeMask *collision_node_mask) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  intersected_nodes_list.clear();
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && (!filter || filter(node->nodeIndex)) &&
           (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->collisionNodeId = node->nodeIndex;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    true /*calc_normal*/, bsphere_scale, behaviorFilter, nodeFilter, callback, out_stats, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const CollisionNodeFilter &filter, const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;
  float closest = VERY_BIG_NUMBER;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!filter || filter(node->nodeIndex));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    vec3f cl = closest_point_on_line(pos, in.vFrom, in.vDir);
    float dist = v_extract_x(v_length3_sq_x(v_sub(cl, pos)));
    if (dist < closest)
    {
      closest = dist;
      v_stu_p3(&intersected_node.normal.x, normal);
      intersected_node.intersectionT = t;
      v_stu_p3(&intersected_node.intersectionPos.x, pos);
      intersected_node.collisionNodeId = node->nodeIndex;
    }
  };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::TRACE_CAPSULE, is_single_ray>(tm, geom_node_tree, traces,
    true /*calc_normal*/, bsphere_scale, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, false /*force_no_cull*/);
}

bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    in_out_t = t;
    v_stu_p3(&out_normal.x, normal);
    v_stu_p3(&out_pos.x, pos);
    out_mat_id = node->physMatId;
  };

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_out_t), in.vFrom);
  in.t = in_out_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_CAPSULE, true /*is_single_ray*/>(tm, geom_node_tree,
    traces, true /*out_normal*/, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;
  float closest = VERY_BIG_NUMBER;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behavior_filter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    vec3f cl = closest_point_on_line(pos, in.vFrom, in.vDir);
    float dist = v_extract_x(v_length3_sq_x(v_sub(cl, pos)));
    if (dist < closest)
    {
      closest = dist;
      v_stu_p3(&intersected_node.normal.x, normal);
      intersected_node.intersectionT = t;
      v_stu_p3(&intersected_node.intersectionPos.x, pos);
      intersected_node.collisionNodeId = node->nodeIndex;
    }
  };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::TRACE_CAPSULE, is_single_ray>(tm, geom_node_tree, traces,
    true /*calc_normal*/, bsphere_scale, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, false /*force_no_cull*/);
}

bool CollisionResource::traceMultiRay(const mat44f &tm, dag::Span<CollisionTrace> traces, int ray_mat_id,
  uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/) {
    CollisionTrace &trace = traces[trace_id];
    v_stu_p3(&trace.norm.x, normal);
    trace.t = t;
    trace.outMatId = node->physMatId;
    trace.isHit = true;
  };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.isHit = false;
  }

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, nullptr /*geom_node_tree*/, traces,
    true /*calc_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

bool CollisionResource::traceMultiRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces,
  MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  intersected_nodes_list.clear();

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos) {
    traces[trace_id].isHit = true;

    MultirayIntersectedNode *inode = (MultirayIntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->collisionNodeId = node->nodeIndex;
    inode->rayId = trace_id;
  };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.isHit = false;
  }

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, traces, true /*calc_normal*/,
    bsphere_scale, behavior_filter, nodeFilter, callback, out_stats, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

VECTORCALL bool CollisionResource::traceRayMeshNodeLocal(const CollisionNode &node, vec4f v_local_from, vec4f v_local_dir,
  float &in_out_t, vec4f *v_out_norm) const
{
  if (!node.checkBehaviorFlags(CollisionNode::TRACEABLE))
    return false;

  bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), bbox))
    return false;

  bool isLightNode = node.indices.size() < traceMeshNodeLocalApi.threshold;
  return (isLightNode
            ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalCullCCW
            : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalCullCCW)(node, v_local_from, v_local_dir, in_out_t, v_out_norm);
}


VECTORCALL bool CollisionResource::traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull) const
{
  if (!node.checkBehaviorFlags(CollisionNode::TRACEABLE))
    return false;
  const vec4f vLocalFrom = v_ldu_p3(&from.x);
  const vec4f vLocalDir = v_ldu_p3(&dir.x);
  const bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(vLocalFrom, vLocalDir, v_set_x(in_t), bbox))
    return false;

  all_nodes_ret_t allNodes;
  const bool isLightNode = node.indices.size() < traceMeshNodeLocalApi.threshold;
  // -V::601 for some reason PVS glitches and fails with "The 'true' value becomes a class object" on calc_normal parameter
  const bool res = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                                : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(node, vLocalFrom, vLocalDir, in_t, true,
    force_no_cull, allNodes);

  intersected_nodes_list.reserve(allNodes.size());
  for (const vec4f &normAndT : allNodes)
  {
    auto &n = intersected_nodes_list.push_back();
    n.intersectionT = v_extract_w(normAndT);
    v_stu_p3(&n.intersectionPos.x, v_madd(vLocalDir, v_splat_w(normAndT), vLocalFrom));
    v_stu_p3(&n.normal.x, normAndT);
    n.collisionNodeId = node.nodeIndex;
  }
  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);

  return res;
}

bool CollisionResource::testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth) const
{
  vec4f bsph;
  v_bsph_init(bsph, v_ldu(&sphere.c.x), v_splats(sphere.r));

  if (!v_test_bsph_bsph_intersection(bsph, getBoundingSphereXYZR()))
    return false;

  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if ((filter && !filter(meshNode->nodeIndex)) || (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE)))
      continue;
    Point3 nodeNorm;
    float nodeDepth;
    if (test_sphere_node_intersection(sphere, meshNode, dir_norm, nodeNorm, nodeDepth))
    {
      out_norm = meshNode->tm % nodeNorm;
      out_depth = nodeDepth;
      return true;
    }
  }
  return false;
}

bool CollisionResource::testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius) const
{
  bbox3f bbox;
  vec3f radV = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, v_ldu(&p0.x), radV);
  v_bbox3_add_pt(bbox, v_sub(v_ldu(&p1.x), radV));
  v_bbox3_add_pt(bbox, v_add(v_ldu(&p1.x), radV));

  if (!v_bbox3_test_sph_intersect(bbox, vBoundingSphere, v_splat_w(vBoundingSphere)))
    return false;

  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    if (test_capsule_node_intersection(p0, p1, radius, meshNode))
      return true;
  }
  return false;
}

VECTORCALL bool CollisionResource::traceQuad(const CollisionNodeFilter &filter, vec3f a00, vec3f a01, vec3f a10, vec3f a11,
  Point3 &out_point, int &out_node_index) const
{
  bbox3f bbox;
  v_bbox3_init(bbox, a00);
  v_bbox3_add_pt(bbox, a01);
  v_bbox3_add_pt(bbox, a10);
  v_bbox3_add_pt(bbox, a11);

  if (!v_bbox3_test_sph_intersect(bbox, vBoundingSphere, v_splat_w(vBoundingSphere)))
    return false;

  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if ((filter && !filter(meshNode->nodeIndex)) || (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE)))
      continue;
    mat44f nodeTm, nodeItm;
    v_mat44_make_from_43cu_unsafe(nodeTm, meshNode->tm.array);
    bbox3f nodeBox;
    v_bbox3_init(nodeBox, nodeTm, v_ldu_bbox3(meshNode->modelBBox));
    const uint16_t *__restrict cur = meshNode->indices.data();
    PREFETCH_DATA(0, cur);
    const uint16_t *__restrict end = cur + meshNode->indices.size();
    const Point3_vec4 *__restrict vertices = meshNode->vertices.data();

    if (!v_bbox3_test_box_intersect(nodeBox, bbox))
      continue;

    v_mat44_inverse43(nodeItm, nodeTm);
    vec3f localA00 = v_mat44_mul_vec3p(nodeItm, a00);
    vec3f localA01 = v_mat44_mul_vec3p(nodeItm, a01);
    vec3f localA10 = v_mat44_mul_vec3p(nodeItm, a10);
    vec3f localA11 = v_mat44_mul_vec3p(nodeItm, a11);

    vec3f localQuadUnnormal = v_cross3(v_sub(localA10, localA00), v_sub(localA11, localA10));
    if (v_extract_x(v_length3_sq_x(localQuadUnnormal)) < 1.0e-3f)
      return false;
    vec3f localQuadNormal = v_norm3(localQuadUnnormal);
    vec3f moveDirUnnorm = v_cross3(v_sub(localA11, localA10), localQuadNormal);

    while (cur < end)
    {
      vec3f corner0 = v_ldu(&vertices[*(cur++)].x);
      vec3f corner1 = v_ldu(&vertices[*(cur++)].x);
      vec3f corner2 = v_ldu(&vertices[*(cur++)].x);

      vec3f triangleUnnormal = v_cross3(v_sub(corner2, corner0), v_sub(corner1, corner0));
      if (v_extract_x(v_dot3_x(moveDirUnnorm, triangleUnnormal)) > 0.0f)
        continue;

      bool isIntersected = v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA01, localA11) ||
                           v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA11, localA10);

      if (isIntersected)
      {
        vec3f trgCenter = v_mul(v_add(v_add(corner0, corner1), corner2), v_splats(0.333333f));
        v_stu_p3(&out_point.x, v_mat44_mul_vec3p(nodeTm, trgCenter));
        out_node_index = meshNode->nodeIndex;
        return true;
      }
    }
  }
  return false;
}

template <bool check_bounding>
VECTORCALL DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalCullCCW(const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float &in_out_t, vec4f *v_out_norm)
{
  int resultIdx = -1;
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), box)))
      continue;

    int ret = traceray4TrianglesCullCCW(v_local_from, v_local_dir, in_out_t, vert, batchSize);
    if (ret >= 0)
      resultIdx = i + ret * 3;
  }

  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    int ret = traceray4TrianglesCullCCW(v_local_from, v_local_dir, in_out_t, vert, count);
    if (ret >= 0)
      resultIdx = i + ret * 3;
  }

  if (resultIdx >= 0 && v_out_norm)
  {
    vec4f v0 = v_ld(&vertices[indices[resultIdx + 0]].x);
    vec4f v1 = v_ld(&vertices[indices[resultIdx + 1]].x);
    vec4f v2 = v_ld(&vertices[indices[resultIdx + 2]].x);
    *v_out_norm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
  }
  return resultIdx >= 0;
}

VECTORCALL DAGOR_NOINLINE bool CollisionResource::traceCapsuleMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f v_local_from,
  const vec4f v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos) const
{
  bool ret = false;
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  // float bestScore = FLT_MIN;

  for (uint32_t i = 0; DAGOR_LIKELY(i < indicesSize); i += 3)
  {
    vec3f v0 = v_ld(&vertices[indices[i + 0]].x);
    vec3f v1 = v_ld(&vertices[indices[i + 1]].x);
    vec3f v2 = v_ld(&vertices[indices[i + 2]].x);

    // fast bounding check
    bbox3f bbox;
    v_bbox3_init(bbox, v0);
    v_bbox3_add_pt(bbox, v1);
    v_bbox3_add_pt(bbox, v2);
    if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), bbox))
      continue;

    vec4f norm, pos;
    float t = in_out_t;
    if (!test_capsule_triangle_intersection(v_local_from, v_local_dir, v0, v1, v2, radius, t, norm, pos, false))
      continue;
    float hitRadius = v_extract_x(distance_to_line_x(pos, v_local_from, v_local_dir));
    if (hitRadius < radius)
    {
      ret = true;
      radius = hitRadius;
      in_out_t = t;
      v_out_norm = norm;
      v_out_pos = pos;
    }
  }

  return ret;
}

template <bool check_bounding>
VECTORCALL DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalAllHits(const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float in_t, bool calc_normal, bool force_no_cull, all_nodes_ret_t &ret_array)
{
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();
  bool noCull = force_no_cull || node.checkBehaviorFlags(CollisionNode::SOLID);

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), box)))
      continue;

    vec4f vInOutT = v_splats(in_t);
    int ret = traceray4TrianglesMask(v_local_from, v_local_dir, vInOutT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
      alignas(16) float outT[batchSize];
      v_st(outT, vInOutT);
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < batchSize; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = v_ld(&vertices[indices[i + j * 3 + 0]].x);
            vec4f v1 = v_ld(&vertices[indices[i + j * 3 + 1]].x);
            vec4f v2 = v_ld(&vertices[indices[i + j * 3 + 2]].x);
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
        }
      }
    }
  }

  // tail
  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    vec4f vInOutT = v_splats(in_t);
    int ret = traceray4TrianglesMask(v_local_from, v_local_dir, vInOutT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
      alignas(16) float outT[batchSize];
      v_st(outT, vInOutT);
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < count; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = vert[j][0];
            vec4f v1 = vert[j][1];
            vec4f v2 = vert[j][2];
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
        }
      }
    }
  }
  return !ret_array.empty();
}

template <bool check_bounding>
VECTORCALL DAGOR_NOINLINE bool CollisionResource::rayHitMeshNodeLocalCullCCW(const CollisionNode &node, const vec3f &v_local_from,
  const vec3f &v_local_dir, float in_t)
{
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), box)))
      continue;

    if (rayhit4TrianglesCullCCW(v_local_from, v_local_dir, in_t, vert, batchSize))
      return true;
  }

  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }
    if (rayhit4TrianglesCullCCW(v_local_from, v_local_dir, in_t, vert, count))
      return true;
  }

  return false;
}

VECTORCALL DAGOR_NOINLINE bool CollisionResource::capsuleHitMeshNodeLocalCullCCW(const CollisionNode &node, vec4f v_local_from,
  vec4f v_local_dir, float in_t, float radius) const
{
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  for (uint32_t i = 0; DAGOR_LIKELY(i < indicesSize); i += 3)
  {
    vec3f v0 = v_ld(&vertices[indices[i + 0]].x);
    vec3f v1 = v_ld(&vertices[indices[i + 1]].x);
    vec3f v2 = v_ld(&vertices[indices[i + 2]].x);

    // fast bounding check
    bbox3f bbox;
    v_bbox3_init(bbox, v0);
    v_bbox3_add_pt(bbox, v1);
    v_bbox3_add_pt(bbox, v2);
    if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), bbox))
      continue;

    if (test_capsule_triangle_hit(v_local_from, v_local_dir, v0, v1, v2, radius, in_t, false))
      return true;
  }

  return false;
}

int CollisionResource::getNodeIndexByFaceId(int face_id, uint8_t behavior_filter) const
{
  int facesNum = 0;
  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if (!meshNode->checkBehaviorFlags(behavior_filter))
      continue;
    facesNum += meshNode->indices.size() / 3;
    if (face_id < facesNum)
      return meshNode->nodeIndex;
  }
  return -1;
}

bool CollisionResource::checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const
{
  intersected_nodes_list.clear();
  vec4f vPos = v_ldu(&pos.x);
  if (!v_test_vec_x_le(v_length3_sq(v_sub(vPos, vBoundingSphere)), v_splat_w(vBoundingSphere)))
    return false;

  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    // Test only for bbox
    TMatrix itm = meshNode->getInverseTmFlags();
    Point3 localPos = itm * pos;
    if (meshNode->modelBBox & localPos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->collisionNodeId = meshNode->nodeIndex;
  }

  for (const CollisionNode *boxNode = boxNodesHead; boxNode; boxNode = boxNode->nextNode)
    if (boxNode->modelBBox & pos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->collisionNodeId = boxNode->nodeIndex;

  for (const CollisionNode *sphNode = sphereNodesHead; sphNode; sphNode = sphNode->nextNode)
    if (sphNode->boundingSphere & pos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->collisionNodeId = sphNode->nodeIndex;

  for (const CollisionNode *capNode = capsuleNodesHead; capNode; capNode = capNode->nextNode)
    if (capNode->capsule.isInside(pos))
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->collisionNodeId = capNode->nodeIndex;

  return !intersected_nodes_list.empty();
}

bool CollisionResource::calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move,
  const CollisionNode &node_to_check, Point3 &offset)
{
  offset.zero();

  G_ASSERTF(node_to_check.type == COLLISION_NODE_TYPE_CONVEX, "final node '%s' is not of convex type, only convex is supported",
    node_to_check.name.str());
  TMatrix finalTm = node_to_check.getInverseTmFlags() * tm1;
  const int numIterations = 5;
  vec4f vOffset = v_zero();
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
    {
      Point3_vec4 point = finalTm * node_to_move.modelBBox.point(i);
      boundPoints[i] = v_ld(&point.x);
    }
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < boundPoints.size(); ++i)
      {
        for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(node_to_check.convexPlanes[j], v_add(boundPoints[i], vOffset)), v_zero());
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), node_to_check.convexPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_SPHERE)
  {
    BSphere3 localSph = finalTm * node_to_move.boundingSphere;
    vec4f sph = v_ldu(&localSph.c.x);
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
      {
        vec4f dist = v_max(v_add(v_plane_dist_x(node_to_check.convexPlanes[j], v_add(sph, vOffset)), v_splat_w(sph)), v_zero());
        hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
        vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), node_to_check.convexPlanes[j]));
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    Tab<vec4f> vertices(tmpmem);
    reserve_and_resize(vertices, node_to_move.vertices.size());
    for (int i = 0; i < vertices.size(); ++i)
      vertices[i] = v_mat44_mul_vec3p(vFinalTm, v_ld(&node_to_move.vertices[i].x));
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < vertices.size(); ++i)
      {
        vec4f vert = vertices[i];
        for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(node_to_check.convexPlanes[j], v_add(vert, vOffset)), v_zero());
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), node_to_check.convexPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else
    G_ASSERTF(0, "unsupported type to check against box");
  return false;
}

bool CollisionResource::calcOffsetForSeparation(const TMatrix &tm1, const CollisionNode &node_to_move,
  const CollisionNode &node_to_check, const Point3 &axis, Point3 &offset)
{
  offset.zero();

  G_ASSERTF(node_to_check.type == COLLISION_NODE_TYPE_CONVEX, "final node '%s' is not of convex type, only convex is supported",
    node_to_check.name.str());
  TMatrix finalTm = node_to_check.getInverseTmFlags() * tm1;
  const int numIterations = 5;
  vec4f zero = v_zero();
  vec4f vOffset = v_zero();
  vec4f axis_v = v_ldu(&axis.x);
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
    {
      Point3_vec4 point = finalTm * node_to_move.modelBBox.point(i);
      boundPoints[i] = v_ld(&point.x);
    }
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < boundPoints.size(); ++i)
      {
        for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(node_to_check.convexPlanes[j], v_add(boundPoints[i], vOffset)), zero);
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), node_to_check.convexPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_SPHERE)
  {
    BSphere3 localSph = finalTm * node_to_move.boundingSphere;
    vec4f sph = v_ldu(&localSph.c.x);
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool allInside = true;
      vec4f separationAxis = zero;
      vec4f minDist = V_C_MAX_VAL;
      for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
      {
        vec4f dist = v_sub(v_plane_dist(node_to_check.convexPlanes[j], v_add(sph, vOffset)), v_splat_w(sph));
        dist = v_mul(dist, v_abs(v_dot3(node_to_check.convexPlanes[j], axis_v)));
        allInside &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
        separationAxis = v_sel(separationAxis, node_to_check.convexPlanes[j], v_cmp_gt(minDist, v_splat_x(dist)));
        minDist = v_min(minDist, dist);
      }
      // All points lies inside
      if (!allInside)
      {
        v_stu_p3(&offset.x, vOffset);
        offset = offset - (offset * axis) * axis;
        return true;
      }
      else
        vOffset = v_add(vOffset, v_mul(minDist, separationAxis));
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    Tab<vec4f> vertices(tmpmem);
    reserve_and_resize(vertices, node_to_move.vertices.size());
    for (int i = 0; i < vertices.size(); ++i)
      vertices[i] = v_mat44_mul_vec3p(vFinalTm, v_ld(&node_to_move.vertices[i].x));
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < vertices.size(); ++i)
      {
        vec4f vert = vertices[i];
        for (int j = 0; j < node_to_check.convexPlanes.size(); ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(node_to_check.convexPlanes[j], v_add(vert, vOffset)), zero);
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), node_to_check.convexPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        offset = offset - (offset * axis) * axis;
        return true;
      }
    }
    return false;
  }
  else
    G_ASSERTF(0, "unsupported type to check against box");
  return false;
}

bool CollisionResource::testInclusion(const CollisionNode &node_to_test, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex,
  const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree, Point3 *res_pos) const
{
  TMatrix testTm;
  if (test_node_tree)
    getCollisionNodeTm(&node_to_test, tm_test, test_node_tree, testTm);
  else
    testTm = tm_test * node_to_test.tm;

  TMatrix finalTm = inverse(tm_restrain) * testTm;
  if (node_to_test.type == COLLISION_NODE_TYPE_BOX)
  {
    for (int i = 0; i < 8; ++i)
    {
      Point3_vec4 point = finalTm * node_to_test.modelBBox.point(i);
      vec4f vert = v_ld(&point.x);
      bool insideAll = true;
      for (int j = 0; j < convex.size() && insideAll; ++j)
      {
        vec4f dist = v_plane_dist_x(convex[j], vert);
        insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
      }
      if (insideAll)
      {
        if (res_pos)
          *res_pos = tm_restrain * point;
        return true;
      }
    }
    return false;
  }
  else if (node_to_test.type == COLLISION_NODE_TYPE_SPHERE)
  {
    BSphere3 localSph = finalTm * node_to_test.boundingSphere;
    vec4f sph = v_ldu(&localSph.c.x);
    bool insideAll = true;
    for (int j = 0; j < convex.size() && insideAll; ++j)
    {
      vec4f dist = v_sub(v_plane_dist_x(convex[j], sph), v_splat_w(sph));
      insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
    }
    if (insideAll && res_pos)
      *res_pos = tm_restrain * localSph.c;
    return insideAll;
  }
  else if (node_to_test.type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    for (int i = 0; i < node_to_test.vertices.size(); ++i)
    {
      bool insideAll = true;
      vec4f vert = v_mat44_mul_vec3p(vFinalTm, v_ld(&node_to_test.vertices[i].x));
      for (int j = 0; j < convex.size() && insideAll; ++j)
      {
        vec4f dist = v_plane_dist_x(convex[j], vert);
        insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
      }
      if (insideAll)
      {
        if (res_pos)
        {
          Point3_vec4 pt;
          v_st(&pt.x, vert);
          *res_pos = tm_restrain * pt;
        }
        return true;
      }
    }
    return false;
  }
  else
    G_ASSERTF(0, "unsupported type to check against convex");
  return false;
}

bool CollisionResource::testInclusion(const CollisionNode &node_to_test, const TMatrix &tm_test, const CollisionNode &restraining_node,
  const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree, const GeomNodeTree *restrain_node_tree) const
{
  G_ASSERTF(restraining_node.type == COLLISION_NODE_TYPE_CONVEX, "restrain node '%s' is not of convex type, only convex is supported",
    restraining_node.name.str());
  TMatrix restrainTm;
  if (restrain_node_tree)
    getCollisionNodeTm(&restraining_node, tm_restrain, restrain_node_tree, restrainTm);
  else
    restrainTm = tm_restrain * restraining_node.tm;

  return testInclusion(node_to_test, tm_test, restraining_node.convexPlanes, restrainTm, test_node_tree);
}

VECTORCALL bool CollisionResource::rayHit(const mat44f &tm, const Point3 &from, const Point3 &dir, float in_t, int ray_mat_id,
  int &out_mat_id, uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };
  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float /*t*/, vec3f /*normal*/, vec3f /*pos*/) {
    out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, nullptr /*geom_node_tree*/, v_ldu(&from.x),
    v_ldu(&dir.x), in_t, false /*out_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

VECTORCALL bool CollisionResource::rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *collision_node_mask, int *out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [out_mat_id](int /*trace_id*/, const CollisionNode *node, float /*t*/, vec3f /*normal*/, vec3f /*pos*/) {
    if (out_mat_id)
      *out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x),
    in_t, false /*out_normal*/, bsphere_scale, behaviorFilter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

bool CollisionResource::capsuleHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float, vec3f, vec3f) { nodes_hit.push_back(node->nodeIndex); };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::CAPSULE_HIT, is_single_ray>(tm, geom_node_tree, traces,
    false /*calc_normal*/, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*out_stats*/,
    false /*force_no_cull*/);
}

bool CollisionResource::multiRayHit(const TMatrix &instance_tm, dag::Span<CollisionTrace> traces) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int trace_id, const CollisionNode *, float, vec3f, vec3f) { traces[trace_id].isHit = true; };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.isHit = false;
  }

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, nullptr /*geom_node_tree*/, traces,
    false /*calc_normal*/, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*out_stats*/,
    false /*force_no_cull*/);
}

void CollisionResource::initializeWithGeomNodeTree(const GeomNodeTree &geom_node_tree)
{
  for (auto &node : allNodesList)
    node.geomNodeId = geom_node_tree.findNodeIndex(node.name);
}

struct ITestIntersectionAlgo
{
  virtual bool apply(const CollisionNode *node_a_head, const mat44f &tm_a, const CollisionNode *node_b_head, const mat44f &tm_b,
    float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes) = 0;
  virtual ~ITestIntersectionAlgo() = 0;

  Point3 collisionPointA;
  Point3 collisionPointB;
};

ITestIntersectionAlgo::~ITestIntersectionAlgo() {}

class TestMeshNodeMeshNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  typedef StaticSceneRayTracerT<uint16_t> tracer_t;
  TestMeshNodeMeshNodesIntersectionAlgo(vec4f a_wbsph, uint32_t a_nodes_count, vec4f b_wbsph, tracer_t *b_tracer) :
    a_wbsph(a_wbsph), a_nodes_count(a_nodes_count), b_wbsph(b_wbsph), b_tracer(b_tracer)
  {}
  ~TestMeshNodeMeshNodesIntersectionAlgo() final = default;

  bool apply(const CollisionNode *node_a_head, const mat44f &tm_a, const CollisionNode *node_b_head, const mat44f &tm_b,
    float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes)
  {
    outsideOfBounding.clear();
    reserve_and_resize(outsideOfBounding, a_nodes_count);
    aWtms.clear();
    aWtms.reserve(a_nodes_count);
    aWtms.resize(a_nodes_count);

    for (const CollisionNode *nodeA = node_a_head; nodeA; nodeA = nodeA->nextNode)
    {
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      mat44f vWtmA;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      vec4f aNodeWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x)), v_splats(nodeA->boundingSphere.r));
      outsideOfBounding.set(nodeA->nodeIndex, !isBoundingsIntersect(aNodeWbsph, v_set_x(a_max_scale), b_wbsph, v_set_x(b_max_scale)));
      aWtms[nodeA->nodeIndex] = vWtmA;
    }

    for (const CollisionNode *nodeB = node_b_head; nodeB; nodeB = nodeB->nextNode)
    {
      if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;

      mat44f vWtmB, vIWtmB;
      v_mat44_make_from_43cu_unsafe(vWtmB, nodeB->tm.array);
      v_mat44_mul43(vWtmB, tm_b, vWtmB);
      vec3f bNodeBoundingCenter = v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x));
      if (!isBoundingsIntersect(bNodeBoundingCenter, v_set_x(b_max_scale), a_wbsph, v_set_x(a_max_scale)))
        continue;
      v_mat44_inverse43(vIWtmB, vWtmB);
      vec3f sphereCenterNodeB = v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x));

      for (const CollisionNode *nodeA = node_a_head; nodeA; nodeA = nodeA->nextNode)
      {
        if (outsideOfBounding[nodeA->nodeIndex])
          continue;
        if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;

        mat44f vWtmA = aWtms[nodeA->nodeIndex];
        vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, tmAtoB, tmBtoA;
        v_mat44_inverse43(vIWtmA, vWtmA);
        v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
        v_mat44_mul43(tmBtoA, vIWtmA, vWtmB);

        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_intersect_rel_tm(bboxA, tmAtoB, bboxB, tmBtoA) == false)
          continue;

        if (isMeshNodeIntersectedWithMeshNode(nodeA, nodeB, tmAtoB))
          return true;
      }
    }

    return false;
  }

  static bool isBoundingsIntersect(vec4f a, vec4f a_max_scale, vec4f b, vec4f b_max_scale)
  {
    vec4f dist = v_length3_sq_x(v_sub(a, b));
    vec4f aRad = v_mul_x(v_splat_w(a), a_max_scale);
    vec4f bRad = v_mul_x(v_splat_w(b), b_max_scale);
    vec4f radSum = v_add_x(aRad, bRad);
    return v_test_vec_x_le(dist, v_mul_x(radSum, radSum));
  }

private:
  bool isMeshNodeIntersectedWithMeshNode(const CollisionNode *node_a, const CollisionNode *node_b, const mat44f &tm_a_to_b)
  {
    if (b_tracer)
    {
      bbox3f bbox;
      v_bbox3_init(bbox, tm_a_to_b, v_ldu_bbox3(node_a->modelBBox));
      BBox3 sbox;
      v_stu_bbox3(sbox, bbox);
      nodeBfaces.clear();
      nodeBfaces.reserve(node_b->indices.size());
      b_tracer->getFaces(nodeBfaces, sbox);
    }

    for (uint32_t nodeAIndex = 0u, countA = node_a->indices.size(); nodeAIndex < countA; nodeAIndex += 3u)
    {
      vec3f a0 = v_ldu(&node_a->vertices[node_a->indices[nodeAIndex + 0]].x);
      vec3f a1 = v_ldu(&node_a->vertices[node_a->indices[nodeAIndex + 1]].x);
      vec3f a2 = v_ldu(&node_a->vertices[node_a->indices[nodeAIndex + 2]].x);
      vec3f a0b = v_mat44_mul_vec3p(tm_a_to_b, a0);
      vec3f a1b = v_mat44_mul_vec3p(tm_a_to_b, a1);
      vec3f a2b = v_mat44_mul_vec3p(tm_a_to_b, a2);

      if (!b_tracer)
      {
        for (uint32_t nodeBIndex = 0u, countB = node_b->indices.size(); nodeBIndex < countB; nodeBIndex += 3u)
        {
          vec3f b0 = v_ldu(&node_b->vertices[node_b->indices[nodeBIndex + 0]].x);
          vec3f b1 = v_ldu(&node_b->vertices[node_b->indices[nodeBIndex + 1]].x);
          vec3f b2 = v_ldu(&node_b->vertices[node_b->indices[nodeBIndex + 2]].x);

          if (v_test_triangle_triangle_intersection(a0b, a1b, a2b, b0, b1, b2))
          {
            mat44f tmA, tmB;
            v_mat44_make_from_43cu_unsafe(tmA, node_a->tm.array);
            v_mat44_make_from_43cu_unsafe(tmB, node_b->tm.array);
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
            vec3f bc = v_mul(v_add(b0, v_add(b1, b2)), v_splats(1 / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(tmA, ac));
            v_stu_p3(&collisionPointB.x, v_mat44_mul_vec3p(tmB, bc));
            return true;
          }
        }
      }
      else
      {
        for (int nodeBFace : nodeBfaces)
        {
          const auto &face = b_tracer->faces(nodeBFace);
          vec3f b0 = v_ldu(&b_tracer->verts(face.v[0]).x);
          vec3f b1 = v_ldu(&b_tracer->verts(face.v[1]).x);
          vec3f b2 = v_ldu(&b_tracer->verts(face.v[2]).x);

          if (v_test_triangle_triangle_intersection(a0b, a1b, a2b, b0, b1, b2))
          {
            mat44f tmA, tmB;
            v_mat44_make_from_43cu_unsafe(tmA, node_a->tm.array);
            v_mat44_make_from_43cu_unsafe(tmB, node_b->tm.array);
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
            vec3f bc = v_mul(v_add(b0, v_add(b1, b2)), v_splats(1 / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(tmA, ac));
            v_stu_p3(&collisionPointB.x, v_mat44_mul_vec3p(tmB, bc));
            return true;
          }
        }
      }
    }

    return false;
  }

  vec4f a_wbsph; // pos|r
  vec4f b_wbsph; // pos|r
  tracer_t *b_tracer;
  uint32_t a_nodes_count;
  eastl::bitvector<framemem_allocator> outsideOfBounding;
  dag::RelocatableFixedVector<mat44f, 40> aWtms;
  Tab<int> nodeBfaces;
};


class TestMeshNodeBoxNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeBoxNodesIntersectionAlgo() = default;
  ~TestMeshNodeBoxNodesIntersectionAlgo() final = default;

  bool apply(const CollisionNode *node_a_head, const mat44f &tm_a, const CollisionNode *node_b_head, const mat44f &tm_b,
    float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (const CollisionNode *nodeA = node_a_head; nodeA; nodeA = nodeA->nextNode)
    {
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, vIWtmA, tmAtoB, tmBtoA;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
      v_mat44_inverse43(vIWtmA, vWtmA);
      v_mat44_mul43(tmBtoA, vIWtmA, tm_b);
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));

      for (const CollisionNode *nodeB = node_b_head; nodeB; nodeB = nodeB->nextNode)
      {
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_intersect_rel_tm(bboxA, tmAtoB, bboxB, tmBtoA) == false)
          continue;

        for (size_t nodeAIndex = 0u, countA = nodeA->indices.size(); nodeAIndex < countA; nodeAIndex += 3u)
        {
          Point3_vec4 a0b, a1b, a2b;
          vec3f a0 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 0]].x);
          vec3f a1 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 1]].x);
          vec3f a2 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 2]].x);
          v_st(&a0b.x, v_mat44_mul_vec3p(tmAtoB, a0));
          v_st(&a1b.x, v_mat44_mul_vec3p(tmAtoB, a1));
          v_st(&a2b.x, v_mat44_mul_vec3p(tmAtoB, a2));

          if (test_triangle_box_intersection(a0b, a1b, a2b, nodeB->modelBBox))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            collisionPointB = nodeB->boundingSphere.c;
            return true;
          }
        }
      }
    }

    return false;
  }
};

class TestBoxNodeBoxNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestBoxNodeBoxNodesIntersectionAlgo() = default;
  ~TestBoxNodeBoxNodesIntersectionAlgo() final = default;

  bool apply(const CollisionNode *node_a_head, const mat44f &tm_a, const CollisionNode *node_b_head, const mat44f &tm_b,
    float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, vIWtmB, tmBToA, tmAToB;
    v_mat44_inverse43(vIWtmA, tm_a);
    v_mat44_inverse43(vIWtmB, tm_b);
    v_mat44_mul43(tmBToA, vIWtmA, tm_b);
    v_mat44_mul43(tmAToB, vIWtmB, tm_a);

    for (const CollisionNode *nodeA = node_a_head; nodeA; nodeA = nodeA->nextNode)
    {
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(tm_a, v_ldu(&nodeA->boundingSphere.c.x));
      for (const CollisionNode *nodeB = node_b_head; nodeB; nodeB = nodeB->nextNode)
      {
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        if (v_bbox3_test_trasformed_box_intersect_rel_tm(v_ldu_bbox3(nodeA->modelBBox), tmAToB, v_ldu_bbox3(nodeB->modelBBox), tmBToA))
        {
          collisionPointA = nodeA->boundingSphere.c;
          collisionPointB = nodeB->boundingSphere.c;
          return true;
        }
      }
    }

    return false;
  }
};

class TestMeshNodeSphereNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeSphereNodesIntersectionAlgo() = default;
  ~TestMeshNodeSphereNodesIntersectionAlgo() final = default;

  bool apply(const CollisionNode *node_a_head, const mat44f &tm_a, const CollisionNode *node_b_head, const mat44f &tm_b,
    float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (const CollisionNode *nodeA = node_a_head; nodeA; nodeA = nodeA->nextNode)
    {
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAToB;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      v_mat44_mul43(tmAToB, vIWtmB, vWtmA);

      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
      for (const CollisionNode *nodeB = node_b_head; nodeB; nodeB = nodeB->nextNode)
      {
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        for (size_t nodeAIndex = 0u, countA = nodeA->indices.size(); nodeAIndex < countA; nodeAIndex += 3u)
        {
          vec3f a0 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 0]].x);
          vec3f a1 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 1]].x);
          vec3f a2 = v_ldu(&nodeA->vertices[nodeA->indices[nodeAIndex + 2]].x);
          vec3f a0b = v_mat44_mul_vec3p(tmAToB, a0);
          vec3f a1b = v_mat44_mul_vec3p(tmAToB, a1);
          vec3f a2b = v_mat44_mul_vec3p(tmAToB, a2);

          if (v_test_triangle_sphere_intersection(a0b, a1b, a2b, v_ldu(&nodeB->boundingSphere.c.x), v_set_x(nodeB->boundingSphere.r2)))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1.f / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            collisionPointB = nodeB->boundingSphere.c;
            return true;
          }
        }
      }
    }

    return false;
  }
};

bool CollisionResource::testIntersection(const CollisionResource *res_a, const TMatrix &tm_a, const CollisionResource *res_b,
  const TMatrix &tm_b, Point3 &collisionPointA, Point3 &collisionPointB, bool checkOnlyPhysNodes, /* = false,*/
  bool useTraceFaces /* = false*/)
{
  G_ASSERT(res_a);
  G_ASSERT(res_b);

  alignas(EA_CACHE_LINE_SIZE) mat44f vTmA, vTmB;
  v_mat44_make_from_43cu_unsafe(vTmA, tm_a.array);
  v_mat44_make_from_43cu_unsafe(vTmB, tm_b.array);
  float maxScaleA = v_extract_x(v_mat44_max_scale43_x(vTmA));
  float maxScaleB = v_extract_x(v_mat44_max_scale43_x(vTmB));

  // world xyz|r
  vec4f aWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vTmA, res_a->vBoundingSphere), v_splats(res_a->boundingSphereRad));
  vec4f bWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vTmB, res_b->vBoundingSphere), v_splats(res_b->boundingSphereRad));
  if (!TestMeshNodeMeshNodesIntersectionAlgo::isBoundingsIntersect(aWbsph, v_set_x(maxScaleA), bWbsph, v_set_x(maxScaleB)))
    return false;

  TestMeshNodeMeshNodesIntersectionAlgo testMeshNodeMeshNodesIntersectionAlgo(aWbsph, res_a->allNodesList.size(), bWbsph,
    useTraceFaces ? res_b->gridForTraceable.tracer.get() : nullptr);
  TestMeshNodeBoxNodesIntersectionAlgo testMeshNodeBoxNodesIntersectionAlgo;
  TestMeshNodeSphereNodesIntersectionAlgo testMeshNodeSphereNodesIntersectionAlgo;
  TestBoxNodeBoxNodesIntersectionAlgo testBoxNodeBoxNodesIntersectionAlgo;

  ITestIntersectionAlgo *arrIntersectionCall[COLLISION_NODE_TYPE_CAPSULE][COLLISION_NODE_TYPE_CAPSULE] = {};
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_MESH] = &testMeshNodeMeshNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_BOX] = &testMeshNodeBoxNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_SPHERE] = &testMeshNodeSphereNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_BOX][COLLISION_NODE_TYPE_BOX] = &testBoxNodeBoxNodesIntersectionAlgo;

  for (int nodeTypeIxA = COLLISION_NODE_TYPE_MESH; nodeTypeIxA < COLLISION_NODE_TYPE_CAPSULE; nodeTypeIxA++)
  {
    if (res_a->nodeLists[nodeTypeIxA] == nullptr)
      continue;

    for (int nodeTypeIxB = COLLISION_NODE_TYPE_MESH; nodeTypeIxB < COLLISION_NODE_TYPE_CAPSULE; nodeTypeIxB++)
    {
      if (res_b->nodeLists[nodeTypeIxB] == nullptr)
        continue;

      bool result = false;
      if (ITestIntersectionAlgo *testAB = arrIntersectionCall[nodeTypeIxA][nodeTypeIxB])
      {
        result = testAB->apply(res_a->nodeLists[nodeTypeIxA], vTmA, res_b->nodeLists[nodeTypeIxB], vTmB, maxScaleA, maxScaleB,
          checkOnlyPhysNodes);

        collisionPointA = testAB->collisionPointA;
        collisionPointB = testAB->collisionPointB;
      }
      else if (ITestIntersectionAlgo *testBA = arrIntersectionCall[nodeTypeIxB][nodeTypeIxA])
      {
        result = testBA->apply(res_b->nodeLists[nodeTypeIxB], vTmB, res_a->nodeLists[nodeTypeIxA], vTmA, maxScaleB, maxScaleA,
          checkOnlyPhysNodes);

        collisionPointB = testBA->collisionPointA;
        collisionPointA = testBA->collisionPointB;
      }

      if (result)
        return true;
    }
  }

  return false;
}

bool CollisionResource::testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
  const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2, Point3 &collisionPoint1,
  Point3 &collisionPoint2, uint16_t *nodeIndex1, uint16_t *nodeIndex2, Tab<uint16_t> *node_indices1)
{
  G_ASSERT(res1);
  G_ASSERT(res2);

  float scale1 = tm1.getcol(1).length();
  float scale2 = tm2.getcol(1).length();

  TMatrix modelTm2to1 = inverse(tm1) * tm2;

  Tab<bool> boxOutside(framemem_ptr());
  reserve_and_resize(boxOutside, res2->allNodesList.size());
  Tab<int> faces(framemem_ptr());

#if DAGOR_DBGLEVEL > 0
  unsigned int numNodesDebug = 0;
  unsigned int numSpheresDebug = 0;
  unsigned int numBoxesDebug = 0;
  unsigned int numTrianglesDebug = 0;
  unsigned int numBoxesInsideDebug = 0;
#define _INC(x) ++(x)
#else
#define _INC(x)
#endif

  for (const CollisionNode *node1 = res1->meshNodesHead; node1; node1 = node1->nextNode)
  {
    if (filter1 && !filter1(node1->nodeIndex))
      continue;

    Point3 sphereCenter1 = tm1 * (node1->tm * node1->boundingSphere.c);
    TMatrix invTm1, tm1ToWorld;
    bool invTm1ready = false;
    mem_set_0(boxOutside);

    for (const CollisionNode *node2 = res2->meshNodesHead; node2; node2 = node2->nextNode)
    {
      _INC(numNodesDebug);

      if (filter2 && !filter2(node2->nodeIndex))
        continue;

      if (node2->insideOfNode != 0xffff && boxOutside[node2->insideOfNode])
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }

      _INC(numBoxesInsideDebug);

      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);
      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) < sumRad * sumRad)
      {
        _INC(numSpheresDebug);

        if (!invTm1ready)
        {
          tm1ToWorld = tm1 * node1->tm;
          invTm1 = inverse(tm1ToWorld);
          invTm1ready = true;
        }

        TMatrix tm2to1 = invTm1 * (tm2 * node2->tm);

        if (test_box_box_intersection(node1->modelBBox, node2->modelBBox, tm2to1))
        {
          _INC(numBoxesDebug);

          if (!res2->gridForTraceable.tracer)
          {
            for (size_t index1 = 0; index1 < node1->indices.size(); index1 += 3)
            {
              Point3 v1_0 = node1->vertices[node1->indices[index1 + 0]];
              Point3 v1_1 = node1->vertices[node1->indices[index1 + 1]];
              Point3 v1_2 = node1->vertices[node1->indices[index1 + 2]];
              for (size_t index2 = 0; index2 < node2->indices.size(); index2 += 3)
              {
                _INC(numTrianglesDebug);

                const Point3 &v2_0 = node2->vertices[node2->indices[index2 + 0]];
                const Point3 &v2_1 = node2->vertices[node2->indices[index2 + 1]];
                const Point3 &v2_2 = node2->vertices[node2->indices[index2 + 2]];
                bool isIntersected =
                  test_triangle_triangle_intersection_mueller(v1_0, v1_1, v1_2, tm2to1 * v2_0, tm2to1 * v2_1, tm2to1 * v2_2);

                if (isIntersected)
                {
                  collisionPoint1 = node1->tm * ((v1_0 + v1_1 + v1_2) * 0.333333f);
                  collisionPoint2 = node2->tm * ((v2_0 + v2_1 + v2_2) * 0.333333f);
                  if (nodeIndex1)
                    *nodeIndex1 = node1->nodeIndex;
                  if (nodeIndex2)
                    *nodeIndex2 = node2->nodeIndex;

                  if (!node_indices1)
                    return true;
                  else
                  {
                    node_indices1->push_back(node1->nodeIndex);
                    break;
                  }
                }
              }
            }
          }
          else
          {
            TMatrix tm1to2 = inverse(tm2 * node2->tm) * tm1ToWorld;
            faces.clear();
            if (res2->gridForTraceable.tracer->getFaces(faces, tm1to2 * node1->modelBBox))
            {
              for (size_t index1 = 0; index1 < node1->indices.size(); index1 += 3)
              {
                Point3 v1_0 = node1->vertices[node1->indices[index1 + 0]];
                Point3 v1_1 = node1->vertices[node1->indices[index1 + 1]];
                Point3 v1_2 = node1->vertices[node1->indices[index1 + 2]];
                for (size_t index2 = 0; index2 < faces.size(); index2++)
                {
                  _INC(numTrianglesDebug);

                  const auto &face = res2->gridForTraceable.tracer->faces(faces[index2]);
                  const Point3 &v2_0 = res2->gridForTraceable.tracer->verts(face.v[0]);
                  const Point3 &v2_1 = res2->gridForTraceable.tracer->verts(face.v[1]);
                  const Point3 &v2_2 = res2->gridForTraceable.tracer->verts(face.v[2]);

                  bool isIntersected =
                    test_triangle_triangle_intersection_mueller(v1_0, v1_1, v1_2, tm2to1 * v2_0, tm2to1 * v2_1, tm2to1 * v2_2);

                  if (isIntersected)
                  {
                    collisionPoint1 = node1->tm * ((v1_0 + v1_1 + v1_2) * 0.333333f);
                    collisionPoint2 = node2->tm * ((v2_0 + v2_1 + v2_2) * 0.333333f);
                    if (nodeIndex1)
                      *nodeIndex1 = node1->nodeIndex;
                    if (nodeIndex2)
                      *nodeIndex2 = node2->nodeIndex;

                    if (!node_indices1)
                      return true;
                    else
                    {
                      node_indices1->push_back(node1->nodeIndex);
                      break;
                    }
                  }
                }
              }
            }
          }
        }
        else
        {
          boxOutside[node2->nodeIndex] = true;
        }
      }
    }

    TMatrix tm1to2;
    bool tm1to2ready = false;

    for (const CollisionNode *node2 = res2->boxNodesHead; node2; node2 = node2->nextNode)
    {
      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);

      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) < sumRad * sumRad)
      {
        if (test_box_box_intersection(node1->modelBBox, node2->modelBBox, modelTm2to1))
        {
          if (!tm1to2ready)
          {
            tm1to2 = inverse(tm2) * tm1 * node1->tm;
            tm1to2ready = true;
          }

          for (size_t index1 = 0; index1 < node1->indices.size(); index1 += 3)
          {
            bool isIntersected = test_triangle_box_intersection(tm1to2 * node1->vertices[node1->indices[index1]],
              tm1to2 * node1->vertices[node1->indices[index1 + 1]], tm1to2 * node1->vertices[node1->indices[index1 + 2]],
              node2->modelBBox);

            if (isIntersected)
            {
              collisionPoint1 = node1->tm *
                                (node1->vertices[node1->indices[index1]] + node1->vertices[node1->indices[index1 + 1]] +
                                  node1->vertices[node1->indices[index1 + 2]]) *
                                0.333333f;

              collisionPoint2 = node2->boundingSphere.c;

              if (nodeIndex1)
                *nodeIndex1 = node1->nodeIndex;
              if (nodeIndex2)
                *nodeIndex2 = node2->nodeIndex;

              if (!node_indices1)
                return true;
              else
              {
                node_indices1->push_back(node1->nodeIndex);
                break;
              }
            }
          }
        }
      }
    }

    for (const CollisionNode *node2 = res2->sphereNodesHead; node2; node2 = node2->nextNode)
    {
      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);

      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) < sqr(sumRad))
      {
        if (!tm1to2ready)
        {
          tm1to2 = inverse(tm2) * tm1 * node1->tm;
          tm1to2ready = true;
        }

        for (size_t index1 = 0; index1 < node1->indices.size(); index1 += 3)
        {
          bool isIntersected = test_triangle_sphere_intersection(tm1to2 * node1->vertices[node1->indices[index1]],
            tm1to2 * node1->vertices[node1->indices[index1 + 1]], tm1to2 * node1->vertices[node1->indices[index1 + 2]],
            node2->boundingSphere);

          if (isIntersected)
          {
            collisionPoint1 = node1->tm *
                              (node1->vertices[node1->indices[index1]] + node1->vertices[node1->indices[index1 + 1]] +
                                node1->vertices[node1->indices[index1 + 2]]) *
                              0.333333f;

            collisionPoint2 = node2->boundingSphere.c;

            if (nodeIndex1)
              *nodeIndex1 = node1->nodeIndex;
            if (nodeIndex2)
              *nodeIndex2 = node2->nodeIndex;

            if (!node_indices1)
              return true;
            else
            {
              node_indices1->push_back(node1->nodeIndex);
              break;
            }
          }
        }
      }
    }
  }

#undef _INC

  return node_indices1 ? !node_indices1->empty() : false;
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  TMatrix &out_tm) const
{
  mat44f tm, outTm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  getCollisionNodeTm(node, tm, geom_node_tree, outTm);
  v_mat_43cu_from_mat44(out_tm.array, outTm);
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm, const GeomNodeTree *geom_node_tree,
  mat44f &out_tm) const
{
  if (node->type == COLLISION_NODE_TYPE_MESH || node->type == COLLISION_NODE_TYPE_CONVEX || node->type == COLLISION_NODE_TYPE_CAPSULE)
    out_tm = getMeshNodeTmInline(node, instance_tm, v_zero(), geom_node_tree);
  else
    out_tm = instance_tm;
}

__forceinline mat44f CollisionResource::getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
  const GeomNodeTree *geom_node_tree) const
{
  mat44f outTm;
  if (auto idx = (geom_node_tree && node->geomNodeId.index() < geom_node_tree->nodeCount()) ? node->geomNodeId : dag::Index16())
  {
    outTm = geom_node_tree->getNodeWtmRel(idx); //-V1004
    outTm.col3 = v_add(outTm.col3, v_sub(geom_node_tree->getWtmOfs(), instance_woffset));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    {
      mat44f relGeomNodeTm;
      v_mat44_make_from_43cu_unsafe(relGeomNodeTm, relGeomNodeTms[node->nodeIndex].array);
      v_mat44_mul43(outTm, outTm, relGeomNodeTm);
    }
  }
  else
  {
    mat44f nodeTm;
    v_mat44_make_from_43cu_unsafe(nodeTm, node->tm.m[0]);
    v_mat44_mul43(outTm, instance_tm, nodeTm);
  }
  return outTm;
}

void CollisionResource::collapseAndOptimize(bool need_frt, bool frt_build_fast)
{
  if (collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
  {
    // debug("skip already optimized %p", this);
    return;
  }
  collisionFlags |= COLLISION_RES_FLAG_OPTIMIZED;
  if (!meshNodesHead) // || !meshNodesHead->nextNode)
  {
    // debug("CollisionResource: nothing to optimize %p", this);
    return;
  }

  // debug("CollisionResource: optimizing %p", this);
  {
    for (CollisionNode *m = const_cast<CollisionNode *>(meshNodesHead); m; m = m->nextNode)
    {
      if (m->type != COLLISION_NODE_TYPE_CONVEX && m->type != COLLISION_NODE_TYPE_MESH)
        continue;
      if ((m->flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT || !m->vertices.size())
        continue;
      mat44f nodeTm;
      bbox3f box;
      v_mat44_make_from_43cu(nodeTm, m->tm[0]);
      v_bbox3_init_empty(box);
      for (vec4f *__restrict verts = (vec4f *)(void *)m->vertices.data(), *ve = verts + m->vertices.size(); verts != ve; ++verts)
      {
        *verts = v_mat44_mul_vec3p(nodeTm, *verts);
        v_bbox3_add_pt(box, *verts);
      }
      vec4f vSphereC = v_bbox3_center(box), sphereRad2 = v_zero();
      for (vec4f *__restrict verts = (vec4f *)(void *)m->vertices.data(), *ve = verts + m->vertices.size(); verts != ve; ++verts)
        sphereRad2 = v_max(sphereRad2, v_length3_sq_x(v_sub(vSphereC, *verts)));

      mat44f N, TN;
      v_mat44_inverse(N, nodeTm);
      v_mat44_transpose(TN, N);
      for (plane3f *__restrict planes = m->convexPlanes.data(), *pe = planes + m->convexPlanes.size(); planes != pe; ++planes)
        *planes = v_mat44_mul_vec4(TN, *planes);

      v_stu_bbox3(m->modelBBox, box);
      v_stu_p3(&m->boundingSphere.c.x, vSphereC);
      m->boundingSphere.r = v_extract_x(v_sqrt_x(sphereRad2));
      m->boundingSphere.r2 = v_extract_x(sphereRad2);

      if (m->tm.det() < 0.f) // swap indices order
        for (int i = 0; i < m->indices.size(); i += 3)
          eastl::swap(m->indices[i + 0], m->indices[i + 2]);
      m->tm.identity();
      m->flags = CollisionNode::IDENT | (m->flags & (~CollisionNode::TRANSLATE));
      m->flags = CollisionNode::ORTHONORMALIZED | (m->flags & (~CollisionNode::ORTHOUNIFORM));
      m->cachedMaxTmScale = 1.f;
    }
  }

  IMemAlloc *framemem = framemem_ptr();
  Tab<Tab<CollisionNode *>> meshNodesByMat(framemem);
  Tab<eastl::pair<PhysMat::MatID, bool>> matIndices(framemem);

  for (CollisionNode *m = const_cast<CollisionNode *>(meshNodesHead); m; m = m->nextNode)
  {
    const bool isTraceable = m->checkBehaviorFlags(CollisionNode::TRACEABLE);
    const bool isPhysCollidable = m->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
    if (m->type == COLLISION_NODE_TYPE_MESH && isTraceable)
    {
      const eastl::pair<PhysMat::MatID, bool> matIndicesValue = eastl::make_pair(m->physMatId, isPhysCollidable);
      intptr_t bucket = find_value_idx(matIndices, matIndicesValue);
      if (bucket == -1)
      {
        Tab<CollisionNode *> nodes(framemem);
        bucket = meshNodesByMat.size();
        meshNodesByMat.push_back(nodes);
        matIndices.push_back(matIndicesValue);
      }
      meshNodesByMat[size_t(bucket)].push_back(m);
    }
  }

  for (const Tab<CollisionNode *> &nodes : meshNodesByMat)
  {
    CollisionNode *targetNode = nodes[0];
    int v_num = 0, i_num = 0;
    for (const CollisionNode *node : nodes)
    {
      v_num += node->vertices.size();
      i_num += node->indices.size();
    }
    if (v_num > 65530)
    {
      debug("CollisionResource: too many faces %d, cannot optimize %p", v_num, this);
      continue;
    }
    if (v_num == 0)
      continue;

    Point3_vec4 *vertices = memalloc_typed<Point3_vec4>(v_num, midmem);
    uint16_t *indices = memalloc_typed<uint16_t>(i_num, midmem);
    BBox3 bbox;

    v_num = 0;
    i_num = 0;
    for (CollisionNode *m : nodes)
    {
      for (int i = 0; i < m->vertices.size(); i++)
        bbox += (vertices[v_num + i] = m->vertices[i]);
      for (int i = 0; i < m->indices.size(); i += 3)
      {
        indices[i_num + i + 0] = m->indices[i + 0] + v_num;
        indices[i_num + i + 1] = m->indices[i + 1] + v_num;
        indices[i_num + i + 2] = m->indices[i + 2] + v_num;
      }
      v_num += m->vertices.size();
      i_num += m->indices.size();

      m->resetVertices();
      m->resetIndices();
      m->flags &= ~(m->FLAG_VERTICES_ARE_REFS | m->FLAG_INDICES_ARE_REFS);
    }

    targetNode->modelBBox = bbox;
    targetNode->boundingSphere.c = bbox.center();
    targetNode->boundingSphere.r2 = lengthSq(vertices[0] - targetNode->boundingSphere.c);
    for (int i = 1; i < v_num; i++)
      inplace_max(targetNode->boundingSphere.r2, lengthSq(vertices[i] - targetNode->boundingSphere.c));
    targetNode->boundingSphere.r = sqrtf(targetNode->boundingSphere.r2);

    targetNode->tm.identity();
    targetNode->flags |= targetNode->IDENT;
    targetNode->flags &= ~targetNode->TRANSLATE;
    targetNode->cachedMaxTmScale = 1.f;

    targetNode->resetVertices({vertices, v_num});
    targetNode->resetIndices({indices, i_num});
    targetNode->flags &= ~(targetNode->FLAG_VERTICES_ARE_REFS | targetNode->FLAG_INDICES_ARE_REFS);
  }

  dag::RelocatableFixedVector<int, 256> nodesToRemove;
  nodesToRemove.reserve(allNodesList.size());
  Tab<CollisionNode> newAllNodes(tmpmem);
  newAllNodes.reserve(allNodesList.size());
  for (const Tab<CollisionNode *> &nodes : meshNodesByMat)
    for (int i = 1; i < nodes.size(); ++i)
      nodesToRemove.push_back(nodes[i]->nodeIndex);
  for (CollisionNode &node : allNodesList)
  {
    if (find_value_idx(nodesToRemove, node.nodeIndex) == -1)
    {
      node.insideOfNode = 0xffff;
      newAllNodes.push_back(node);
    }
  }
  newAllNodes.shrink_to_fit();
  allNodesList = eastl::move(newAllNodes);
  sortNodesList();
  rebuildNodesLL();

  gridForTraceable.reset();
  gridForCollidable.reset();

  if (need_frt)
  {
    bool coll_and_trace_frt_equals = true;
    for (const CollisionNode *node = meshNodesHead; node; node = node->nextNode)
      if (node->checkBehaviorFlags(CollisionNode::TRACEABLE) != node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      {
        coll_and_trace_frt_equals = false;
        break;
      }

    gridForTraceable.buildFRT(this, CollisionNode::TRACEABLE, frt_build_fast);
    if (coll_and_trace_frt_equals)
      collisionFlags |= COLLISION_RES_FLAG_REUSE_TRACE_FRT;
    else
      gridForCollidable.buildFRT(this, CollisionNode::PHYS_COLLIDABLE, frt_build_fast);
    if (gridForTraceable.tracer.get())
      collisionFlags |= COLLISION_RES_FLAG_HAS_TRACE_FRT;
    if (gridForCollidable.tracer.get())
      collisionFlags |= COLLISION_RES_FLAG_HAS_COLL_FRT;
  }
}

void CollisionResource::clipCapsule(const TMatrix &instance_tm, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized)
{
  TMatrix itm = inverse(instance_tm);

  ::Capsule nc = c;
  // probably we need to apply itm for capsule radius
  nc.transform(itm);

  real nmd = MAX_REAL;
  Point3 nlpt, nwpt;
  // probably we need to apply itm for movedirNormalized
  this->clipCapsule(nc, nlpt, nwpt, nmd, movedirNormalized);

  if (nmd < -0.0001)
  {
    nlpt = instance_tm * nlpt;
    nwpt = instance_tm * nwpt;

    nmd = -(nlpt - nwpt).length();
    if (nmd < md)
    {
      md = nmd;
      cp1 = nlpt;
      cp2 = nwpt;
    }
  }
}

void CollisionResource::clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized)
{
  if (auto *tracer = ((collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT) ? gridForTraceable : gridForCollidable).tracer.get())
    tracer->clipCapsule(c, cp1, cp2, md, movedirNormalized);
}

bool CollisionResource::test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth)
{
  TMatrix itm = inverse(node->tm);

  BSphere3 localSphere(itm * sphere.c, sphere.r);
  if (!(node->modelBBox & localSphere))
    return false;

  const uint16_t *__restrict cur = node->indices.data();
  PREFETCH_DATA(0, cur);
  const uint16_t *__restrict end = cur + node->indices.size();
  const Point3_vec4 *__restrict vertices = node->vertices.data();
  const Point3 localDirNorm = itm % dir_norm;

  PREFETCH_DATA(128, cur);
  while (cur < end)
  {
    const Point3_vec4 &corner0 = vertices[*(cur++)];
    const Point3_vec4 &corner1 = vertices[*(cur++)];
    const Point3_vec4 &corner2 = vertices[*(cur++)];

    if (test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere))
    {
      const Point3 cross = (corner1 - corner0) % (corner2 - corner0);

      if (localDirNorm * cross < -VERY_SMALL_NUMBER)
        continue;

      const float crossLen = cross.length();
      out_norm = cross * safeinv(crossLen);
      out_depth = -out_norm * (corner0 - localSphere.c) - sphere.r;
      return true;
    }
  }
  return false;
}

bool CollisionResource::test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node)
{
  TMatrix itm = inverse(node->tm);
  Point3 localCylinderPoint0 = itm * p0;
  Point3 localCylinderPoint1 = itm * p1;

  const Point3 radiusVec(radius, radius, radius);
  BBox3 bbox;
  bbox += localCylinderPoint0 - radiusVec;
  bbox += localCylinderPoint0 + radiusVec;
  bbox += localCylinderPoint1 - radiusVec;
  bbox += localCylinderPoint1 + radiusVec;
  if (!(node->modelBBox & bbox))
    return false;

  BSphere3 localSphere0(localCylinderPoint0, radius);
  BSphere3 localSphere1(localCylinderPoint1, radius);

  const uint16_t *__restrict cur = node->indices.data();
  PREFETCH_DATA(0, cur);
  const uint16_t *__restrict end = cur + node->indices.size();
  const Point3_vec4 *__restrict vertices = node->vertices.data();

  PREFETCH_DATA(128, cur);
  while (cur < end)
  {
    const Point3_vec4 &corner0 = vertices[*(cur++)];
    const Point3_vec4 &corner1 = vertices[*(cur++)];
    const Point3_vec4 &corner2 = vertices[*(cur++)];

    if (test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere0) ||
        test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere1) ||
        test_triangle_cylinder_intersection(corner0, corner1, corner2, localCylinderPoint0, localCylinderPoint1, radius))
      return true;
  }

  return false;
}

CollisionResource::Grid::~Grid() = default;
CollisionResource::Grid::Grid() = default;
void CollisionResource::Grid::reset() { tracer.reset(); }

void CollisionResource::Grid::buildFRT(CollisionResource *parent, uint8_t behavior_flag, bool frt_build_fast)
{
  reset();

  if (!USE_TRACE_GRID || !parent->meshNodesHead || !(parent->meshNodesHead->flags & CollisionNode::IDENT))
    return;

  bbox3f fullMeshBox;
  v_bbox3_init_empty(fullMeshBox);
  int totalIndices = 0;
  for (const CollisionNode *node = parent->meshNodesHead; node; node = node->nextNode)
  {
    if (!node->checkBehaviorFlags(behavior_flag))
      continue;
    if (node->checkBehaviorFlags(CollisionNode::SOLID))
      return; // node requires trace without culling
    totalIndices += node->indices.size();
    v_bbox3_add_box(fullMeshBox, v_ldu_bbox3(node->modelBBox)); // tm is ident
  }

  const float minLeafSize = 0.25f; // there is no much sense in leaf size less than 0.25 cm. Even for raytracing it is too detailed
  const int blockSize = 28;        // it's not precise resulting number, but gives good results
  vec3f size = v_bbox3_size(fullMeshBox);
  vec3f checkMin = v_cmp_gt(size, v_splats(minLeafSize));
  int minMask = v_signmask(checkMin);
  size = v_sel(V_C_ONE, size, checkMin);
  float vol = v_extract_x(v_hmul3(size));
  float facesCount = totalIndices / 3.f;
  float s = minMask != 0 ? (powf(vol / (facesCount / blockSize), 1.f / __popcount(minMask)) + 0.001f) // get best leaf size
                         : v_extract_x(v_hmax3(size)); // very small object, use biggest side
  vec3f vs = v_splats(max(s, minLeafSize));
  float xyzSum = v_extract_x(v_hadd3_x(v_div(size, vs)));
  if (facesCount >= MIN_FACES_TO_CREATE_GRID && xyzSum > MIN_WIDTH_TO_CREATE_GRID) // ignore small objects
  {
    // debug("building grid v=%d i=%d", meshNodesHead->vertices.size(), intIndices.size());
    Point3_vec4 leafSize;
    v_st(&leafSize.x, vs);
    auto *localGrid = new BuildableStaticSceneRayTracerT<uint16_t>(leafSize, 3);

    uint32_t maxIndices = 0, totalIndices = 0, totalVertices = 0;
    for (const CollisionNode *node = parent->meshNodesHead; node; node = node->nextNode)
    {
      if (!node->checkBehaviorFlags(behavior_flag)) // we do not add non-traceable node to grid
        continue;
      maxIndices = max(maxIndices, node->indices.size());
      totalIndices += node->indices.size();
      totalVertices += node->vertices.size();
    }
    localGrid->reserve(totalIndices, totalVertices);

    for (const CollisionNode *node = parent->meshNodesHead; node; node = node->nextNode)
    {
      if (!node->checkBehaviorFlags(behavior_flag)) // we do not add non-traceable node to grid
        continue;

      uint32_t v_ofs = localGrid->dump.vertsCount, f_ofs = localGrid->dump.facesCount;
      localGrid->addmesh((uint8_t *)node->vertices.data(), sizeof(vec4f), node->vertices.size(), //
        node->indices.data(), sizeof(uint16_t) * 3, node->indices.size() / 3, nullptr, false);

      if (!(node->flags & node->FLAG_VERTICES_ARE_REFS))
      {
        const_cast<CollisionNode *>(node)->resetVertices({localGrid->dump.vertsPtr + v_ofs, (int)node->vertices.size()});
        const_cast<CollisionNode *>(node)->flags |= node->FLAG_VERTICES_ARE_REFS;
      }
      if (!(node->flags & node->FLAG_INDICES_ARE_REFS) && f_ofs == 0 /* when f_ofs > 0 indices are biased by v_ofs */)
      {
        const_cast<CollisionNode *>(node)->resetIndices({&localGrid->dump.facesPtr[f_ofs].v[0], (int)node->indices.size()});
        const_cast<CollisionNode *>(node)->flags |= node->FLAG_INDICES_ARE_REFS;
      }
    }
    localGrid->setCullFlags(StaticSceneRayTracer::CULL_BOTH);
    localGrid->rebuild(frt_build_fast);
    tracer.reset(localGrid);
  }
}

vec4f CollisionResource::getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const
{
  if (geom_node_tree && bsphereCenterNode)
    return geom_node_tree->getNodeWpos(bsphereCenterNode);
  else
    return v_mat44_mul_vec3p(tm, vBoundingSphere);
}

Point3 CollisionResource::getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const
{
  mat44f vTm;
  v_mat44_make_from_43cu_unsafe(vTm, tm.array);
  Point3 ret;
  v_stu_p3(&ret.x, getWorldBoundingSphere(vTm, geom_node_tree));
  return ret;
}

CollisionResource::TraceMeshNodeLocalApi CollisionResource::traceMeshNodeLocalApi = {
  26 * 3, // check bounding for each 4 if triangles >= 26
  {&CollisionResource::traceRayMeshNodeLocalCullCCW<false>, &CollisionResource::rayHitMeshNodeLocalCullCCW<false>,
    &CollisionResource::traceRayMeshNodeLocalAllHits<false>},
  {&CollisionResource::traceRayMeshNodeLocalCullCCW<true>, &CollisionResource::rayHitMeshNodeLocalCullCCW<true>,
    &CollisionResource::traceRayMeshNodeLocalAllHits<true>},
};

void CollisionResource::check_avx_mesh_api_support()
{
  if (cpu_feature_fast_256bit_avx_checked && haveTraceMeshNodeLocalApi_AVX256)
    traceMeshNodeLocalApi = traceMeshNodeLocalApi_AVX256;
}

bool CollisionResource::getGridSize(uint8_t behavior_filter, IPoint3 &width, Point3 &leaf_size) const
{
  if (auto tracer = getTracer(behavior_filter))
  {
    width = tracer->getLeafLimits().width() + IPoint3(1, 1, 1);
    leaf_size = tracer->getLeafSize();
    return true;
  }
  return false;
}

int CollisionResource::getTrianglesCount(uint8_t behavior_filter) const
{
  int count = 0;
  for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if (meshNode->checkBehaviorFlags(behavior_filter))
      count += meshNode->indices.size() / 3;
  }
  return count;
}

DAGOR_NOINLINE void CollisionResource::addTracesProfileTag(dag::Span<CollisionTrace> traces)
{
  unsigned activeTraces = 0;
  for (CollisionTrace &trace : traces)
  {
    if (trace.isHit || trace.isectBounding)
      activeTraces++;
  }
  DA_PROFILE_TAG(collres_traces, ": %u/%u", activeTraces, traces.size());
}

DAGOR_NOINLINE void CollisionResource::addMeshNodesProfileTag(const ProfileStats &profile_stats)
{
#if DA_PROFILER_ENABLED
  DA_PROFILE_TAG(mesh_nodes, ": %u/%u/%u; Tri: %u/%u", profile_stats.meshNodesBoxCheckPassed, profile_stats.meshNodesSphCheckPassed,
    profile_stats.meshNodesNum, profile_stats.meshTrianglesHits, profile_stats.meshTrianglesTraced);
#else
  G_UNUSED(profile_stats);
#endif
}
