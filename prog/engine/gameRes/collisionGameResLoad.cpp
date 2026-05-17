// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/dag_plane3.h>
#include <sceneRay/dag_sceneRayBuildable.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_btagCompr.h>
#include <generic/dag_sort.h>
#include <debug/dag_debug.h>

#if (_TARGET_PC && !_TARGET_STATIC_LIB)
// Enable FRT for all objects in daEditor for capsule clipping
#define MIN_FACES_TO_CREATE_GRID 0
#define MIN_WIDTH_TO_CREATE_GRID 0
#else
#define MIN_FACES_TO_CREATE_GRID 64
#define MIN_WIDTH_TO_CREATE_GRID 5
#endif
#define USE_TRACE_GRID 1

CollisionResource *CollisionResource::loadResource(IGenLoad &crd, int res_id)
{
  CollisionResource *resource = new CollisionResource;
  resource->load(crd, res_id);
  return resource;
}

static void singleMesh_initBounds(CollisionResource *res, const BBox3 &bbox, const BSphere3 &bsphere)
{
  v_bbox3_init(res->vFullBBox, v_ldu(&bbox[0].x));
  v_bbox3_add_pt(res->vFullBBox, v_ldu(&bbox[1].x));
  res->vBoundingSphere = v_make_vec4f(bsphere.c.x, bsphere.c.y, bsphere.c.z, bsphere.r2);
  res->boundingBox = bbox;
  res->boundingSphereRad = bsphere.r;
}

CollisionResource *CollisionResource::createSingleMesh(dag::ConstSpan<Point3_vec4> ref_vertices, dag::ConstSpan<uint16_t> indices,
  const BBox3 &bbox, const BSphere3 &bsphere, uint32_t flags)
{
  G_ASSERTF(!ref_vertices.empty() && ref_vertices.size() <= 0x10000u, "single-mesh vert count %d out of range (1..65536)",
    (int)ref_vertices.size());
  CollisionResource *res = new CollisionResource;
  singleMesh_initBounds(res, bbox, bsphere);
  res->ownVertices.assign(ref_vertices.begin(), ref_vertices.end());
  res->ownIndices.assign(indices.begin(), indices.end());
  res->meshVertsBase = res->ownVertices.data();
  res->meshIndicesBase = res->ownIndices.data();
  CollisionNode &node = res->createNode();
  node.flags = flags | CollisionNode::IDENT;
  node.type = COLLISION_NODE_TYPE_MESH;
  node.modelBBox = bbox;
  node.boundingSphere.c = bsphere.c;
  node.boundingSphere.r = bsphere.r;
  node.verticesOfs = 0;
  node.verticesCount = (uint16_t)(ref_vertices.size() - 1); // count-minus-one encoding (1..65536)
  node.indicesOfs = 0;
  node.indicesCount = (uint32_t)indices.size();
  res->numMeshNodes = 1;
  res->meshNodesHead = 0; // sole node is at index 0; node.nextNode is INVALID_IDX by default
  return res;
}

CollisionResource *CollisionResource::createSingleMeshNonOwning(dag::ConstSpan<Point3_vec4> ref_vertices,
  dag::ConstSpan<uint16_t> indices, const BBox3 &bbox, const BSphere3 &bsphere)
{
  G_ASSERTF(!ref_vertices.empty() && ref_vertices.size() <= 0x10000u, "single-mesh vert count %d out of range (1..65536)",
    (int)ref_vertices.size());
  CollisionResource *res = new CollisionResource;
  singleMesh_initBounds(res, bbox, bsphere);
  // Verts come from an externally-owned buffer (e.g. FRT); indices are copied into the
  // resource's own dense storage so the caller does not need to keep its index buffer alive.
  res->ownIndices.assign(indices.begin(), indices.end());
  res->meshVertsBase = ref_vertices.data();
  res->meshIndicesBase = res->ownIndices.data();
  CollisionNode &node = res->createNode();
  node.flags = CollisionNode::IDENT;
  node.type = COLLISION_NODE_TYPE_MESH;
  node.modelBBox = bbox;
  node.boundingSphere.c = bsphere.c;
  node.boundingSphere.r = bsphere.r;
  node.verticesOfs = 0;
  node.verticesCount = (uint16_t)(ref_vertices.size() - 1); // count-minus-one encoding (1..65536)
  node.indicesOfs = 0;
  node.indicesCount = (uint32_t)indices.size();
  res->numMeshNodes = 1;
  res->meshNodesHead = 0; // sole node is at index 0; node.nextNode is INVALID_IDX by default
  return res;
}

