// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_legacySceneRay.h>

// Every way a CollisionResource gets its content. The state is the Data block unfrozen, one
// growable vector per array, plus the raw geometry the passes edit; a pass reads and writes it
// directly. The product is the current stream (write) or a resource landed from it (build); the
// resource itself is never mutated past its construction.
struct CollisionResourceBuilder
{
  // parallel to nodes: authoredTm, bindMeta, geom, and relGeomTm under HAS_REL_GEOM_NODE_ID
  dag::Vector<CollisionNode> nodes; // verticesCount / indicesCount are the raw counts until the landing
  dag::Vector<TMatrix> authoredTm;
  dag::Vector<TMatrix> relGeomTm;
  dag::Vector<CollisionResourceInstance::PoseMeta> bindMeta; // the gates' verdicts, valid after every authored write
  dag::Vector<uint16_t> physMatPool;
  dag::Vector<Capsule> capsules;
  dag::Vector<plane3f> convexPlanes;
  dag::Vector<char> names;
  // the raw geometry: never in the stream, never in a resource
  dag::Vector<Point3_vec4> rawVerts;
  dag::Vector<uint32_t> rawIndices;
  dag::Vector<uint8_t> rawFaceUser; // per face of a fused node: its material's index in the node's set
  struct NodeGeom
  {
    static constexpr uint32_t NO_FACE_USERS = ~0u;
    uint32_t vertsOfs = 0, indicesOfs = 0;
    uint32_t faceUserOfs = NO_FACE_USERS;
    bool hasFaceUsers() const { return faceUserOfs != NO_FACE_USERS; }
  };
  dag::Vector<NodeGeom> geom;

  bbox3f vFullBBox;
  vec4f vBoundingSphere; // center | r^2
  BBox3 boundingBox;
  float boundingSphereRad = 0.f;
  uint32_t collisionFlags = 0;

  CollisionResourceBuilder();

