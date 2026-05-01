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

CollisionResource *CollisionResource::createSingleMesh(dag::Span<Point3_vec4> ref_vertices, dag::Span<uint16_t> indices,
  const BBox3 &bbox, const BSphere3 &bsphere, uint32_t flags)
{
  CollisionResource *res = new CollisionResource;
  v_bbox3_init(res->vFullBBox, v_ldu(&bbox[0].x));
  v_bbox3_add_pt(res->vFullBBox, v_ldu(&bbox[1].x));
  res->vBoundingSphere = v_make_vec4f(bsphere.c.x, bsphere.c.y, bsphere.c.z, bsphere.r2);
  res->boundingBox = bbox;
  res->boundingSphereRad = bsphere.r;

  CollisionNode &node = res->createNode();
  node.flags = flags | CollisionNode::IDENT;
  node.type = COLLISION_NODE_TYPE_MESH;
  node.modelBBox = bbox;
  node.boundingSphere.c = bsphere.c;
  node.boundingSphere.r = bsphere.r;
  node.resetVertices(ref_vertices);
  node.resetIndices(indices);

  res->numMeshNodes = 1;
  res->meshNodesHead = &node;
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

void CollisionResource::load(IGenLoad &_cb, int res_id)
{
  collisionFlags = 0;
  gridForTraceable.tracer.reset(nullptr);
  gridForCollidable.tracer.reset(nullptr);
  allNodesList.clear();
  names.clear();
  capsules.clear();

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

    reserve_and_resize(n.convexPlanes, zcrd->readIntP<2>());
    zcrd->readTabData(n.convexPlanes);

    if (int cnt = zcrd->readInt())
    {
      if (n.flags & n.FLAG_VERTICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        {
          auto &tracer = ((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer;
          n.resetVertices({(Point3_vec4 *)&tracer->verts(ofs & 0xFFFFFF), cnt});
        }
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
        auto &tracer = ((ofs & 0x40000000) ? gridForCollidable : gridForTraceable).tracer;
        n.resetIndices({(uint16_t *)tracer->faces(0).v + (ofs & 0xFFFFFF), cnt});
      }
      else
      {
        n.resetIndices({memalloc_typed<uint16_t>(cnt, midmem), cnt});
        zcrd->read(n.indices.data(), data_size(n.indices));
      }
    }
    if (n.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      Capsule c;
      c.set(n.modelBBox);
      c.transform(n.tm);
      n.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
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
        DAG_FATAL("Mesh vertexes count %i > %i in node <%s> of res <%s>", tempVertices.size(), verticesLimit, getNodeNameStr(node),
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
        v_bbox3_add_pt(nodeBBox, v_ld(&node.vertices[i].x));

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
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      Capsule c;
      c.set(node.modelBBox);
      c.transform(node.tm);
      node.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }
  }

  vFullBBox = totalBBox;
  v_stu_bbox3(boundingBox, totalBBox);

  names.shrink_to_fit();
  capsules.shrink_to_fit();
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
      debug("CollisionResource: too many faces %d, cannot optimize %p res <%s>", v_num, this, res_name);
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
    float r2 = lengthSq(vertices[0] - targetNode->boundingSphere.c);
    for (int i = 1; i < v_num; i++)
      inplace_max(r2, lengthSq(vertices[i] - targetNode->boundingSphere.c));
    targetNode->boundingSphere.r = sqrtf(r2);

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