int CollisionResource::addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere)
{
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_SPHERE;
  n.flags = CollisionNode::IDENT;
  n.tm = TMatrix::IDENT;
  n.boundingSphere.c = bsphere.c;
  n.boundingSphere.r = bsphere.r;
  n.modelBBox = BBox3(bsphere);
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

int CollisionResource::addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox)
{
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_BOX;
  n.flags = CollisionNode::IDENT;
  n.tm = TMatrix::IDENT;
  n.modelBBox = bbox;
  n.boundingSphere.c = bbox.center();
  n.boundingSphere.r = bbox.width().length() / 2.f;
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

int CollisionResource::addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius)
{
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_CAPSULE;
  n.flags = CollisionNode::IDENT;
  n.tm = TMatrix::IDENT;
  n.modelBBox = (BBox3(BSphere3(p0, radius)) += BSphere3(p1, radius));
  n.boundingSphere.c = (p0 + p1) / 2.f;
  n.boundingSphere.r = (p0 - p1).length() / 2.f + radius;
  n.capsuleIndex = (uint16_t)capsules.size();
  capsules.push_back(Capsule(p0, p1, radius));
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

int CollisionResource::addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  const BSphere3 &bsphere, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, uint16_t behavior_flags, uint8_t flags)
{
  G_ASSERTF_RETURN(verts.size() <= 0x10000u, -1, "addMeshNode vert count %d out of range (0..65536)", (int)verts.size());
  G_ASSERTF_RETURN(verts.size() > 2 && indices.size() > 2 && (indices.size() % 3) == 0, -1,
    "addMeshNode: malformed mesh geometry: verts.size=%d (need >2), indices.size=%d (need >2 and %%3==0)", (int)verts.size(),
    (int)indices.size());
  // Owning-mode only: trailing meshVertsBase/meshIndicesBase reassignments would silently repoint
  // existing non-owning nodes (createSingleMeshNonOwning seeds meshVertsBase from a caller-owned
  // buffer). Refuse in release too, not just debug -- silent retarget corrupts the prior node.
  G_ASSERTF_RETURN(meshVertsBase == nullptr || meshVertsBase == ownVertices.data(), -1,
    "addMeshNode: cannot extend a non-owning CollisionResource (meshVertsBase points at external buffer)");
  G_ASSERTF_RETURN(meshIndicesBase == nullptr || meshIndicesBase == ownIndices.data(), -1,
    "addMeshNode: cannot extend a non-owning CollisionResource (meshIndicesBase points at external buffer)");
  CollisionNode &n = createNode();
  const int idx = (int)allNodesList.size() - 1;
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_MESH;
  n.flags = flags;
  n.behaviorFlags = behavior_flags;
  n.tm = tm;
  const float len0sq = tm.getcol(0).lengthSq();
  const float len1sq = tm.getcol(1).lengthSq();
  const float len2sq = tm.getcol(2).lengthSq();
  n.cachedMaxTmScale = sqrtf(max(len0sq, max(len1sq, len2sq)));
  n.modelBBox = bbox;
  n.boundingSphere.c = bsphere.c;
  n.boundingSphere.r = bsphere.r;
  n.nodeIndex = (uint16_t)idx;
  if (!verts.empty())
  {
    n.verticesOfs = (uint32_t)ownVertices.size();
    n.verticesCount = (uint16_t)(verts.size() - 1); // count-minus-one encoding (1..65536)
    ownVertices.insert(ownVertices.end(), verts.begin(), verts.end());
    meshVertsBase = ownVertices.data();
  }
  if (!indices.empty())
  {
    n.indicesOfs = (uint32_t)ownIndices.size();
    n.indicesCount = (uint32_t)indices.size();
    ownIndices.insert(ownIndices.end(), indices.begin(), indices.end());
    meshIndicesBase = ownIndices.data();
  }
  numMeshNodes++;
  return idx;
}