  // Physmat ids are runtime ids; write maps them to names. A geometry node starts at its tm's
  // class; a primitive at identity. A mesh node's sphere is derived: the box center, the farthest vert.
  int addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere);
  int addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox);
  int addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius);
  int addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, dag::ConstSpan<Point3_vec4> verts,
    dag::ConstSpan<uint32_t> indices,
    uint16_t behavior_flags = CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE |
                              CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);
  int addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, dag::ConstSpan<Point3_vec4> verts,
    dag::ConstSpan<uint16_t> indices,
    uint16_t behavior_flags = CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE |
                              CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);
  int addConvexNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, dag::ConstSpan<Point3_vec4> verts,
    dag::ConstSpan<uint16_t> indices, dag::ConstSpan<plane3f> convex_planes,
    uint16_t behavior_flags = CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE |
                              CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);
  // One IDENT node per (part, class, material), numbered by the builder; -1 adds nothing.
  // A face's centroid picks its cell: 65536 m for a face past 512 m with every edge 1 m or more, else 3072 m.
  // A cell past the face or vert caps splits into parts.
  // A face's class is its index into class_flags, which holds one behaviour mask each.
  int addSplitMeshNodes(dag::ConstSpan<uint16_t> class_flags, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices,
    dag::ConstSpan<int16_t> face_pmid, dag::ConstSpan<uint8_t> face_class = {});
  // The FRT's faces as a soup: what its masks trace is the traceable class, the rest the
  // phys-only class without material. It answers as addSplitMeshNodes does, -1 adding nothing, and
  // an empty face_pmid gives every traceable face PHYSMAT_DEFAULT.
  int addStaticCollisionFrt(const StaticSceneRayTracer &frt, dag::ConstSpan<uint8_t> face_pmid);
  void eraseNode(int node_index); // every parallel array too; re-stamps nodeIndex
  // The set phys_mat_ids (dups and any order accepted; one entry stores inline, an empty set means
  // none). False, the node untouched, when the set is not encodable. coverage_order = false keeps
  // the caller's order (the fuse orders every union from one pre-install coverage snapshot).
  bool setNodePhysMats(int node_index, dag::ConstSpan<int> phys_mat_ids, bool coverage_order = true);
  // The authored placement: classified like loaded content, then the bind gates.
  void setNodeTm(int node_index, const TMatrix &tm);
  void setRelGeomNodeTm(int node_index, const TMatrix &tm); // sets HAS_REL_GEOM_NODE_ID

  const char *nodeName(int node_index) const { return names.empty() ? "" : names.data() + nodes[node_index].nameOfs; }
  // The node's palette_index-th material, PHYSMAT_INVALID past its count: the only public read of the sign-encoded physMatId.
  int nodePhysMatId(int node_index, int palette_index) const;
  // The raw slices of a mesh or convex node (empty for a dropped or primitive node).
  dag::ConstSpan<Point3_vec4> nodeVerts(int node_index) const
  {
    const CollisionNode &n = nodes[node_index];
    return n.hasGeometry() ? dag::ConstSpan<Point3_vec4>(rawVerts.data() + geom[node_index].vertsOfs, n.verticesCount)
                           : dag::ConstSpan<Point3_vec4>();
  }
  dag::ConstSpan<uint32_t> nodeIndices(int node_index) const
  {
    const CollisionNode &n = nodes[node_index];
    return n.hasGeometry() ? dag::ConstSpan<uint32_t>(rawIndices.data() + geom[node_index].indicesOfs, n.indicesCount)
                           : dag::ConstSpan<uint32_t>();
  }
  dag::ConstSpan<uint8_t> nodeFaceUsers(int node_index) const
  {
    const NodeGeom &ng = geom[node_index];
    return ng.hasFaceUsers() ? dag::ConstSpan<uint8_t>(rawFaceUser.data() + ng.faceUserOfs, nodes[node_index].indicesCount / 3u)
                             : dag::ConstSpan<uint8_t>();
  }
  dag::ConstSpan<plane3f> nodeConvexPlanes(int node_index) const
  {
    return dag::ConstSpan<plane3f>(convexPlanes.data() + nodes[node_index].planesOfs, nodes[node_index].planesCount);
  }
  // A mesh node's geometry replaced (node-local verts; empty: the node keeps no geometry): the
  // slice is appended, the node box and sphere follow the verts.
  void replaceNodeGeometry(int node_index, dag::ConstSpan<Point3> verts, dag::ConstSpan<uint32_t> indices);
  // The historical composed frame the bounds read: BOX / SPHERE through the authored placement,
  // the rest stored-space.
  BBox3 nodeBBox(int node_index) const;
  float nodeMaxTmScale(int node_index) const { return bindMeta[node_index].maxTmScale; }

  // Bakes a mesh or convex node's placement into its raw geometry (false: nothing to bake).
  bool bakeNodeTransform(int node_index);
  bool bakeMirroredNodes(); // every mesh or convex node under a mirrored (det < 0) placement
  // The collapse: bake every placed mesh node, merge the same-material same-behavior mesh nodes,
  // sort; sets OPTIMIZED, so the landing fuses the survivors by behavior with material sets.
  void collapse(const char *res_name);
  void sortNodes(); // size order and containment (insideOfNode), from scratch; re-stamps nodeIndex
  void recomputeBounds();

  // A v0 or v1 body after its label (v0 opens its own blocks; the caller opened the v1 block).
  bool loadLegacy(IGenLoad &crd, unsigned label, const char *res_name, int (*resolve_phmat)(const char *) = nullptr);
  // One node as the exporter's DAG walk and the v0 layout hold it: the exporter's box and sphere,
  // node-local geometry. beginRawNodes, then addRawNode for each node, then endRawNodes.
  struct RawNode
  {
    const char *name = "";
    const char *physMat = nullptr; // null: no material
    CollisionResourceNodeType type = COLLISION_NODE_TYPE_MESH;
    uint16_t behaviorFlags = 0; // read under COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS only
    TMatrix tm = TMatrix::IDENT;
    TMatrix relGeomTm = TMatrix::IDENT; // read under COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID only
    BSphere3 sphere;
    BBox3 box;
    dag::ConstSpan<Plane3> planes; // a convex node's
    dag::ConstSpan<Point3> verts;
    dag::ConstSpan<int> indices;
  };
  void beginRawNodes(uint32_t collision_flags, const BSphere3 &res_sphere);
  void addRawNode(const RawNode &src, const char *res_name, int (*resolve_phmat)(const char *));
  void endRawNodes();
  // Chunks decoded back to raw faces (with their per-face users), the authored flags rebaselined
  // from the source's live ones, everything else copied; the node order kept. The landing
  // re-chunks, so chunk bytes may differ from the source's.
  void fromResource(const CollisionResource &res);

  // The stream body of the landed state (the caller wraps the label and the block).
  bool write(IGenSave &cwr, const char *res_name, const char *(*mat_name)(int id));
  // A resource from the stream, physmat ids kept through the synthetic name pair; never null (a
  // refused stream leaves it empty). inplace_mem: the caller's storage, else operator new.
  CollisionResource *build(const char *res_name, void *inplace_mem = nullptr);

  // debug dump: raw block, no rebuild
  static void writeRaw(const CollisionResource &res, IGenSave &cwr);
  static bool readRaw(CollisionResource &res, IGenLoad &crd);