int CollisionResource::addConvexNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  const BSphere3 &bsphere, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, dag::ConstSpan<plane3f> convex_planes,
  uint16_t behavior_flags, uint8_t flags)
{
  // Reuses addMeshNode for the geometry payload, then re-types as CONVEX and appends planes.
  // numMeshNodes counts mesh-typed nodes; bump back down since this node ends up CONVEX.
  // A CONVEX node with planesCount == 0 is malformed (convex code paths iterate planesCount
  // directly), so reject an empty plane span before the type retag.
  G_ASSERTF_RETURN(!convex_planes.empty(), -1, "addConvexNode: convex_planes must be non-empty");
  G_ASSERTF_RETURN(convexPlanes.size() + convex_planes.size() < 0x10000u, -1, "convex plane buffer overflow: %d + %d > 65536",
    (int)convexPlanes.size(), (int)convex_planes.size());
  const int idx = addMeshNode(name, phys_mat_id, tm, bbox, bsphere, verts, indices, behavior_flags, flags);
  if (idx < 0)
    return idx;
  CollisionNode &n = allNodesList[idx];
  n.type = COLLISION_NODE_TYPE_CONVEX;
  if (numMeshNodes > 0)
    numMeshNodes--;
  n.planesOfs = (uint16_t)convexPlanes.size();
  n.planesCount = (uint16_t)convex_planes.size();
  convexPlanes.insert(convexPlanes.end(), convex_planes.begin(), convex_planes.end());
  return idx;
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

static inline auto load_frt16(IGenLoad &cb) { return DeserializedStaticSceneRayTracerT<uint16_t>::load(cb); }

// Legacy on-disk bits in CollisionNode::flags signaling that the per-node mesh data was written
// as an offset into the FRT vertex/face dump rather than as raw data. The runtime no longer keeps
// these flags; the loader decodes the FRT slice into ownVertices/ownIndices and clears the bits.
static constexpr uint8_t LEGACY_FLAG_VERTICES_ARE_REFS = 64;
static constexpr uint8_t LEGACY_FLAG_INDICES_ARE_REFS = 128;

void CollisionResource::load(IGenLoad &_cb, int res_id)
{
  collisionFlags = 0;
  gridForTraceable.tracer.reset(nullptr);
  gridForCollidable.tracer.reset(nullptr);
  allNodesList.clear();
  names.clear();
  capsules.clear();
  convexPlanes.clear();
  ownVertices.clear();
  ownIndices.clear();
  meshVertsBase = nullptr;
  meshIndicesBase = nullptr;

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
  String tmp_node_name;
  // Reused across iterations to stage planes before committing them to the resource-level
  // convexPlanes pool. The commit happens at the bottom of each iteration so that any future
  // node-drop logic added between read and commit cannot orphan plane data.
  dag::RelocatableFixedVector<plane3f, 16, true, framemem_allocator> stagedPlanes;
  for (auto &n : allNodesList)
  {
    zcrd->readString(tmp_node_name);
    n.nameOfs = addName(tmp_node_name.c_str());
    zcrd->readString(tmp_str);
    n.physMatId = PhysMat::getMaterialId(tmp_str.str());

    zcrd->read(&n.modelBBox, sizeof(n.modelBBox));
    {
      BSphere3 tmpSph;
      zcrd->read(&tmpSph, sizeof(BSphere3));
      n.boundingSphere.c = tmpSph.c;
      n.boundingSphere.r = tmpSph.r;
    }
    n.behaviorFlags = zcrd->readIntP<2>();
    n.flags = zcrd->readIntP<1>();
    n.type = (CollisionResourceNodeType)zcrd->readIntP<1>();
    zcrd->readReal(n.cachedMaxTmScale);
    zcrd->read(&n.tm, sizeof(n.tm));
    n.insideOfNode = zcrd->readIntP<2>();

    {
      uint16_t planesCnt = (uint16_t)zcrd->readIntP<2>();
      stagedPlanes.resize(planesCnt);
      if (planesCnt)
        zcrd->read(stagedPlanes.data(), planesCnt * sizeof(plane3f));
    }

    if (int cnt = zcrd->readInt())
    {
      if ((uint32_t)cnt > 0x10000u)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        resName = "unknown";
#endif
        DAG_FATAL("Mesh vertex count %d > 65536 in node <%s> of res <%s>", cnt, getNodeNameStr(n), resName.c_str());
      }
      const uint32_t prev = (uint32_t)ownVertices.size();
      n.verticesOfs = prev;
      n.verticesCount = (uint16_t)(cnt - 1); // count-minus-one encoding (1..65536)
      ownVertices.resize(prev + cnt);
      if (n.flags & LEGACY_FLAG_VERTICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        auto &tracer = ((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer;
        memcpy(ownVertices.data() + prev, &tracer->verts(ofs & 0xFFFFFF), cnt * sizeof(Point3_vec4));
      }
      else
        zcrd->read(ownVertices.data() + prev, cnt * sizeof(Point3_vec4));
    }

    if (int cnt = zcrd->readInt())
    {
      const uint32_t prev = (uint32_t)ownIndices.size();
      n.indicesOfs = prev;
      n.indicesCount = (uint32_t)cnt;
      ownIndices.resize(prev + cnt);
      if (n.flags & LEGACY_FLAG_INDICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        auto &tracer = ((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer;
        memcpy(ownIndices.data() + prev, (uint16_t *)tracer->faces(0).v + (ofs & 0xFFFFFF), cnt * sizeof(uint16_t));
      }
      else
        zcrd->read(ownIndices.data() + prev, cnt * sizeof(uint16_t));
    }
    n.flags &= ~(LEGACY_FLAG_VERTICES_ARE_REFS | LEGACY_FLAG_INDICES_ARE_REFS);
    if (n.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      Capsule c;
      c.set(n.modelBBox);
      c.transform(n.tm);
      n.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }

    // Commit staged planes only after the node is fully populated and known to be kept.
    // If a future revision adds an empty-bbox or validation-based node-drop here, simply
    // `continue` without running this block to avoid orphan entries in convexPlanes.
    if (!stagedPlanes.empty())
    {
      size_t prevSize = convexPlanes.size();
      // planesOfs is uint16_t; offsets above 65535 would wrap on cast and read the wrong slice
      // back. Bail out with a fatal error rather than silently corrupting the resource.
      if (prevSize > eastl::numeric_limits<uint16_t>::max())
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize,
          getNodeNameStr(n), resName.c_str());
      }
      n.planesOfs = (uint16_t)prevSize;
      n.planesCount = (uint16_t)stagedPlanes.size();
      convexPlanes.resize(prevSize + stagedPlanes.size());
      memcpy(convexPlanes.data() + prevSize, stagedPlanes.data(), stagedPlanes.size() * sizeof(plane3f));
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

  names.shrink_to_fit();
  capsules.shrink_to_fit();
  convexPlanes.shrink_to_fit();
  ownVertices.shrink_to_fit();
  ownIndices.shrink_to_fit();
  meshVertsBase = ownVertices.data();
  meshIndicesBase = ownIndices.data();
  rebuildNodesLL();
  /* if (!validateVerticesForJolt())
  {
    String resName;
    get_game_resource_name(res_id, resName);
    logerr("Degenerative triangles detected in res: %s", resName.c_str());
  }*/
}

void CollisionResource::loadLegacyRawFormat(IGenLoad &_cb, int res_id, int (*resolve_phmat)(const char *))
{
  convexPlanes.clear();
  ownVertices.clear();
  ownIndices.clear();
  meshVertsBase = nullptr;
  meshIndicesBase = nullptr;
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

    String tmp_node_name;
    cb.readString(tmp_node_name);
    node.nameOfs = addName(tmp_node_name.c_str());
    if (hasMaterialData)
    {
      String matName;
      cb.readString(matName);
      node.physMatId = resolve_phmat ? resolve_phmat(matName.c_str()) : PhysMat::getMaterialId(matName.c_str());
    }
    else
      node.physMatId = PHYSMAT_INVALID;

    uint32_t typeAndBehFlags = cb.readInt();
    node.type = (CollisionResourceNodeType)(typeAndBehFlags & 0xFFu);
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

    {
      BSphere3 tmpSph;
      cb.read(&tmpSph, sizeof(BSphere3));
      node.boundingSphere.c = tmpSph.c;
      node.boundingSphere.r = tmpSph.r;
    }
    cb.read(&node.modelBBox, sizeof(BBox3));
    // Stage convex planes in tempConvexPlanes; they are pushed into the resource-level
    // convexPlanes array only after the bbox-validity check below, so dropped nodes do not
    // orphan plane data.
    if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      readTab(cb, tempConvexPlanes);
      for (int i = 0; i < tempConvexPlanes.size(); ++i)
        tempConvexPlanes[i].normalize();
    }
    else
      tempConvexPlanes.clear();
    readTab(cb, tempVertices);
    if (!tempVertices.empty())
    {
      int verticesLimit = eastl::numeric_limits<uint16_t>::max();
      if (tempVertices.size() > verticesLimit)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Mesh vertexes count %i > %i in node <%s> of res <%s>", tempVertices.size(), verticesLimit, getNodeNameStr(node),
          resName.c_str());
      }
    }

    readTab(cb, tempIndices);
    bbox3f nodeBBox = v_ldu_bbox3(node.modelBBox);
    if (!tempIndices.empty() && !tempVertices.empty())
    {
      v_bbox3_init_empty(nodeBBox);
      for (int i = 0; i < tempVertices.size(); i++)
        v_bbox3_add_pt(nodeBBox, v_ldu(&tempVertices[i].x));

      v_stu_bbox3(node.modelBBox, nodeBBox);
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
      continue;
    }

    // Node is kept: commit its mesh data into the resource-wide arrays.
    if (!tempVertices.empty())
    {
      const uint32_t prev = (uint32_t)ownVertices.size();
      node.verticesOfs = prev;
      node.verticesCount = (uint16_t)(tempVertices.size() - 1); // count-minus-one encoding
      ownVertices.resize(prev + tempVertices.size());
      Point3_vec4 *dst = ownVertices.data() + prev;
      for (int i = 0; i < tempVertices.size(); ++i)
      {
        dst[i] = tempVertices[i];
        dst[i].resv = 1.0f;
      }
    }
    if (!tempIndices.empty() && !tempVertices.empty())
    {
      const uint32_t prev = (uint32_t)ownIndices.size();
      node.indicesOfs = prev;
      node.indicesCount = (uint32_t)tempIndices.size();
      ownIndices.resize(prev + tempIndices.size());
      uint16_t *dst = ownIndices.data() + prev;
      for (int i = 0; i < tempIndices.size(); i += 3) // rotate (0,1,2)->(0,2,1) and narrow-convert int -> uint16_t
      {
        dst[i + 0] = (uint16_t)tempIndices[i + 0];
        dst[i + 2] = (uint16_t)tempIndices[i + 1];
        dst[i + 1] = (uint16_t)tempIndices[i + 2];
      }
    }

    if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      Capsule c;
      c.set(node.modelBBox);
      c.transform(node.tm);
      node.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }
    else if (node.type == COLLISION_NODE_TYPE_CONVEX && !tempConvexPlanes.empty())
    {
      size_t prevSize = convexPlanes.size();
      if (prevSize > eastl::numeric_limits<uint16_t>::max())
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize,
          getNodeNameStr(node), resName.c_str());
      }
      node.planesOfs = (uint16_t)prevSize;
      node.planesCount = (uint16_t)tempConvexPlanes.size();
      convexPlanes.resize(prevSize + tempConvexPlanes.size());
      for (int i = 0; i < tempConvexPlanes.size(); ++i)
        convexPlanes[prevSize + i] = v_ldu(&tempConvexPlanes[i].n.x);
    }
  }

  vFullBBox = totalBBox;
  v_stu_bbox3(boundingBox, totalBBox);

  names.shrink_to_fit();
  capsules.shrink_to_fit();
  convexPlanes.shrink_to_fit();
  ownVertices.shrink_to_fit();
  ownIndices.shrink_to_fit();
  meshVertsBase = ownVertices.data();
  meshIndicesBase = ownIndices.data();
  sortNodesList();
  rebuildNodesLL();

  _cb.endBlock();
}