private:
  struct NodeBlasBuildScratch; // the chunk builds' reusable buffers
  uint32_t landName(const char *name);
  int newNode(const char *name);
  int nodePhysMatCount(int node_index) const;
  TMatrix geometryTm(int node_index) const; // stored geometry -> resource space at bind
  void setAuthoredNodeTm(int node_index, mat44f_cref tm, uint8_t class_flags, float max_scale);
  // Serialized BOX/SPHERE geometry to node-local storage; the singular and non-conformal
  // placements keep their bake (GEOMETRY_BAKED, RETAINED_BAKE).
  static void unbakePrimNode(CollisionNode &n, const TMatrix &tm, CollisionResourceInstance::PoseMeta &pm);
  void stampPoseScales();
  bool loadV0(IGenLoad &crd, const char *res_name, int (*resolve_phmat)(const char *));
  bool loadV1(IGenLoad &crd, const char *res_name, int (*resolve_phmat)(const char *));
  void mergeMeshNodes(const char *res_name, bool fuse_materials);
  void buildNodeBlasChunks(dag::Span<CollisionNode> built, dag::Vector<uint8_t> &chunks);
  bool buildOneNodeBlasChunk(CollisionNode &node, const Point3_vec4 *node_verts, unsigned node_vert_count, const uint32_t *node_idx,
    unsigned node_idx_count, NodeBlasBuildScratch &scratch, dag::Vector<uint8_t> &chunk_store, const uint8_t *face_user);
  void stampTwoSidedNodes(dag::Span<CollisionNode> built);
  bool isNodeEligibleForTwoSided(const CollisionNode &n, uint8_t behavior_flag) const;
  // Landing again is for a non-OPTIMIZED builder only: the fuse bakes and merges the raw state in
  // place. Only the stream constructor's legacy arms land without the stream; every other product
  // is write or build.
  bool land(CollisionResource &dst, const char *res_name);
  friend class CollisionResource;
  friend struct CollisionResourceUnittest;
};

// load_frt_block's pair, split into nodes and landed two-sided. Null, with the reason logged, when the split
// adds no node, which a face-less tracer also answers. The caller owns the resource.
CollisionResource *build_static_collision_frt(const StaticSceneRayTracer &frt, dag::ConstSpan<uint8_t> face_pmid, const char *name);

// An old level's FRT block, read from its version word on, which goes unchecked, and built into out_res, which the caller owns.
// False when the tracer does not read, true when it holds no face or builds nothing too; out_res is null unless a resource is built.
bool load_frt_block_as_collision(IGenLoad &crd, CollisionResource *&out_res);