void CollisionResource::collapseAndOptimize(const char *res_name, bool need_frt, bool frt_build_fast)
{
  if (collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
  {
    // debug("skip already optimized %p", this);
    return;
  }
  collisionFlags |= COLLISION_RES_FLAG_OPTIMIZED;
  if (meshNodesHead == CollisionNode::INVALID_IDX) // || numMeshNodes < 2
  {
    // debug("CollisionResource: nothing to optimize %p", this);
    return;
  }

  // Phase A: bake non-IDENT TM into geometry, in place against ownVertices/ownIndices.
  G_ASSERT(meshVertsBase == ownVertices.data() && meshIndicesBase == ownIndices.data());
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    CollisionNode *m = &allNodesList[mi];
    if (m->type != COLLISION_NODE_TYPE_CONVEX && m->type != COLLISION_NODE_TYPE_MESH)
      continue;
    if ((m->flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT || m->indicesCount == 0)
      continue;
    Point3_vec4 *vbase = ownVertices.data() + m->verticesOfs;
    uint16_t *ibase = ownIndices.data() + m->indicesOfs;
    const uint32_t mVCount = (uint32_t)m->verticesCount + 1u;
    mat44f nodeTm;
    bbox3f box;
    v_mat44_make_from_43cu(nodeTm, m->tm[0]);
    v_bbox3_init_empty(box);
    for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
    {
      *verts = v_mat44_mul_vec3p(nodeTm, *verts);
      v_bbox3_add_pt(box, *verts);
    }
    vec4f vSphereC = v_bbox3_center(box), sphereRad2 = v_zero();
    for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
      sphereRad2 = v_max(sphereRad2, v_length3_sq_x(v_sub(vSphereC, *verts)));

    mat44f N, TN;
    v_mat44_inverse(N, nodeTm);
    v_mat44_transpose(TN, N);
    plane3f *__restrict base = convexPlanes.data() + m->planesOfs;
    for (plane3f *__restrict planes = base, *pe = base + m->planesCount; planes != pe; ++planes)
      *planes = v_mat44_mul_vec4(TN, *planes);

    v_stu_bbox3(m->modelBBox, box);
    v_stu_p3(&m->boundingSphere.c.x, vSphereC);
    m->boundingSphere.r = v_extract_x(v_sqrt_x(sphereRad2));

    if (m->tm.det() < 0.f) // swap indices order
      for (uint32_t i = 0, e = m->indicesCount; i + 2 < e; i += 3)
        eastl::swap(ibase[i + 0], ibase[i + 2]);
    m->tm.identity();
    m->flags = CollisionNode::IDENT | (m->flags & (~CollisionNode::TRANSLATE));
    m->flags = CollisionNode::ORTHONORMALIZED | (m->flags & (~CollisionNode::ORTHOUNIFORM));
    m->cachedMaxTmScale = 1.f;
  }

  // Phase B: bucket mesh nodes by (matId, isPhysCollidable) and build per-bucket merged geometry
  // into framemem temps. We do not mutate ownVertices/ownIndices here -- the rebuild pass below
  // produces fresh dense arrays from the existing slices plus the merged buckets.
  IMemAlloc *framemem = framemem_ptr();
  Tab<Tab<CollisionNode *>> meshNodesByMat(framemem);
  Tab<eastl::pair<PhysMat::MatID, bool>> matIndices(framemem);

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    CollisionNode *m = &allNodesList[mi];
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

  // Per-bucket merged data, keyed by target nodeIndex (= nodes[0]->nodeIndex). The eventual rebuild
  // walks allNodesList; for each surviving node we look up a possibly-pending bucket merge here.
  struct BucketMerge
  {
    uint16_t targetNodeIndex;
    dag::Vector<Point3_vec4, framemem_allocator> verts;
    dag::Vector<uint16_t, framemem_allocator> indices;
  };
  dag::Vector<BucketMerge, framemem_allocator> bucketMerges;
  bucketMerges.reserve(meshNodesByMat.size());
  dag::RelocatableFixedVector<int, 256> nodesToRemove;
  nodesToRemove.reserve(allNodesList.size());

  for (const Tab<CollisionNode *> &nodes : meshNodesByMat)
  {
    CollisionNode *targetNode = nodes[0];
    uint64_t v_total = 0, i_total = 0;
    for (const CollisionNode *node : nodes)
    {
      // Bucketed nodes are mesh nodes added to meshNodesByMat only when type==MESH and traceable;
      // they always have valid mesh data (indicesCount > 0), so the +1 decode is safe.
      v_total += (uint32_t)node->verticesCount + 1u;
      i_total += node->indicesCount;
    }
    if (v_total > 65530)
    {
      debug("CollisionResource: too many faces %llu, cannot optimize %p res <%s>", (unsigned long long)v_total, this, res_name);
      continue;
    }
    if (v_total == 0)
      continue;

    BucketMerge bm;
    bm.targetNodeIndex = targetNode->nodeIndex;
    bm.verts.reserve((size_t)v_total);
    bm.indices.reserve((size_t)i_total);
    BBox3 bbox;
    uint16_t v_off = 0;
    for (CollisionNode *m : nodes)
    {
      const Point3_vec4 *srcVerts = ownVertices.data() + m->verticesOfs;
      const uint16_t *srcIdx = ownIndices.data() + m->indicesOfs;
      const uint32_t mVCount = (uint32_t)m->verticesCount + 1u;
      for (uint32_t i = 0, e = mVCount; i < e; ++i)
      {
        bm.verts.push_back(srcVerts[i]);
        bbox += srcVerts[i];
      }
      for (uint32_t i = 0, e = m->indicesCount; i < e; ++i)
        bm.indices.push_back(uint16_t(srcIdx[i] + v_off));
      v_off = (uint16_t)(v_off + mVCount);
    }

    targetNode->modelBBox = bbox;
    targetNode->boundingSphere.c = bbox.center();
    float r2 = 0.f;
    for (const Point3_vec4 &v : bm.verts)
      inplace_max(r2, lengthSq(v - targetNode->boundingSphere.c));
    targetNode->boundingSphere.r = sqrtf(r2);
    targetNode->tm.identity();
    targetNode->flags |= targetNode->IDENT;
    targetNode->flags &= ~targetNode->TRANSLATE;
    targetNode->cachedMaxTmScale = 1.f;

    bucketMerges.push_back(eastl::move(bm));
  }

  // Drop duplicate bucket members for EVERY bucket, including ones that exceeded the merge limit
  // (matches the pre-284581bf1d behavior: failed buckets keep only nodes[0] so total vert count
  // stays under the 16-bit FRT index limit; trades silent data loss for a successful build).
  // fixme: this has to be removed when CollisionResource is on BVH
  for (const Tab<CollisionNode *> &nodes : meshNodesByMat)
    for (int i = 1; i < nodes.size(); ++i)
      nodesToRemove.push_back(nodes[i]->nodeIndex);

  // Compact allNodesList: drop merged-away nodes. Surviving nodes keep their (now-stale) offsets;
  // they will be re-stamped during the rebuild pass below.
  Tab<CollisionNode> newAllNodes(tmpmem);
  newAllNodes.reserve(allNodesList.size());
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

  // Rebuild ownVertices/ownIndices in node order: for each surviving mesh/convex node, append
  // either the bucket-merged data (if the node is a merge target) or the existing slice from the
  // old pool. Stamp fresh offsets/counts on the node.
  dag::Vector<Point3_vec4> newOwnVertices;
  dag::Vector<uint16_t> newOwnIndices;
  uint64_t totalV = 0, totalI = 0;
  for (CollisionNode &n : allNodesList)
  {
    if (n.indicesCount) // gate on indicesCount: verticesCount uses count-minus-one and means nothing for empty/non-mesh nodes
    {
      totalV += (uint32_t)n.verticesCount + 1u;
      totalI += n.indicesCount;
    }
  }
  for (const BucketMerge &bm : bucketMerges)
  {
    totalV += bm.verts.size();
    totalI += bm.indices.size();
  }
  newOwnVertices.reserve((size_t)totalV);
  newOwnIndices.reserve((size_t)totalI);
  for (CollisionNode &n : allNodesList)
  {
    if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    const BucketMerge *bm = nullptr;
    for (const BucketMerge &candidate : bucketMerges)
      if (candidate.targetNodeIndex == n.nodeIndex)
      {
        bm = &candidate;
        break;
      }
    if (bm)
    {
      n.verticesOfs = (uint32_t)newOwnVertices.size();
      n.verticesCount = (uint16_t)(bm->verts.size() - 1); // count-minus-one encoding
      n.indicesOfs = (uint32_t)newOwnIndices.size();
      n.indicesCount = (uint32_t)bm->indices.size();
      newOwnVertices.insert(newOwnVertices.end(), bm->verts.begin(), bm->verts.end());
      newOwnIndices.insert(newOwnIndices.end(), bm->indices.begin(), bm->indices.end());
    }
    else if (n.indicesCount)
    {
      const uint32_t newVOfs = (uint32_t)newOwnVertices.size();
      const uint32_t newIOfs = (uint32_t)newOwnIndices.size();
      const Point3_vec4 *srcV = ownVertices.data() + n.verticesOfs;
      const uint16_t *srcI = ownIndices.data() + n.indicesOfs;
      const uint32_t nVCount = (uint32_t)n.verticesCount + 1u;
      newOwnVertices.insert(newOwnVertices.end(), srcV, srcV + nVCount);
      newOwnIndices.insert(newOwnIndices.end(), srcI, srcI + n.indicesCount);
      n.verticesOfs = newVOfs;
      n.indicesOfs = newIOfs;
    }
  }
  ownVertices = eastl::move(newOwnVertices);
  ownIndices = eastl::move(newOwnIndices);
  ownVertices.shrink_to_fit();
  ownIndices.shrink_to_fit();
  meshVertsBase = ownVertices.data();
  meshIndicesBase = ownIndices.data();

  sortNodesList();
  rebuildNodesLL();

  gridForTraceable.reset();
  gridForCollidable.reset();

  if (need_frt)
  {
    bool coll_and_trace_frt_equals = true;
    for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
    {
      const CollisionNode *node = &allNodesList[mi];
      if (node->checkBehaviorFlags(CollisionNode::TRACEABLE) != node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      {
        coll_and_trace_frt_equals = false;
        break;
      }
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

void CollisionResource::Grid::buildFRT(CollisionResource *parent, uint8_t behavior_flag, bool frt_build_fast)
{
  reset();

  if (!USE_TRACE_GRID || parent->meshNodesHead == CollisionNode::INVALID_IDX ||
      !(parent->allNodesList[parent->meshNodesHead].flags & CollisionNode::IDENT))
    return;

  bbox3f fullMeshBox;
  v_bbox3_init_empty(fullMeshBox);
  int totalIndices = 0;
  for (uint16_t mi = parent->meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = parent->allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &parent->allNodesList[mi];
    if (!node->checkBehaviorFlags(behavior_flag))
      continue;
    if (node->checkBehaviorFlags(CollisionNode::SOLID))
      return; // node requires trace without culling
    totalIndices += node->indicesCount;
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
    for (uint16_t mi = parent->meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = parent->allNodesList[mi].nextNode)
    {
      const CollisionNode *node = &parent->allNodesList[mi];
      if (!node->checkBehaviorFlags(behavior_flag)) // we do not add non-traceable node to grid
        continue;
      maxIndices = max(maxIndices, node->indicesCount);
      totalIndices += node->indicesCount;
      totalVertices += (uint32_t)node->verticesCount + 1u;
    }
    localGrid->reserve(totalIndices, totalVertices);

    for (uint16_t mi = parent->meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = parent->allNodesList[mi].nextNode)
    {
      const CollisionNode *node = &parent->allNodesList[mi];
      if (!node->checkBehaviorFlags(behavior_flag)) // we do not add non-traceable node to grid
        continue;
      const Point3_vec4 *nodeVerts = parent->meshVertsBase + node->verticesOfs;
      const uint16_t *nodeIdx = parent->meshIndicesBase + node->indicesOfs;
      localGrid->addmesh((uint8_t *)nodeVerts, sizeof(vec4f), (uint32_t)node->verticesCount + 1u, //
        nodeIdx, sizeof(uint16_t) * 3, node->indicesCount / 3, nullptr, false);
    }
    localGrid->setCullFlags(StaticSceneRayTracer::CULL_BOTH);
    localGrid->rebuild(frt_build_fast);
    tracer.reset(localGrid);
  }
}
