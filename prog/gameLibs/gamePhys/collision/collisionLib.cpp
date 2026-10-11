// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/dag_math.h>
#if _TARGET_C1 | _TARGET_C2

#elif defined(_MSC_VER) && !defined(__clang__)
#pragma fp_contract(off)
#endif

#include <gamePhys/collision/collisionLib.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_framemem.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/collisionResourceBuilder.h>
#include <perfMon/dag_statDrv.h>

#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <phys/dag_physics.h>
#include <phys/dag_physDebug.h>
#include <gamePhys/collision/physLayers.h>

#if ENABLE_APEX
#include <apex/physx.h>
#endif

#include <startup/dag_globalSettings.h>
#include <gameRes/dag_legacySceneRay.h>

#include <landMesh/lmeshManager.h>
#include <landMesh/landRayTracerSoA4.h>
#ifdef USE_JOLT_PHYSICS
#include <daBVH/dag_swBLAS_soa4.h> // soa4::ChunkRef for the cell-chunk Jolt path
#endif
#include <heightmap/heightmapHandler.h>

#include <gamePhys/collision/contactResultWrapper.h>

#include <util/dag_lookup.h>

#include <scene/dag_physMat.h>
#include <physMap/physMap.h>
#include <math/dag_shapeInertia.h>
#include <osApiWrappers/dag_miscApi.h>

#include "collisionGlobals.h"

static bool enable_apex = false;
static PhysWorld *phys_world = NULL;
static Tab<PhysBody *> frtObj(midmem);
static PhysBody *sphere_cast_shape = nullptr;
static PhysBody *box_cast_shape = nullptr;
static bool debugDrawerForced = false, debugDrawerInited = false;
static Ptr<CollisionResource> static_collres;
static Ptr<CollisionResource> water_collres;
static PhysBody *hmapObj = NULL;
static LandMeshManager *lmeshMgr = NULL;
static PhysMat::MatID lmesh_mat_id = -1;
static PhysMap *phys_map = NULL;
static CollisionObject sphere_collision;
static CollisionObject capsule_collision;
static CollisionObject box_collision;
static Tab<PhysBody *> lmeshObj(midmem);
static Tab<float> lmeshLastUsed(midmem);
static float lmeshRestitution = 0.f;
static float lmeshCurTime = 0.f;
static constexpr float LMESH_BODY_TTL = 5.0f;
static int numBorderCellsXPos = 2;
static int numBorderCellsXNeg = 2;
static int numBorderCellsZPos = 2;
static int numBorderCellsZNeg = 2;

static FFTWater *water = NULL;
static bool only_water2d = false;

static float collapse_contact_threshold = 0.f;

static void create_cast_shapes()
{
  PhysSphereCollision sphShape(1.f);
  PhysBoxCollision boxShape(1.f, 1.f, 1.f);
  PhysBodyCreationData pbcd;
  pbcd.addToWorld = false;
  sphere_cast_shape = new PhysBody(dacoll::get_phys_world(), 0.f, &sphShape, TMatrix::IDENT, pbcd);
  box_cast_shape = new PhysBody(dacoll::get_phys_world(), 0.f, &boxShape, TMatrix::IDENT, pbcd);
}

void dacoll::init_collision_world(dacoll::InitFlags flags, float collapse_contact_thr)
{
  term_collision_world();

  init_physics_engine((flags & InitFlags::SingleThreaded) != InitFlags::None);
  phys_world = new PhysWorld();
  const DataBlock &physBlk = *dgs_get_settings()->getBlockByNameEx("phys");
  phys_world->setMaxSubSteps(physBlk.getInt("maxSubSteps", 3));
  phys_world->setFixedTimeStep(1.f / physBlk.getInt("fixedTimeStepHz", 60));
#if defined(USE_BULLET_PHYSICS)
  phys_world->setMultithreadedUpdate(physBlk.getBool("multithreaded", true));
  phys_world->getScene()->getSolverInfo().m_erp2 = 0.8f;
  phys_world->getScene()->getSolverInfo().m_solverMode = SOLVER_SIMD | SOLVER_USE_WARMSTARTING;
  phys_world->getScene()->getDispatchInfo().m_allowedCcdPenetration = 0.01f;
  phys_world->getScene()->setForceUpdateAllAabbs((flags & InitFlags::ForceUpdateAABB) != InitFlags::None);
  gContactBreakingThreshold = collapse_contact_thr;
#else
  G_UNUSED(physBlk);
  G_UNUSED(flags);
#endif

  create_cast_shapes();
  collapse_contact_threshold = collapse_contact_thr;

#if DAGOR_DBGLEVEL > 0
  if ((flags & InitFlags::EnableDebugDraw) != InitFlags::None)
  {
    physdbg::init<PhysWorld>();
    debugDrawerInited = true;
  }
#endif
}

void dacoll::term_collision_world()
{
  del_it(sphere_cast_shape);
  del_it(box_cast_shape);
  del_it(phys_world);
  close_physics_engine();
#if DAGOR_DBGLEVEL > 0
  physdbg::term<PhysWorld>();
  debugDrawerInited = false;
#endif
}

void dacoll::clear_collision_world()
{
  fetch_sim_res(true);
  del_it(sphere_cast_shape);
  del_it(box_cast_shape);
#if defined(USE_BULLET_PHYSICS)
  free_all_bt_collobjects_memory();
#endif
  phys_world->clear();
  create_cast_shapes();
}

float dacoll::get_collision_object_collapse_threshold(const CollisionObject &co)
{
#if defined(USE_BULLET_PHYSICS)
  btCollisionShape *shape = co.body ? co.body->getActor()->getCollisionShape() : nullptr;
  return shape ? shape->getContactBreakingThreshold(collapse_contact_threshold) : 0.f;
#else
  G_UNUSED(co);
  return 0.f;
#endif
}

void dacoll::destroy_static_collision()
{
  // Bodies first: a Jolt chunk shape reads the resource's chunk memory.
  for (int i = 0; i < frtObj.size(); ++i)
    del_it(frtObj[i]);
  static_collres = nullptr;
  water_collres = nullptr;
  del_it(hmapObj);
  for (int i = 0; i < lmeshObj.size(); ++i)
    del_it(lmeshObj[i]);
  clear_and_shrink(lmeshObj);
  clear_and_shrink(lmeshLastUsed);
  lmeshMgr = NULL;
  clear_and_shrink(frtObj);
  clear_ri_instances();
  destroy_dynamic_collision(sphere_collision);
  sphere_collision.clear_ptrs();
  destroy_dynamic_collision(box_collision);
  box_collision.clear_ptrs();
  destroy_dynamic_collision(capsule_collision);
  capsule_collision.clear_ptrs();
  fetch_sim_res(true);
}

LandMeshManager *dacoll::get_lmesh() { return lmeshMgr; }

PhysMat::MatID dacoll::get_lmesh_mat_id() { return lmesh_mat_id; }

PhysMat::MatID dacoll::get_lmesh_mat_id_at_point(const Point2 &pos)
{
  if (phys_map)
  {
    PhysMat::MatID matId = phys_map->getMaterialAt(pos);
    if (PHYSMAT_INVALID != matId)
      return matId;
  }
  return lmesh_mat_id;
}

void dacoll::set_lmesh_phys_map(PhysMap *phys_map_)
{
  del_it(phys_map);
  phys_map = phys_map_;
}

void dacoll::set_lmesh_phys_map_ptr(PhysMap *phys_map_) { phys_map = phys_map_; }

const PhysMap *dacoll::get_lmesh_phys_map() { return phys_map; }


CollisionResource *dacoll::get_static_collision_resource() { return static_collres.get(); }

// Extent a contact solver can still resolve. A near-singular authored node tm scales the stored
// geometry by 1/det, and the float grid over such a box is coarser than the shapes that meet on it.
// Only the extent: a level's static collision sits far from the origin by design.
static constexpr float MAX_PHYS_NODE_EXTENT = 1e6f;
static bool is_node_phys_usable(const BBox3 &node_box) // a NaN bound fails the compare
{
  return v_check_xyz_all_true(v_cmp_le(v_bbox3_size(v_ldu_bbox3(node_box)), v_splats(MAX_PHYS_NODE_EXTENT)));
}

static void push_static_tri(Tab<Point3> &verts, Tab<int> &indices, vec3f a, vec3f b, vec3f c)
{
  const int base = verts.size();
  v_stu_p3(&verts.push_back().x, a);
  v_stu_p3(&verts.push_back().x, b);
  v_stu_p3(&verts.push_back().x, c);
  for (int i = 0; i < 3; ++i)
    indices.push_back(base + i);
}

bool dacoll::append_static_collision_mesh(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices)
{
  if (!static_collres)
    return false;
  static_collres->visitTrianglesInBox(v_ldu_bbox3(box), CollisionNode::PHYS_COLLIDABLE,
    [&](vec3f a, vec3f b, vec3f c, int, int) { //-V657 collect all
      push_static_tri(vertices, indices, a, b, c);
      return false;
    });
  return true;
}

CollisionResource *dacoll::get_water_collision() { return water_collres.get(); }
// Holds a reference of its own; the caller keeps whatever it has.
static void set_water_collision(CollisionResource *res) { water_collres = res; }

void dacoll::init_phys_materials()
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  if (physWorld)
  {
    for (int n = 0; n < PhysMat::physMatCount(); n++)
    {
      const PhysMat::MaterialData &pm = PhysMat::getMaterial(n);
      int materialId = physWorld->createNewMaterialId(pm.physStaticFriction, pm.physStaticFriction, pm.physRestitution, 1.0f);
      PhysMat::setPhysBodyMaterial(n, materialId);
    }
  }
}

static void add_static_collision_bodies(const CollisionResource &res, [[maybe_unused]] const char *res_name)
{
  using namespace dacoll;
  PhysBodyCreationData pbcd;
  pbcd.useMotionState = false;
  pbcd.materialId = -1;
  pbcd.friction = 1.f;
  pbcd.autoMask = false;
  pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);

  const dag::ConstSpan<CollisionNode> nodes = res.getAllNodes();
  dag::Vector<uint8_t> chunkBodied(nodes.size(), 0);
  bool allBodied = true;
#ifdef USE_JOLT_PHYSICS
  // One body for all chunks: the compound's own tree culls them, so a unit's contact test is one
  // pair test however many nodes the split made, not one per node.
  PhysCompoundCollision chunks;
#endif
  for (int i = 0, e = (int)nodes.size(); i < e; ++i)
  {
    if (!nodes[i].checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;
    const BBox3 nodeBox = res.getNodeBBox(i);
    if (!is_node_phys_usable(nodeBox))
    {
#if DAGOR_DBGLEVEL > 0 // per occurrence: the affected content must be enumerable
      logerr("static collision <%s>: node <%s> spans %@; no body for it, fix the asset", res_name, res.getNodeName(i), nodeBox);
#endif
      chunkBodied[i] = 1; // and its triangles stay out of the static trimesh below
      continue;
    }
#ifdef USE_JOLT_PHYSICS
    // compound_children 0 sizes the sub shape ids for a child per node, an upper bound of this compound
    if (PhysCollision *coll = create_phys_collision_from_coll_resource_node(res, i, /*compound_children*/ 0))
    {
      chunks.addChildCollision(coll, TMatrix::IDENT);
      chunkBodied[i] = 1;
      continue;
    }
#endif
    allBodied = false;
  }
#ifdef USE_JOLT_PHYSICS
  if (chunks.getChildrenCount())
    frtObj.push_back(new PhysBody(dacoll::get_phys_world(), 0.f, &chunks, TMatrix::IDENT, pbcd));
#endif
  if (allBodied)
    return;
  Tab<Point3> verts(tmpmem);
  Tab<int> indices(tmpmem);
  res.visitTrianglesInBox(res.vFullBBox, CollisionNode::PHYS_COLLIDABLE, [&](vec3f a, vec3f b, vec3f c, int, int node) { //-V657
    if (!chunkBodied[node])
      push_static_tri(verts, indices, a, b, c);
    return false;
  });
  if (indices.empty())
    return;
  PhysTriMeshCollision shape(verts, indices);
  shape.setDebugNamePtr("frt");
  frtObj.push_back(new PhysBody(dacoll::get_phys_world(), 0.f, &shape, TMatrix::IDENT, pbcd));
}

#if ENABLE_APEX == 1
static void add_static_collision_apex(const CollisionResource &res, const char *name)
{
  dag::Vector<Point3_vec4> verts;
  dag::Vector<unsigned> faces;
  unsigned vertCount = 0;
  for (const CollisionNode &n : res.getAllNodes())
    if (n.type == COLLISION_NODE_TYPE_MESH && n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      vertCount += (unsigned)res.getNodeVertCount(n.nodeIndex);
  verts.reserve(vertCount);
  faces.reserve(3 * (unsigned)res.getTrianglesCount(CollisionNode::PHYS_COLLIDABLE));
  for (const CollisionNode &n : res.getAllNodes())
  {
    if (n.type != COLLISION_NODE_TYPE_MESH || !n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;
    const unsigned base = (unsigned)verts.size();
    res.iterateNodeVerts(n.nodeIndex, [&](int, vec4f v) { v_st(&verts.push_back().x, v); });
    res.iterateNodeFaces(n.nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
      faces.push_back(base + i0);
      faces.push_back(base + i1);
      faces.push_back(base + i2);
    });
  }
  if (!faces.empty())
    physx_add_static_collision_mesh(make_span_const(verts), make_span_const(faces), res.boundingBox, name);
}
#endif

static void install_static_collision(CollisionResource *res, const char *name)
{
  static_collres = res;
  if (phys_world && phys_world->getScene())
    add_static_collision_bodies(*res, name);
#if ENABLE_APEX == 1
  if (enable_apex)
    add_static_collision_apex(*res, name);
#endif
}

bool dacoll::add_static_collision_frt(const StaticSceneRayTracer *frt, const char *name, dag::ConstSpan<unsigned char> *pmid)
{
  if (!frt || frt->getFacesCount() <= 0)
    return true;
  CollisionResource *res = build_static_collision_frt(*frt, pmid ? *pmid : dag::ConstSpan<unsigned char>(), name);
  if (!res)
    return false;
  install_static_collision(res, name);
  return true;
}

bool dacoll::load_static_collision(IGenLoad &crd)
{
  // No resolver: PhysMat's own answer lands a name the table lacks as the default, and the cook
  // already warns for it by name.
  Ptr<CollisionResource> res = new CollisionResource(crd, -1, crd.getTargetName());
  if (res->getAllNodes().empty())
  {
    logerr("static collision: the stream in %s landed empty", crd.getTargetName());
    return false;
  }
  debug("static collision: %d nodes, %d faces from %s", (int)res->getAllNodes().size(),
    (int)res->getTrianglesCount(CollisionNode::PHYS_COLLIDABLE), crd.getTargetName());
  install_static_collision(res, crd.getTargetName());
  return true;
}

bool dacoll::load_water_collision(IGenLoad &crd)
{
  Ptr<CollisionResource> res = new CollisionResource(crd, -1, crd.getTargetName());
  if (res->getAllNodes().empty())
  {
    logerr("water: the stream in %s landed empty", crd.getTargetName());
    return false;
  }
  debug("water: %d nodes, %d faces from %s", (int)res->getAllNodes().size(), (int)res->getTrianglesCount(CollisionNode::TRACEABLE),
    crd.getTargetName());
  set_water_collision(res.get());
  return true;
}

bool dacoll::load_static_collision_frt(IGenLoad *crd)
{
  CollisionResource *res = nullptr;
  if (!load_frt_block_as_collision(*crd, res))
    return false;
  // A block that builds nothing leaves the level without static collision: the stream read, so the
  // level is not refused over it.
  if (res)
    install_static_collision(res, crd->getTargetName());
  return true;
}

bool dacoll::make_water_collision(dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices)
{
  set_water_collision(nullptr);
  if (indices.empty())
    return false;
  static const uint16_t classes[] = {CollisionNode::TRACEABLE};
  CollisionResourceBuilder builder;
  const int parts = builder.addSplitMeshNodes(make_span_const(classes, 1), verts, indices, {});
  if (parts <= 0)
  {
    logerr("water: %d faces gave no collision (%d parts)", (int)indices.size() / 3, parts);
    return false;
  }
  builder.recomputeBounds();
  builder.collapse("water");
  builder.collisionFlags |= COLLISION_RES_FLAG_BLAS_TWO_SIDED; // the water walks cull nothing
  CollisionResource *res = builder.build("water");
  debug("water: %d faces -> %d parts -> %d nodes", (int)indices.size() / 3, parts, (int)res->getAllNodes().size());
  set_water_collision(res);
  return true;
}

int dacoll::get_hmap_step() { return hmapObj ? phys_body_get_hmap_step(hmapObj) : -1; }

int dacoll::set_hmap_step(int step) { return hmapObj ? phys_body_set_hmap_step(hmapObj, step) : -1; }

void dacoll::add_collision_hmap(LandMeshManager *land, float restitution, float margin)
{
  if (!phys_world || !phys_world->getScene() || !land || !land->getHmapHandler())
  {
    hmapObj = NULL;
    return;
  }

#if ENABLE_APEX == 1
  if (enable_apex)
    physx_add_static_collision_heightmap(land);
#endif

  auto hmap = land->getHmapHandler();
  const float hmapScale = hmap->getHeightmapCellSize() > 1 ? 1.f : 2.f; // scale heightmap according to hmap cell size
  PhysHeightfieldCollision coll(hmap, Point2::xz(hmap->getHeightmapOffset()), hmap->getWorldBox(), 1.f, hmapScale,
    land->getHolesManager());
  coll.setMargin(margin);
  PhysBodyCreationData pbcd;
  pbcd.useMotionState = false;
  pbcd.materialId = -1;
  pbcd.friction = 1.f;
  pbcd.restitution = restitution;
  pbcd.autoMask = false;
  pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);
  hmapObj = new PhysBody(dacoll::get_phys_world(), 0.f, &coll, TMatrix::IDENT, pbcd);
}

static PhysBody *create_lmesh_body(int idx)
{
  using namespace dacoll;
  auto *lray = lmeshMgr ? lmeshMgr->getLandTracer() : nullptr;
  if (!lray || idx < 0 || idx >= lray->getCellCount() || !lray->getCellTriCount(idx))
    return nullptr;

  TIME_PROFILE(create_lmesh_body);
  PhysBodyCreationData pbcd;
  pbcd.useMotionState = false;
  pbcd.materialId = -1;
  pbcd.friction = 1.f;
  pbcd.restitution = lmeshRestitution;
  pbcd.autoMask = false;
  pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);

#ifdef USE_JOLT_PHYSICS
  // The cell's BLAS is the Jolt shape itself: no MeshShape build, no copy of the geometry. The body
  // dies in destroy_static_collision, before the tracer that owns the chunk.
  soa4::ChunkRef chunk;
  if (lray->getCellChunk(idx, chunk))
    if (PhysCollision *coll = create_phys_collision_from_blas_chunk(chunk))
    {
      PhysBody *body = new PhysBody(dacoll::get_phys_world(), 0.f, coll, TMatrix::IDENT, pbcd);
      PhysCollision::clearAllocatedData(*coll);
      delete coll;
      return body;
    }
#endif
  // cook from the enumerated world-space geometry (transient: the phys engine copies on creation)
  Tab<Point3> verts(tmpmem);
  verts.reserve(lray->getCellVertCount(idx));
  lray->iterateCellVertices(idx, [&](const Point3 &p) { verts.push_back(p); });
  Tab<int> indices(tmpmem);
  indices.reserve(lray->getCellTriCount(idx) * 3);
  lray->iterateCellFaces(idx, [&](int v0, int v1, int v2) {
    indices.push_back(v0);
    indices.push_back(v1);
    indices.push_back(v2);
  });

  PhysTriMeshCollision shape(verts, indices);
  char debugShapeName[] = "lmesh_????";
#if DAGOR_DBGLEVEL > 0 || _TARGET_PC
  snprintf(debugShapeName + sizeof("lmesh_") - 1, sizeof("????"), "%04d", idx);
#endif
  shape.setDebugNamePtr(debugShapeName);
  return new PhysBody(dacoll::get_phys_world(), 0.f, &shape, TMatrix::IDENT, pbcd);
}

static PhysBody *ensure_lmesh_body(int idx)
{
  if (!lmeshObj[idx])
    lmeshObj[idx] = create_lmesh_body(idx);
  if (lmeshObj[idx])
    lmeshLastUsed[idx] = lmeshCurTime;
  return lmeshObj[idx];
}

void dacoll::update_lmesh_phys_bodies(float dt)
{
  lmeshCurTime += dt;
  for (int i = 0; i < lmeshObj.size(); ++i)
  {
    if (lmeshObj[i] && (lmeshCurTime - lmeshLastUsed[i]) > LMESH_BODY_TTL)
      del_it(lmeshObj[i]);
  }
}

void dacoll::add_collision_landmesh(LandMeshManager *land, const char *name, float restitution)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  if (!physWorld || !physWorld->getScene() || !land)
    return;

  lmesh_mat_id = PhysMat::physMatCount() > 0 ? PhysMat::getMaterialId("horLandMesh") : PHYSMAT_INVALID;
  lmeshMgr = land;

  auto *lray = land->getLandTracer();
  if (!lray)
    return;

#if ENABLE_APEX == 1
  if (enable_apex)
    physx_add_static_collision_landmesh(land, name);
#else
  (void)name;
#endif

  lmeshRestitution = restitution;
  lmeshCurTime = 0.f;
  lmeshObj.resize(lray->getCellCount());
  lmeshLastUsed.resize(lray->getCellCount());
  mem_set_0(lmeshObj);
  mem_set_0(lmeshLastUsed);
  debug("add_collision_landmesh: %d cells (lazy init)", lray->getCellCount());
}

void dacoll::set_landmesh_mirroring(int cells_x_pos, int cells_x_neg, int cells_z_pos, int cells_z_neg)
{
  numBorderCellsXPos = cells_x_pos;
  numBorderCellsXNeg = cells_x_neg;
  numBorderCellsZPos = cells_z_pos;
  numBorderCellsZNeg = cells_z_neg;
}

void dacoll::set_water_source(FFTWater *in_water, bool has_only_water2d)
{
  water = in_water;
  only_water2d = has_only_water2d;
}

FFTWater *dacoll::get_water() { return water; }

bool dacoll::has_only_water2d() { return only_water2d; }

void dacoll::get_landmesh_mirroring(int &cells_x_pos, int &cells_x_neg, int &cells_z_pos, int &cells_z_neg)
{
  cells_x_pos = numBorderCellsXPos;
  cells_x_neg = numBorderCellsXNeg;
  cells_z_pos = numBorderCellsZPos;
  cells_z_neg = numBorderCellsZNeg;
}

void dacoll::set_enable_apex(bool flag) { enable_apex = flag; }

bool dacoll::is_apex_enabled() { return enable_apex; }

CollisionObject dacoll::create_coll_obj_from_shape(const PhysCollision &shape, void *userPtr, bool kinematic, bool add_to_world,
  bool auto_mask, int group_filter, int mask, const TMatrix *wtm, int physMatId)
{
  PhysBodyCreationData pbcd;
  pbcd.materialId = physMatId;
  pbcd.useMotionState = false;
  pbcd.addToWorld = add_to_world;
  pbcd.autoMask = auto_mask;
  if (!auto_mask)
    pbcd.group = group_filter, pbcd.mask = mask;
  pbcd.kinematic = kinematic;
  pbcd.userPtr = userPtr;
  pbcd.allowFastInaccurateCollTm = false;
  PhysBody *obj = new PhysBody(dacoll::get_phys_world(), 0.f, &shape, (wtm ? *wtm : TMatrix::IDENT), pbcd);
  if (DAGOR_UNLIKELY(!obj->isValid())) // Internal alloc failure?
    del_it(obj);
  return CollisionObject(obj, NULL);
}

CollisionObject dacoll::add_dynamic_cylinder_collision(const TMatrix &tm, float rad, float ht, void *user_ptr, bool add_to_world)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  if (!physWorld || !physWorld->getScene())
    return CollisionObject();

  PhysCompoundCollision shape;
  shape.addChildCollision(new PhysCylinderCollision(rad, ht, 1 /*Y*/), tm);
  return dacoll::create_coll_obj_from_shape(shape, user_ptr, /*kinematic*/ true, add_to_world, /* auto mask */ !add_to_world);
}

CollisionObject dacoll::add_dynamic_sphere_collision(const TMatrix &tm, float rad, void *user_ptr, bool add_to_world)
{
  DataBlock collBlk;
  DataBlock *sphBlk = collBlk.addNewBlock("sphere");
  sphBlk->addTm("tm", tm);
  sphBlk->addReal("rad", rad);
  return add_dynamic_collision(collBlk, user_ptr, /*is_player*/ false, add_to_world);
}

CollisionObject dacoll::add_dynamic_capsule_collision(const TMatrix &tm, float rad, float height, void *user_ptr, bool add_to_world)
{
  DataBlock collBlk;
  DataBlock *capBlk = collBlk.addNewBlock("capsule");
  capBlk->addTm("tm", tm);
  capBlk->addReal("rad", rad);
  capBlk->addReal("height", height);
  return add_dynamic_collision(collBlk, user_ptr, /*is_player*/ false, add_to_world);
}

CollisionObject dacoll::add_dynamic_box_collision(const TMatrix &tm, const Point3 &width, void *user_ptr, bool add_to_world)
{
  DataBlock collBlk;
  DataBlock *capBlk = collBlk.addNewBlock("box");
  capBlk->addTm("tm", tm);
  capBlk->addPoint3("width", width);
  return add_dynamic_collision(collBlk, user_ptr, /*is_player*/ false, add_to_world);
}

CollisionObject dacoll::add_dynamic_collision_with_mask(const DataBlock &props, void *userPtr, bool is_player, bool add_to_world,
  bool auto_mask, int mask, const TMatrix *wtm)
{
  if (!phys_world || !phys_world->getScene())
    return CollisionObject();

#if ENABLE_APEX == 1
  physx::PxRigidActor *pxObj = NULL;
  if (enable_apex && add_to_world)
    pxObj = physx_add_dynamic_collision(props, is_player);
#else
  (void)is_player;
#endif

  PhysCompoundCollision compShape;
  PhysCollision *shape = (props.blockCount() == 1) ? nullptr : &compShape;
  auto addChildShape = [&shape, &compShape](const TMatrix &tm, PhysCollision *child_shape) {
    if (!shape)
    {
      if (memcmp(&tm, &TMatrix::IDENT, sizeof(TMatrix)) == 0) //-V1014
      {
        shape = child_shape;
        return false;
      }
      shape = &compShape;
    }
    compShape.addChildCollision(child_shape, tm);
    return true;
  };

  int cylinderNid = props.getNameId("cylinder");
  int boxNid = props.getNameId("box");
  int sphereNid = props.getNameId("sphere");
  int capsuleNid = props.getNameId("capsule");
  for (int i = 0, c = props.blockCount(); i < c; ++i)
  {
    const DataBlock *blk = props.getBlock(i);
    int blkNid = blk->getBlockNameId();
    if (blkNid == boxNid)
    {
      const TMatrix &tm = blk->getTm("tm", TMatrix::IDENT);
      Point3 width = blk->getPoint3("width", Point3(0.f, 0.f, 0.f));
      if (!addChildShape(tm, new PhysBoxCollision(width.x, width.y, width.z)))
        break;
    }
    else if (blkNid == cylinderNid)
    {
      const TMatrix &tm = blk->getTm("tm", TMatrix::IDENT);
      Point3 width = blk->getPoint3("width", Point3(0.f, 0.f, 0.f));
      if (!addChildShape(tm, new PhysCylinderCollision(width.x, width.y * 2, 1 /*Y*/)))
        break;
    }
    else if (blkNid == sphereNid)
    {
      const TMatrix &tm = blk->getTm("tm", TMatrix::IDENT);
      float rad = blk->getReal("rad", 0.f);
      if (!addChildShape(tm, new PhysSphereCollision(rad)))
        break;
    }
    else if (blkNid == capsuleNid)
    {
      TMatrix tm = blk->getTm("tm", TMatrix::IDENT);
      float rad = blk->getReal("rad", 0.f);
      float hgt = blk->getReal("height", 0.f);
      if (!addChildShape(tm, new PhysCapsuleCollision(rad, hgt + rad * 2, 1 /*Y*/)))
        break;
    }
    else
      G_ASSERTF(false, "unsupported coll shape %s", blk->getBlockName());
  }

  G_ASSERT_RETURN(shape && (shape != &compShape || compShape.getChildrenCount()), {});

  CollisionObject co =
    create_coll_obj_from_shape(*shape, userPtr, /*kinematic*/ true, add_to_world, auto_mask, EPL_KINEMATIC, mask, wtm);
#if ENABLE_APEX
  if (enable_apex && add_to_world)
    co.physxObject = pxObj;
#endif
  if (shape != &compShape)
  {
    PhysCollision::clearAllocatedData(*shape);
    delete shape;
  }

  return co;
}

enum DynColShapeType
{
  CST_UNDEFINED = -1,
  CST_BOX = 0,
  CST_MESH,
  CST_CONVEX_HULL,
  CST_SPHERE,
  CST_NUM
};

static const char *dynCollShapeTypeNames[CST_NUM] = {"box", "mesh", "convex_hull", "sphere"};

// Materialises one collision node's geometry into the caller-provided per-child buffers: vertices for
// MESH / CONVEX_HULL nodes (and for any node when out_center is requested, to derive the centroid), plus
// -- for MESH nodes -- the face list recovered via iterateNodeFaces (faces live in the BLAS at runtime;
// the resource keeps no index list and Jolt asserts on a zero-face mesh). When out_center is non-null it
// receives the node centroid and the verts are recentred about it and
// scaled. The buffers back the spans handed to create_shape_from_collision_node and must outlive the shape.
static void append_collision_node_geometry(const CollisionResource *coll_res, const CollisionNode *node, float scale,
  DynColShapeType node_type, Point3 *out_center, Tab<Point3_vec4> &vertices_stor, Tab<uint32_t> &indices_stor)
{
  if (node_type == CST_MESH || node_type == CST_CONVEX_HULL || out_center)
  {
    const int base = vertices_stor.size();
    const int count = coll_res->getNodeVertCount(node->nodeIndex);
    vertices_stor.resize(base + count);
    coll_res->iterateNodeVerts(node->nodeIndex, [&](int i, vec4f v) { v_st(&vertices_stor[base + i].x, v); });

    if (out_center)
    {
      out_center->zero();
      // A degenerate-dropped node has 0 verts; skip the centroid (1/count) and recenter. create_shape_from_
      // collision_node then rejects the empty span, so the caller drops this child.
      if (count > 0)
      {
        for (int i = 0; i < count; i++)
          (*out_center) += vertices_stor[base + i];
        (*out_center) *= 1.f / (float)count;

        TMatrix tm = TMatrix::IDENT;
        tm.setcol(3, -(*out_center));
        (*out_center) = tm.getcol(3);

        for (int i = 0; i < count; i++)
          vertices_stor[base + i] = scale * tm * vertices_stor[base + i];
      }
    }
  }

  if (node_type == CST_MESH)
  {
    // Faces live in the BLAS at runtime (the resource keeps no index list): iterateNodeFaces walks the per-node
    // chunk tree for non-resident nodes, the grid for residents. Its sub-triangle count matches
    // getNodeFaceCount() only by a build-time invariant, not at this call site -- reserve + push_back so
    // a miscount grows the buffer instead of overrunning it with indexed writes.
    indices_stor.reserve(indices_stor.size() + coll_res->getNodeFaceCount(node->nodeIndex) * 3);
    coll_res->iterateNodeFaces(node->nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
      indices_stor.push_back(i0);
      indices_stor.push_back(i1);
      indices_stor.push_back(i2);
    });
  }
}

// Builds the physics child shape from geometry already materialised by append_collision_node_geometry().
// vertices / indices are spans into the caller's per-child buffers; PhysTriMeshCollision /
// PhysConvexHullCollision retain the raw data pointer until PhysBody construction, so those buffers must
// outlive the compound build. BOX / SPHERE ignore the spans and read their parameters straight from the node.
static PhysCollision *create_shape_from_collision_node(const CollisionResource *coll_res, const CollisionNode *node,
  DynColShapeType node_type, float scale, dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint32_t> indices)
{
  switch (node_type)
  {
    case CST_BOX:
    {
      // A node dropped as degenerate at load has no collision surface, but its bbox/bsphere keep
      // their stale pre-drop values -- an override would add a phantom collider at the old bounds.
      // Skip it like the mesh/convex cases below.
      if (coll_res->getNodeVertCount(node->nodeIndex) == 0)
        return nullptr;
      Point3 width = coll_res->getNodeBBox(node->nodeIndex).width();
      return new PhysBoxCollision(width.x, width.y, width.z);
    }
    case CST_MESH:
      // A node dropped as degenerate at load yields no faces; skip it (caller treats null as "no
      // child") so Jolt is never handed an empty MeshShapeSettings.
      if (indices.empty())
        return nullptr;
      G_ASSERTF_RETURN(indices.size() / 3 < 300 * 1024, nullptr,
        "Too much triangles in mesh: %d verts and %d faces! 300k is too much already!", vertices.size(), indices.size() / 3);
      return new PhysTriMeshCollision(vertices, indices, nullptr, false, false /*reverse normals*/);
    case CST_CONVEX_HULL:
      // A node dropped as degenerate at load yields no verts; skip it (Jolt derefs point 0 on an empty hull).
      if (vertices.empty())
        return nullptr;
      return new PhysConvexHullCollision(vertices, false);
    case CST_SPHERE:
      // Same stale-bounds hazard as CST_BOX: a dropped node's bounding sphere keeps its pre-drop radius.
      if (coll_res->getNodeVertCount(node->nodeIndex) == 0)
        return nullptr;
      return new PhysSphereCollision(scale * coll_res->getNodeBSphere(node->nodeIndex).r);
    default: return nullptr;
  }
}

CollisionObject dacoll::add_simple_dynamic_collision_from_coll_resource(const DataBlock &props, const CollisionResource *resource,
  GeomNodeTree *tree, float margin, float scale, Point3 &out_center, TMatrix &out_tm, TMatrix &out_tm_in_model, bool add_to_world)
{
  if (!phys_world || !phys_world->getScene() || !resource)
    return CollisionObject();

  PhysCompoundCollision shape;
  shape.setMargin(margin);

  // PhysTriMeshCollision / PhysConvexHullCollision keep the raw vert/idx pointer they are handed until
  // create_coll_obj_from_shape() (below) builds the PhysBody, so every child needs a buffer that survives
  // the whole compound build -- a single reused buffer would leave earlier children pointing at a span the
  // next child overwrote. Each per-child Tab keeps its own heap storage as the outer Tab grows. Mirrors
  // add_dynamic_collision_from_coll_resource.
  Tab<Tab<Point3_vec4>> vertices_stor(framemem_ptr());
  Tab<Tab<uint32_t>> indices_stor(framemem_ptr());

  const CollisionNode *rootNode = nullptr;
  TMatrix rootTm = TMatrix::IDENT;

  for (int i = 0; i < props.paramCount(); ++i)
  {
    const char *shapeType = props.getStr(i);
    if (!shapeType)
      continue;

    const char *nodeName = props.getParamName(i);
    resource->forEachMeshNode([&](const CollisionNode &meshNode) -> bool {
      if (!resource->checkNodeBehaviorFlags(meshNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        return false;

      if (::strcmp(nodeName, resource->getNodeName(meshNode.nodeIndex)) != 0)
        return false;

      TMatrix nodeTm;
      resource->getCollisionNodeTm(&meshNode, TMatrix::IDENT, tree, nodeTm);

      TMatrix childTm = nodeTm * inverse(rootTm);

      if (!rootNode)
        childTm.setcol(3, ZERO<Point3>());

      const DynColShapeType nodeType =
        DynColShapeType(lup(shapeType, dynCollShapeTypeNames, countof(dynCollShapeTypeNames), CST_MESH));

      Tab<Point3_vec4> &childVerts = vertices_stor.push_back();
      dag::set_allocator(childVerts, dag::get_allocator(vertices_stor));
      Tab<uint32_t> &childIdx = indices_stor.push_back();
      dag::set_allocator(childIdx, dag::get_allocator(indices_stor));

      Point3 outCenter = ZERO<Point3>();
      append_collision_node_geometry(resource, &meshNode, scale, nodeType, rootNode ? nullptr : &outCenter, childVerts, childIdx);

      if (nodeType == CST_BOX)
        childTm.setcol(3, childTm * resource->getNodeBBox(meshNode.nodeIndex).center());

      // MESH faces were materialised into childIdx above (from the BLAS); non-mesh leaves it empty.
      dag::ConstSpan<uint32_t> idxSpan = make_span_const(childIdx);
      PhysCollision *childShape =
        create_shape_from_collision_node(resource, &meshNode, nodeType, scale, make_span_const(childVerts), idxSpan);
      if (!childShape)
        return false;

      childShape->setMargin(margin);
      shape.addChildCollision(childShape, childTm);

      if (!rootNode)
      {
        rootNode = &meshNode;
        rootTm = nodeTm;
        rootTm.setcol(3, rootTm.getcol(3) - childTm * outCenter);
        out_center = childTm * outCenter;
        out_tm_in_model = nodeTm;
        out_tm = nodeTm;
        out_tm.setcol(3, childTm * outCenter);
      }

      return true; // stop after first match
    });
  }

  // All requested nodes were degenerate-dropped: no child shapes. Don't hand Jolt an empty compound
  // (same guard as the other dynamic-collision builders).
  if (!shape.getChildrenCount())
    return CollisionObject();

  return dacoll::create_coll_obj_from_shape(shape, nullptr, /*kinematic*/ true, /* add to world */ add_to_world,
    /* auto mask */ false);
}

void dacoll::build_dynamic_collision_from_coll_resource(const CollisionResource *coll_resource, PhysCompoundCollision *compound,
  PhysBodyProperties &out_properties)
{
  if (coll_resource->boundingBox.isempty())
    return;

  const Point3 boxWidth = coll_resource->boundingBox.width();
  const Point3 boxCenter = coll_resource->boundingBox.center();

  out_properties.momentOfInertia = calc_box_inertia(boxWidth);
  out_properties.centerOfMass = boxCenter;
  out_properties.volume = boxWidth.x * boxWidth.y * boxWidth.z;

  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, boxCenter);

  compound->addChildCollision(new PhysBoxCollision(boxWidth.x, boxWidth.y, boxWidth.z, true), tm);
}

CollisionObject dacoll::build_dynamic_collision_from_coll_resource(const CollisionResource *coll_resource, bool add_to_world,
  int phys_layer, int mask, PhysBodyProperties &out_properties)
{
  if (!phys_world || !phys_world->getScene() || !coll_resource)
    return CollisionObject();

  // Start with just an overal box from coll_resource
  PhysCompoundCollision shape;

  build_dynamic_collision_from_coll_resource(coll_resource, &shape, out_properties);

  // The builder above adds nothing for a resource with an empty bbox (same guard as the other
  // dynamic-collision builders here)
  if (!shape.getChildrenCount())
    return CollisionObject();

  // TODO: add per-node collision infer and build collision such as convex hull with convex hull computers
  return dacoll::create_coll_obj_from_shape(shape, nullptr, true, add_to_world, /* auto mask */ false, phys_layer, mask);
}

CollisionObject dacoll::add_dynamic_collision_from_coll_resource(const DataBlock *props, const CollisionResource *coll_resource,
  void *user_ptr, int flags, int phys_layer, int mask, const TMatrix *wtm, const char *debug_name, const TMatrix *ptr_inv_parent_tm)
{
  // Must match the set of node types this function actually attaches as child shapes (mesh, box,
  // sphere, capsule). A broader getAllNodes().empty() test would let point-only / fully-filtered
  // resources through and reach physx_add_dynamic_collision_from_coll_resource() below, which
  // allocates the rigid actor before any child shape is built; with shape.getChildrenCount()==0
  // this function later returns an empty CollisionObject and the actor leaks.
  if (!phys_world || !phys_world->getScene() || !coll_resource ||
      (coll_resource->meshNodes().empty() && coll_resource->boxNodes().empty() && coll_resource->sphereNodes().empty() &&
        coll_resource->capsuleNodes().empty()))
    return CollisionObject();

#define APPLY_PARENT_ITM(v) ptr_inv_parent_tm ? *ptr_inv_parent_tm *(v) : (v)

#if ENABLE_APEX == 1
  physx::PxRigidActor *pxObj = NULL;
  if (enable_apex && ((flags & ACO_APEX_ADD) || !(flags & ACO_APEX_IS_VEHICLE)))
    pxObj = physx_add_dynamic_collision_from_coll_resource(props, coll_resource, false, flags & ACO_ADD_TO_WORLD,
      flags & ACO_APEX_IS_PLAYER, flags & ACO_APEX_IS_VEHICLE);
#endif

  PhysCompoundCollision shape;
  bool collapseConvexes = coll_resource->getCollisionFlags() & COLLISION_RES_FLAG_COLLAPSE_CONVEXES;
  Tab<Tab<Point3>> vertices_stor(framemem_ptr());
  vertices_stor.reserve(16);
  Tab<Tab<Point3_vec4>> trimesh_verts_stor(framemem_ptr());
  // Parallel storage for the face lists materialised from each MESH node's BLAS (the resource keeps no
  // index list). PhysTriMeshCollision retains a raw pointer through PhysBody construction; framemem
  // outlives that.
  Tab<Tab<uint32_t>> trimesh_idx_stor(framemem_ptr());
  if (collapseConvexes)
  {
    // go through all convexes and collapse them into one
    bool haveNonConvex =
      !coll_resource->boxNodes().empty() || !coll_resource->sphereNodes().empty() || !coll_resource->capsuleNodes().empty();
    Tab<Point3> &convexVerts = vertices_stor.push_back();
    dag::set_allocator(convexVerts, dag::get_allocator(vertices_stor));
    coll_resource->forEachMeshNode([&](const CollisionNode &meshNode) {
      if (!coll_resource->checkNodeBehaviorFlags(meshNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        return;

      if (meshNode.type == COLLISION_NODE_TYPE_CONVEX)
      {
        convexVerts.reserve(coll_resource->getNodeVertCount(meshNode.nodeIndex));
        coll_resource->iterateNodeVerts(meshNode.nodeIndex, [&](int, vec4f v) {
          Point3_vec4 vert;
          v_st(&vert.x, v);
          convexVerts.push_back(APPLY_PARENT_ITM(coll_resource->getNodeTm(meshNode.nodeIndex) * vert));
        });
      }
      haveNonConvex |= meshNode.type != COLLISION_NODE_TYPE_CONVEX;
    });
    if (!convexVerts.empty())
    {
      if (!haveNonConvex)
      {
        CollisionObject result = dacoll::create_coll_obj_from_shape(PhysConvexHullCollision(convexVerts, true), user_ptr,
          flags & ACO_KINEMATIC, flags & ACO_ADD_TO_WORLD, /* auto mask */ false, phys_layer, mask);
#if ENABLE_APEX
        if (enable_apex)
          result.physxObject = pxObj;
#endif

        return result;
      }
      shape.addChildCollision(new PhysConvexHullCollision(convexVerts, true), TMatrix::IDENT);
    }
  }

#if DAGOR_DBGLEVEL > 0
  dag::RelocatableFixedVector<String, 4, true, framemem_allocator> meshDebugNames;
#endif
#ifdef USE_JOLT_PHYSICS
  // the chunk shapes size their sub shape ids for every child the compound may hold: each child
  // source below counts here. A filtered-out node leaves slack that costs id bits only across a
  // power of two; the descent decode it can force benches at contact parity (joltMeshBench).
  unsigned compoundChildren = (unsigned)coll_resource->getAllNodes().size();
  if (props)
    compoundChildren +=
      props->getBlockByNameEx("customSphereCollisions")->paramCount() + props->getBlockByNameEx("customBoxCollisions")->blockCount();
#endif
  coll_resource->forEachMeshNode([&](const CollisionNode &meshNode) {
    if (!coll_resource->checkNodeBehaviorFlags(meshNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
      return;

    // the chunk shape, the trimesh and the boxify below all build over these bounds
    const BBox3 nodeBox = coll_resource->getNodeBBox(meshNode.nodeIndex);
    if (!is_node_phys_usable(nodeBox))
    {
#if DAGOR_DBGLEVEL > 0 // per occurrence: the affected content must be enumerable
      logerr("collision <%s>: node <%s> spans %@; no physics shape for it, fix the asset", debug_name ? debug_name : "?",
        coll_resource->getNodeName(meshNode.nodeIndex), nodeBox);
#endif
      return;
    }

    const char *nodeTypeName = props ? props->getStr(coll_resource->getNodeName(meshNode.nodeIndex), NULL) : NULL;
    if (props && !nodeTypeName)
      return;

    if (collapseConvexes && meshNode.type == COLLISION_NODE_TYPE_CONVEX)
      return;

    DynColShapeType nodeType =
      (flags & ACO_BOXIFY)
        ? CST_BOX
        : ((meshNode.type == COLLISION_NODE_TYPE_CONVEX || (flags & ACO_FORCE_CONVEX_HULL)) ? CST_CONVEX_HULL : CST_MESH);
    if (nodeTypeName)
      nodeType = DynColShapeType(lup(nodeTypeName, dynCollShapeTypeNames, countof(dynCollShapeTypeNames), nodeType));
    switch (nodeType)
    {
      case CST_BOX:
      {
        // A degenerate-dropped node (no verts) has a degenerate bbox; boxifying it would hand Jolt a
        // zero-extent shape. Skip it, matching the indicesCount==0 guard on the mesh path below.
        if (coll_resource->getNodeVertCount(meshNode.nodeIndex) == 0)
          break;
        TMatrix tm = coll_resource->getNodeTm(meshNode.nodeIndex);
        tm.setcol(3, tm * nodeBox.center());
        Point3 width = nodeBox.width();
        shape.addChildCollision(new PhysBoxCollision(width.x, width.y, width.z), APPLY_PARENT_ITM(tm));
      }
      break;

      case CST_MESH:
      {
#ifdef USE_JOLT_PHYSICS
        // The node's BLAS chunk is the Jolt shape itself: no MeshShape build, no copy of the geometry.
        // A node without a chunk (degenerate: no faces) takes the trimesh path below.
        if (PhysCollision *chunk = create_phys_collision_from_coll_resource_node(*coll_resource, meshNode.nodeIndex, compoundChildren))
        {
          shape.addChildCollision(chunk, APPLY_PARENT_ITM(coll_resource->getNodeTm(meshNode.nodeIndex)));
          break;
        }
#endif
        // Owning resources keep verts vert21-packed in the node's BLAS chunk, so decode them back to
        // Point3_vec4 before handing them to PhysTriMeshCollision (the physics engine reads vdata via
        // a stride). Stash the materialised
        // buffer in trimesh_verts_stor (framemem, same scope as `shape`) so the pointer stays valid
        // until PhysBody construction finishes -- the engine copies verts into its own structures
        // during that, after which trimesh_verts_stor frees on framemem reset.
        Tab<Point3_vec4> &matVerts = trimesh_verts_stor.push_back();
        dag::set_allocator(matVerts, dag::get_allocator(trimesh_verts_stor));
        matVerts.reserve(coll_resource->getNodeVertCount(meshNode.nodeIndex));
        coll_resource->iterateNodeVerts(meshNode.nodeIndex, [&](int, vec4f v) {
          Point3_vec4 &dst = matVerts.push_back();
          v_st(&dst.x, v);
        });
        dag::ConstSpan<Point3_vec4> meshVerts = make_span_const(matVerts);

        // Owning resources keep no source-face list (faces live in the BLAS, not a kept index list). The
        // only "no geometry" case is indicesCount==0 (degenerate-dropped at load); skip that so Jolt is
        // never handed an empty MeshShapeSettings, and recover faces for the rest via iterateNodeFaces.
        if (coll_resource->getNodeIndexCount(meshNode.nodeIndex) == 0)
          break; // degenerate-dropped node: no collision surface
        // Recover face indices via iterateNodeFaces (walks the node's chunk tree, decoding node-local
        // indices [0, verticesCount) from each leaf).
        Tab<uint32_t> &matIdx = trimesh_idx_stor.push_back();
        dag::set_allocator(matIdx, dag::get_allocator(trimesh_idx_stor));
        matIdx.reserve(coll_resource->getNodeFaceCount(meshNode.nodeIndex) * 3);
        coll_resource->iterateNodeFaces(meshNode.nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
          matIdx.push_back(i0);
          matIdx.push_back(i1);
          matIdx.push_back(i2);
        });
        dag::ConstSpan<uint32_t> meshIdx = make_span_const(matIdx);
        G_ASSERTF_AND_DO(meshIdx.size() / 3 < 300 * 1024, break,
          "Too much triangles in mesh: %d verts and %d faces! 300k is too much already!", meshVerts.size(), meshIdx.size() / 3);
        auto trim = new PhysTriMeshCollision(meshVerts, meshIdx, nullptr, false, false /*reverse normals*/);
#if DAGOR_DBGLEVEL > 0
        meshDebugNames.emplace_back(framemem_ptr()).printf(0, "%s/%s", debug_name, coll_resource->getNodeName(meshNode.nodeIndex));
        trim->setDebugNamePtr(meshDebugNames.back().c_str());
#else
        trim->setDebugNamePtr(debug_name);
#endif
        shape.addChildCollision(trim, APPLY_PARENT_ITM(coll_resource->getNodeTm(meshNode.nodeIndex)));
      }
      break;

      case CST_CONVEX_HULL:
      {
        Tab<Point3> &vertices = vertices_stor.push_back();
        dag::set_allocator(vertices, dag::get_allocator(vertices_stor));
        coll_resource->iterateNodeVerts(meshNode.nodeIndex, [&](int, vec4f v) {
          Point3_vec4 vert;
          v_st(&vert.x, v);
          vertices.push_back(APPLY_PARENT_ITM(coll_resource->getNodeTm(meshNode.nodeIndex) * vert));
        });
        if (vertices.empty())
          break; // degenerate-dropped node: no hull surface (Jolt derefs point 0 on an empty hull)
        shape.addChildCollision(new PhysConvexHullCollision(vertices, false), TMatrix::IDENT);
      }
      break;

      default: break;
    };
  });

  coll_resource->forEachBoxNode([&](const CollisionNode &boxNode) {
    if (!coll_resource->checkNodeBehaviorFlags(boxNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
      return;
    if (props && !props->getBool(coll_resource->getNodeName(boxNode.nodeIndex), false))
      return;
    TMatrix tm = TMatrix::IDENT;
    BBox3 bbox = coll_resource->getNodeBBox(boxNode.nodeIndex);
    tm.setcol(3, bbox.center());
    Point3 width = bbox.width();
    shape.addChildCollision(new PhysBoxCollision(width.x, width.y, width.z), APPLY_PARENT_ITM(tm));
  });

  coll_resource->forEachSphereNode([&](const CollisionNode &sphereNode) {
    if (!coll_resource->checkNodeBehaviorFlags(sphereNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
      return;
    if (props && !props->getBool(coll_resource->getNodeName(sphereNode.nodeIndex), false))
      return;
    BSphere3 bsph = coll_resource->getNodeBSphere(sphereNode.nodeIndex);
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, bsph.c);
    shape.addChildCollision(new PhysSphereCollision(bsph.r), APPLY_PARENT_ITM(tm));
  });

  coll_resource->forEachCapsuleNode([&](const CollisionNode &capNode) {
    if (!coll_resource->checkNodeBehaviorFlags(capNode.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
      return;
    if (props && !props->getBool(coll_resource->getNodeName(capNode.nodeIndex), false))
      return;
    TMatrix tm = coll_resource->getNodeTm(capNode.nodeIndex);
    Point3 scale = Point3(length(tm.getcol(0)), length(tm.getcol(1)), length(tm.getcol(2)));
    BBox3 capBBox = coll_resource->getNodeBBox(capNode.nodeIndex);
    Point3 boxWidth = capBBox.width();
    boxWidth.x *= scale.x;
    boxWidth.y *= scale.y;
    boxWidth.z *= scale.z;
    float capHt = max(boxWidth.x, max(boxWidth.y, boxWidth.z));
    float capRad = min(boxWidth.x, min(boxWidth.y, boxWidth.z)) * 0.5f;
    tm.setcol(3, tm * capBBox.center());
    tm.orthonormalize();
    int ax = 0;
    if (boxWidth.x > boxWidth.y && boxWidth.x > boxWidth.z)
      ax = 0 /*X*/;
    else if (boxWidth.y > boxWidth.x && boxWidth.y > boxWidth.z)
      ax = 1 /*Y*/;
    else
      ax = 2 /*Z*/;
    shape.addChildCollision(new PhysCapsuleCollision(capRad, capHt + capRad * 2, ax), APPLY_PARENT_ITM(tm));
  });

  if (props)
  {
    if (const DataBlock *customCollPropsBlk = props->getBlockByNameEx("customSphereCollisions", nullptr))
    {
      for (int i = 0; i < customCollPropsBlk->paramCount(); ++i)
      {
        Point4 sph = customCollPropsBlk->getPoint4(i);
        TMatrix tm = TMatrix::IDENT;
        tm.setcol(3, Point3(sph.x, sph.y, sph.z));
        shape.addChildCollision(new PhysSphereCollision(sph.w), APPLY_PARENT_ITM(tm));
      }
    }
    if (const DataBlock *customCollPropsBlk = props->getBlockByNameEx("customBoxCollisions", nullptr))
    {
      for (int i = 0; i < customCollPropsBlk->blockCount(); ++i)
      {
        if (const DataBlock *boxBlk = customCollPropsBlk->getBlock(i))
        {
          G_ASSERTF(boxBlk->paramCount() == 2, "Custom Box Collision have incorrect corner points. Waiting: 2, passed: %d",
            boxBlk->paramCount());
          Point3 firstPoint = boxBlk->getPoint3(0);
          Point3 secondPoint = boxBlk->getPoint3(1);
          BBox3 box(min(firstPoint, secondPoint), max(firstPoint, secondPoint));
          TMatrix tm = TMatrix::IDENT;
          tm.setcol(3, box.center());
          shape.addChildCollision(new PhysBoxCollision(box.width().x, box.width().y, box.width().z), APPLY_PARENT_ITM(tm));
        }
      }
    }
  }

#undef APPLY_PARENT_ITM

  if (!shape.getChildrenCount())
    return CollisionObject();

  CollisionObject result = dacoll::create_coll_obj_from_shape(shape, user_ptr, flags & ACO_KINEMATIC, flags & ACO_ADD_TO_WORLD,
    /* auto mask */ false, phys_layer, mask, wtm);
#if ENABLE_APEX
  if (enable_apex)
    result.physxObject = pxObj;
#endif

  return result;
}

CollisionObject dacoll::add_dynamic_collision_convex_from_coll_resource(const CollisionResource *coll_res,
  dag::ConstSpan<const CollisionNode *> coll_nodes, const Point3 &offset, void *user_ptr, int node_type_flags, bool kinematic,
  bool add_to_world, int phys_layer, int mask, const TMatrix *wtm)
{
  // go through all convexes and collapse them into one
  Tab<Point3_vec4> convexVerts(framemem_ptr());
  for (int i = 0; i < coll_nodes.size(); ++i)
  {
    const CollisionNode &node = *coll_nodes[i];
    if (((1 << node.type) & node_type_flags) == 0)
      continue;
    const int prevCount = convexVerts.size();
    switch (node.type)
    {
      case COLLISION_NODE_TYPE_MESH:
      {
        mat44f nodeTm;
        v_mat44_make_from_43cu(nodeTm, coll_res->getNodeTm(node.nodeIndex)[0]);
        vec4f vOffset = v_ldu(&offset.x);
        const int vertCount = coll_res->getNodeVertCount(node.nodeIndex);
        convexVerts.resize(prevCount + vertCount);
        coll_res->iterateNodeVerts(node.nodeIndex,
          [&](int vi, vec4f v) { v_st(&convexVerts[prevCount + vi].x, v_add(v_mat44_mul_vec3p(nodeTm, v), vOffset)); });
        break;
      }
      case COLLISION_NODE_TYPE_BOX:
      case COLLISION_NODE_TYPE_SPHERE:
      {
        // getNodeBBox already carries the box/sphere placement; multiplying getNodeTm in
        // would double-apply it
        BBox3 bbox = coll_res->getNodeBBox(node.nodeIndex);
        convexVerts.resize(prevCount + 8);
        for (int i = 0; i < 8; ++i)
          convexVerts[prevCount + i] = bbox.point(i) + offset;
        break;
      }
      case COLLISION_NODE_TYPE_CAPSULE:
      case COLLISION_NODE_TYPE_CONVEX:
      case COLLISION_NODE_TYPE_POINTS:
      default:
        convexVerts.push_back(coll_res->getNodeTm(node.nodeIndex).getcol(3) + offset);
        break;
        ;
    }
  }
  if (convexVerts.size() >= 3) // can't create convex with less then 3 vertexes
  {
    static constexpr float hullTolerance = 2.5e-3f; // Empirically this seems to be enough
    PhysConvexHullCollision convexShape(convexVerts, /*build*/ true, hullTolerance);
    static constexpr bool auto_mask = false;
    return create_coll_obj_from_shape(convexShape, user_ptr, kinematic, add_to_world, auto_mask, phys_layer, mask, wtm);
  }
  else
    return CollisionObject();
}

void dacoll::add_object_to_world(CollisionObject co)
{
  if (!phys_world || !phys_world->getScene())
    return;
  phys_world->fetchSimRes(true);
  phys_world->addBody(co.body, true);
}

void dacoll::remove_object_from_world(CollisionObject co)
{
  if (!phys_world || !phys_world->getScene() || !co)
    return;
  phys_world->fetchSimRes(true);
  phys_world->removeBody(co.body);
}

void dacoll::destroy_dynamic_collision(CollisionObject co)
{
#if ENABLE_APEX == 1
  if (co.physxObject && enable_apex)
  {
    physx_destroy_actor(co.physxObject);
    co.physxObject = NULL;
  }
#endif

  del_it(co.body);
}

void dacoll::fetch_sim_res(bool wait)
{
  if (phys_world)
    phys_world->fetchSimRes(wait);
}


PhysWorld *dacoll::get_phys_world() { return phys_world; }

void dacoll::phys_world_start_sim(float dt, bool wake)
{
  if (phys_world)
    phys_world->startSim(dt, wake);
}

void dacoll::phys_world_set_invalid_fetch_sim_res_thread(int64_t tid)
{
#if defined(USE_BULLET_PHYSICS)
  if (phys_world)
    phys_world->setInvalidFetchSimResThread(tid);
#else
  G_UNUSED(tid); //==
#endif
}

void dacoll::phys_world_set_control_thread_id(int64_t tid)
{
#if defined(USE_BULLET_PHYSICS)
  if (phys_world)
    phys_world->setControlThreadId(tid);
#else
  G_UNUSED(tid); //==
#endif
}

CollisionObject &dacoll::get_reusable_sphere_collision()
{
  // This is a reusable, so simultanious access would break things.
  // is_main_thread() is a hacky way to ensure single-threadedness.
  G_ASSERT(is_main_thread());
  if (!sphere_collision.isValid())
    sphere_collision = add_dynamic_sphere_collision(TMatrix::IDENT, 1.f, /*user_ptr*/ nullptr, /*add_to_world*/ false);
  return sphere_collision;
}

CollisionObject &dacoll::get_reusable_capsule_collision()
{
  // This is a reusable, so simultanious access would break things.
  // is_main_thread() is a hacky way to ensure single-threadedness.
  G_ASSERT(is_main_thread());
  if (!capsule_collision.isValid())
    capsule_collision =
      add_dynamic_capsule_collision(TMatrix::IDENT, /*radius*/ 1.f, /*heigh*/ 1.f, /*user_ptr*/ nullptr, /*add_to_world*/ false);
  return capsule_collision;
}

CollisionObject &dacoll::get_reusable_box_collision()
{
  // This is a reusable, so simultanious access would break things.
  // is_main_thread() is a hacky way to ensure single-threadedness.
  G_ASSERT(is_main_thread());
  if (!box_collision.isValid())
    box_collision = add_dynamic_box_collision(TMatrix::IDENT, /*width*/ Point3(1.f, 1.f, 1.f),
      /*user_ptr*/ nullptr, /*add_to_world*/ false);
  return box_collision;
}

void dacoll::set_collision_object_tm(const CollisionObject &co, const TMatrix &tm)
{
  phys_world->fetchSimRes(true);
  co.body->setTmInstant(tm, true);
}

TMatrix dacoll::make_precise_tm(const TMatrix &tm)
{
  mat44f tm4;
  v_mat44_make_from_43cu_unsafe(tm4, tm.array);
  v_mat44_orthonormalize33(tm4, tm4);
  TMatrix rtm;
  v_mat_43cu_from_mat44(rtm.array, tm4);
#ifdef USE_JOLT_PHYSICS
  for (int i = 0; i < 2; ++i)
  {
    alignas(vec4f) Quat q;
    v_st(&q.x, to_jQuat(rtm).mValue.mValue);
    rtm.makeTM(q);
  }
#endif
  vec3f lsq = v_perm_xzac(v_perm_xycd(v_length3_sq(v_ldu(&tm.getcol(0).x)), v_length3_sq(v_ldu(&tm.getcol(1).x))),
    v_length3_sq(v_ldu(&tm.getcol(2).x)));
  vec3f ls = v_mul(v_round(v_mul(v_sqrt(lsq), v_splats(1e5f))), v_splats(1e-5f));
  rtm.setcol(0, rtm.getcol(0) * v_extract_x(ls));
  rtm.setcol(1, rtm.getcol(1) * v_extract_y(ls));
  rtm.setcol(2, rtm.getcol(2) * v_extract_z(ls));
  rtm.setcol(3, tm.getcol(3));
  return rtm;
}

void dacoll::set_vert_capsule_shape_size(const CollisionObject &co, float cap_rad, float cap_cyl_ht)
{
  phys_world->fetchSimRes(true);
  co.body->setVertCapsuleShapeSize(cap_rad, cap_cyl_ht);
}

void dacoll::set_collision_sphere_rad(const CollisionObject &co, float rad)
{
  phys_world->fetchSimRes(true);
  co.body->setSphereShapeRad(rad);
}

void dacoll::set_collision_box_extents(const CollisionObject &co, const Point3 &extents)
{
  phys_world->fetchSimRes(true);
  co.body->setBoxShapeExtents(extents);
}

bool dacoll::test_collision_frt(const CollisionObject &co, Tab<gamephys::CollisionContactData> &out_contacts, int mat_id)
{
  if (!phys_world)
    return false;
  phys_world->fetchSimRes(true);
  int prev_cont = out_contacts.size();
  WrapperContactResultCB collCb(out_contacts, mat_id, dacoll::get_collision_object_collapse_threshold(co));
  // TODO: per-face collision material id
  for (int i = 0; i < frtObj.size(); i++)
    phys_world->contactTestPair(co.body, frtObj[i], collCb);
  return out_contacts.size() > prev_cont;
}

bool dacoll::test_collision_world(const CollisionObject &co, Tab<gamephys::CollisionContactData> &out_contacts, int mat_id,
  dacoll::PhysLayer group, int mask)
{
  if (!phys_world)
    return false;
  phys_world->fetchSimRes(true);
  int prev_cont = out_contacts.size();
  WrapperContactResultCB collCb(out_contacts, mat_id, dacoll::get_collision_object_collapse_threshold(co));
  phys_world->contactTest(co.body, collCb, group, mask);
  return out_contacts.size() > prev_cont;
}

bool dacoll::test_collision_world(dag::ConstSpan<CollisionObject> collision, const TMatrix &tm, float bounding_rad,
  Tab<gamephys::CollisionContactData> &out_contacts, const TraceMeshFaces *trace_cache)
{
  int prevCont = out_contacts.size();
  for (const CollisionObject &co : collision)
  {
    if (!co.isValid())
      continue;
    co.body->setTmInstant(tm);
    dacoll::test_collision_frt(co, out_contacts);
    dacoll::test_collision_lmesh(co, tm, bounding_rad, lmesh_mat_id, out_contacts);
    BSphere3 sphere(Point3(0.f, 0.f, 0.f), bounding_rad);
    dacoll::test_collision_ri(co, sphere, out_contacts, trace_cache);
  }
  return prevCont != out_contacts.size();
}

bool dacoll::test_collision_world(dag::ConstSpan<CollisionObject> collision, float bounding_rad,
  Tab<gamephys::CollisionContactData> &out_contacts, const TraceMeshFaces *trace_cache)
{
  int prevCont = out_contacts.size();
  for (const CollisionObject &co : collision)
  {
    if (!co.isValid())
      continue;
    TMatrix tm = TMatrix::IDENT;
    co.body->getTmInstant(tm);
    dacoll::test_collision_frt(co, out_contacts);
    dacoll::test_collision_lmesh(co, tm, bounding_rad, lmesh_mat_id, out_contacts);
    BSphere3 sphere(Point3(0.f, 0.f, 0.f), bounding_rad);
    dacoll::test_collision_ri(co, sphere, out_contacts, trace_cache);
  }
  return prevCont != out_contacts.size();
}

bool dacoll::test_sphere_collision_world(const Point3 &pos, float radius, int mat_id,
  Tab<gamephys::CollisionContactData> &out_contacts, dacoll::PhysLayer group, int mask)
{
  if (!sphere_collision.isValid())
  {
    sphere_collision = add_dynamic_sphere_collision(TMatrix::IDENT, 1.f, /*up*/ nullptr, /*add_to_world*/ false);
    if (!sphere_collision.isValid())
      return false;
  }
  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, pos);
  dacoll::set_collision_object_tm(sphere_collision, tm);
  sphere_collision.body->setSphereShapeRad(radius);
  return dacoll::test_collision_world(sphere_collision, out_contacts, mat_id, group, mask);
}

bool dacoll::test_box_collision_world(const TMatrix &tm, int mat_id, Tab<gamephys::CollisionContactData> &out_contacts,
  dacoll::PhysLayer group, int mask)
{
  if (!box_collision.isValid())
  {
    box_collision = add_dynamic_box_collision(TMatrix::IDENT, Point3(1.f, 1.f, 1.f), /*up*/ nullptr, /*add_to_world*/ false);
    if (!box_collision.isValid())
      return false;
  }
  Point3 scale(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length());
  box_collision.body->setBoxShapeExtents(scale);
  dacoll::set_collision_object_tm(box_collision, tm);
  return dacoll::test_collision_world(box_collision, out_contacts, mat_id, group, mask);
}

bool dacoll::test_capsule_collision_world(const TMatrix &tm, float radius, float height, int mat_id,
  Tab<gamephys::CollisionContactData> &out_contacts, dacoll::PhysLayer group, int mask)
{
  if (!capsule_collision.isValid())
  {
    capsule_collision = add_dynamic_capsule_collision(TMatrix::IDENT, radius, height, nullptr, /*add_to_world*/ false);
    if (!sphere_collision.isValid())
      return false;
  }
  dacoll::set_collision_object_tm(capsule_collision, tm);
  capsule_collision.body->setVertCapsuleShapeSize(radius, height);
  return dacoll::test_collision_world(capsule_collision, out_contacts, mat_id, group, mask);
}

bool dacoll::test_collision_lmesh(const CollisionObject &co, const TMatrix &tm, float max_rad, int def_mat_id,
  Tab<gamephys::CollisionContactData> &out_contacts, int mat_id)
{
  if (!phys_world)
    return false;
  phys_world->fetchSimRes(true);
  int prev_cont = out_contacts.size();
  WrapperContactResultCB collCb(out_contacts, mat_id, dacoll::get_collision_object_collapse_threshold(co));
  collCb.collMatId = def_mat_id;
  int land_idx[256];
  if (lmeshMgr && lmeshMgr->getLandTracer())
  {
    for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), tm[3][0], tm[3][2], max_rad); j < je;
         j++)
      if (PhysBody *b = ensure_lmesh_body(land_idx[j]))
        phys_world->contactTestPair(co.body, b, collCb);
  }
  if (hmapObj)
    phys_world->contactTestPair(co.body, hmapObj, collCb);
  return out_contacts.size() > prev_cont;
}

bool dacoll::test_pair_collision(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, const TMatrix &tm_a,
  dag::ConstSpan<CollisionObject> co_b, uint64_t cof_b, const TMatrix &tm_b, Tab<gamephys::CollisionContactData> &out_contacts,
  TestPairFlags flags, bool set_co_tms /* = true */)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  if (!physWorld || !physWorld->getScene())
    return false;
  int prev_cont = out_contacts.size();
  physWorld->fetchSimRes(true);

  uint64_t tmSetB = 0;
  for (int i = 0; i < co_a.size(); ++i)
  {
    const CollisionObject &coA = co_a[i];
    if (!coA.isValid())
      continue;
    else if ((cof_a & (1ull << i)) == 0)
      continue;
    bool tmSetA = false;
    for (int j = 0; j < co_b.size(); ++j)
    {
      const CollisionObject &coB = co_b[j];
      if (!coB.isValid())
        continue;
      else if ((cof_b & (1ull << j)) == 0)
        continue;

      if (!coA || !coB)
        continue;
      if (set_co_tms)
      {
        if (!tmSetA)
          coA.body->setTmInstant(tm_a);
        tmSetA = true;
        if (!(tmSetB & (1ull << j)))
          coB.body->setTmInstant(tm_b);
        tmSetB |= 1ull << j;
      }

      if ((flags & TestPairFlags::CheckInWorld) != TestPairFlags::None && (!coA.body->isInWorld() || !coB.body->isInWorld()))
        continue;

      WrapperContactResultCB collCb(out_contacts, -1, dacoll::get_collision_object_collapse_threshold(coA));
      physWorld->contactTestPair(coA.body, coB.body, collCb);
    }
  }
  return out_contacts.size() > prev_cont;
}

bool dacoll::test_pair_collision(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, dag::ConstSpan<CollisionObject> co_b,
  uint64_t cof_b, Tab<gamephys::CollisionContactData> &out_contacts, TestPairFlags flags)
{
  return test_pair_collision(co_a, cof_a, TMatrix::IDENT, co_b, cof_b, TMatrix::IDENT, out_contacts, flags, false);
}

bool dacoll::test_pair_collision_continuous(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, const TMatrix &tm_a,
  const TMatrix &tm_a_prev, dag::ConstSpan<CollisionObject> co_b, uint64_t cof_b, const TMatrix &tm_b, const TMatrix &tm_b_prev,
  Tab<gamephys::CollisionContactData> &out_contacts)
{
  static const float distMinSq = sqr(0.1f);

  PhysWorld *physWorld = dacoll::get_phys_world();
  if (!physWorld || !physWorld->getScene())
    return false;
  const int prevCount = out_contacts.size();
  physWorld->fetchSimRes(true);

  uint64_t tmSetB = 0;
  TMatrix tmAPrev;
  bool tmAPrevReady = false, sweepA = false;
  for (int i = 0; i < co_a.size(); ++i)
  {
    const CollisionObject &coA = co_a[i];
    if (!coA.isValid())
      continue;
    else if ((cof_a & (1ull << i)) == 0)
      continue;
    bool tmSetA = false;
    for (int j = 0; j < co_b.size(); ++j)
    {
      const CollisionObject &coB = co_b[j];
      if (!coB.isValid())
        continue;
      else if ((cof_b & (1ull << j)) == 0)
        continue;

      if (!coA || !coB)
        continue;
      if (!tmSetA)
        coA.body->setTmInstant(tm_a);
      tmSetA = true;
      if (!(tmSetB & (1ull << j)))
        coB.body->setTmInstant(tm_b);
      tmSetB |= 1ull << j;

      if (!coA.body->isInWorld() || !coB.body->isInWorld())
        continue;

      const int prevCountPair = out_contacts.size();
      WrapperContactResultCB collCb(out_contacts, -1, dacoll::get_collision_object_collapse_threshold(coA));
      physWorld->contactTestPair(coA.body, coB.body, collCb);

      if (out_contacts.size() == prevCountPair)
      {
        if (!tmAPrevReady)
        {
          tmAPrev = tm_b * (inverse(tm_b_prev) * tm_a_prev);
          sweepA = !((tm_a.getcol(3) - tmAPrev.getcol(3)).lengthSq() < distMinSq);
          tmAPrevReady = true;
        }
        if (!sweepA)
          continue;

        G_ASSERT_CONTINUE(coA.body->isConvexCollisionShape());

        PhysShapeQueryResult out;
        physWorld->shapeQuery(coA.body, tmAPrev, tm_a, make_span_const(&coB.body, 1), out);
        if (out.t >= 0.0f && out.t < 1.0f)
        {
          gamephys::CollisionContactData &contactData = out_contacts.push_back();
          contactData.wpos = contactData.wposB = out.res;
          contactData.wnormB = out.norm;
          contactData.posA = inverse(tm_a) * contactData.wpos;
          contactData.posB = inverse(tm_b) * contactData.wpos;
          contactData.depth = (out.t - 1.0f) * (contactData.wpos - (tmAPrev * contactData.posA)).length();
          contactData.userPtrA = coA.body->getUserData();
          contactData.userPtrB = coB.body->getUserData();
          contactData.matId = -1;
        }
      }
    }
  }
  return out_contacts.size() > prevCount;
}

void dacoll::shape_query_frt(const PhysBody *shape, const TMatrix &from, const TMatrix &to, dacoll::ShapeQueryOutput &out)
{
  phys_world->shapeQuery(shape, from, to, frtObj, out);
}
void dacoll::shape_query_frt(const PhysSphereCollision &shape, const TMatrix &from, const TMatrix &to, dacoll::ShapeQueryOutput &out)
{
  phys_world->shapeQuery(shape, from, to, frtObj, out);
}

void dacoll::shape_query_lmesh(const PhysBody *shape, const TMatrix &from, const TMatrix &to, dacoll::ShapeQueryOutput &out)
{
  float maxRad = length(from.getcol(3) - to.getcol(3));
  if (maxRad < 0.5f)
    return;

  StaticTab<PhysBody *, 64 + 1> land_body;
  if (hmapObj)
    land_body.push_back(hmapObj);
  if (lmeshMgr && lmeshMgr->getLandTracer())
  {
    int land_idx[64];
    for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), from[3][0], from[3][2], maxRad);
         j < je; j++)
      if (auto *b = ensure_lmesh_body(land_idx[j]))
        land_body.push_back(b);
  }
  phys_world->shapeQuery(shape, from, to, land_body, out);
}

void dacoll::shape_query_lmesh(const PhysSphereCollision &shape, const TMatrix &from, const TMatrix &to, dacoll::ShapeQueryOutput &out)
{
  float maxRad = length(from.getcol(3) - to.getcol(3));
  if (maxRad < 0.5f)
    return;

  StaticTab<PhysBody *, 64 + 1> land_body;
  if (hmapObj)
    land_body.push_back(hmapObj);
  if (lmeshMgr && lmeshMgr->getLandTracer())
  {
    int land_idx[64];
    for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), from[3][0], from[3][2], maxRad);
         j < je; j++)
      if (auto *b = ensure_lmesh_body(land_idx[j]))
        land_body.push_back(b);
  }
  phys_world->shapeQuery(shape, from, to, land_body, out);
}

bool dacoll::sphere_query_ri(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  Tab<rendinst::RendInstDesc> *out_desc, const TraceMeshFaces *handle, RIFilterCB *filterCB)
{
  if (!phys_world || !phys_world->getScene() || !sphere_cast_shape)
    return false;

  TMatrix fromTm = TMatrix::IDENT;
  TMatrix toTm = TMatrix::IDENT;
  fromTm.setcol(3, from);
  toTm.setcol(3, to);

  phys_world->fetchSimRes(true);
  sphere_cast_shape->setSphereShapeRad(rad);

  shape_query_ri(sphere_cast_shape, fromTm, toTm, rad, out, cast_mat_id, out_desc, handle, filterCB);

  return out.t < 1.f;
}

bool dacoll::sphere_cast(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  const TraceMeshFaces *handle)
{
  return sphere_cast_ex(from, to, rad, out, cast_mat_id, dag::ConstSpan<CollisionObject>(), handle,
    EPL_ALL & ~(EPL_CHARACTER | EPL_KINEMATIC | EPL_DEBRIS));
}

static dacoll::gather_game_objects_collision_on_ray_cb gather_game_objs_cb = nullptr;

void dacoll::set_gather_game_objects_on_ray_cb(gather_game_objects_collision_on_ray_cb cb) { gather_game_objs_cb = cb; }

static void shape_cast_begin(dacoll::ShapeQueryOutput &out)
{
  out.vel.zero();

  // TODO: add per-object sphere_cast, so we don't really need world to perform it.
  // TODO: test it now! it's added, maybe we don't need a world at all (fetching the world)
  phys_world->fetchSimRes(true);
}

static bool shape_cast_land_begin(PhysBody *shape_body, const TMatrix &from_tm, const TMatrix &to_tm, float rad,
  dacoll::ShapeQueryOutput &out, int hmap_step)
{
  const Point3 &from = from_tm.getcol(3);
  const Point3 &to = to_tm.getcol(3);

  // frt
  phys_world->shapeQuery(shape_body, from_tm, to_tm, frtObj, out);
  const int prevStep = hmap_step > 0 ? dacoll::set_hmap_step(hmap_step) : -1;
  // lmesh
  if (lmeshMgr && lmeshMgr->getLandTracer())
  {
    float px = (from.x + to.x) / 2.f;
    float pz = (from.z + to.z) / 2.f;
    const float maxRad = length(Point2::xz(to - from)) / 2.f + rad;
    int land_idx[256];
    for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), px, pz, maxRad); j < je; j++)
      if (PhysBody *b = ensure_lmesh_body(land_idx[j]))
        phys_world->shapeQuery(shape_body, from_tm, to_tm, make_span_const(&b, 1), out);
  }
  if (hmapObj)
    phys_world->shapeQuery(shape_body, from_tm, to_tm, make_span_const(&hmapObj, 1), out);

  return prevStep;
}

static void shape_cast_land_end(int prev_step)
{
  if (prev_step > 0)
    dacoll::set_hmap_step(prev_step);
}

static bool shape_cast_land(PhysBody *shape_body, const TMatrix &from_tm, const TMatrix &to_tm, float rad,
  dacoll::ShapeQueryOutput &out, int hmap_step)
{
  shape_cast_begin(out);
  int prevStep = shape_cast_land_begin(shape_body, from_tm, to_tm, rad, out, hmap_step);
  shape_cast_land_end(prevStep);
  return out.t < 1.f;
}

static bool shape_cast_ex(PhysBody *shape_body, const TMatrix &from_tm, const TMatrix &to_tm, float rad, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, dag::ConstSpan<CollisionObject> ignore_objs, const TraceMeshFaces *handle, int mask, int hmap_step)
{
  shape_cast_begin(out);

  int prevStep = shape_cast_land_begin(shape_body, from_tm, to_tm, rad, out, hmap_step);

  if (gather_game_objs_cb)
  {
    const Point3 &from = from_tm.getcol(3);
    const Point3 &to = to_tm.getcol(3);

    MatAndGroupConvexCallback matCb{cast_mat_id, ignore_objs, dacoll::EPL_DEFAULT, mask};
    Tab<PhysBody *> bodies(framemem_ptr());
    gather_game_objs_cb(from, to, rad, [&](CollisionObject obj) {
      if (obj.body && matCb.needsCollision(obj.body, obj.body->getUserData()))
        bodies.push_back(obj.body);
    });
    phys_world->shapeQuery(shape_body, from_tm, to_tm, bodies, out);
  }

  shape_cast_land_end(prevStep);

  dacoll::shape_query_ri(shape_body, from_tm, to_tm, rad, out, cast_mat_id, nullptr, handle, nullptr);

  return out.t < 1.f;
}

bool dacoll::sphere_cast_land(const Point3 &from, const Point3 &to, float rad, ShapeQueryOutput &out, int hmap_step /* = -1 */)
{
  if (!phys_world || !phys_world->getScene() || !sphere_cast_shape)
    return false;

  TIME_PROFILE_DEV(sphere_cast);

  TMatrix fromTm = TMatrix::IDENT;
  fromTm.setcol(3, from);
  TMatrix toTm = TMatrix::IDENT;
  toTm.setcol(3, to);

  sphere_cast_shape->setSphereShapeRad(rad);

  return shape_cast_land(sphere_cast_shape, fromTm, toTm, rad, out, hmap_step);
}

bool dacoll::sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  dag::ConstSpan<CollisionObject> ignore_objs, const TraceMeshFaces *handle, int mask, int hmap_step /*  = -1 */)
{
  if (!phys_world || !phys_world->getScene() || !sphere_cast_shape)
    return false;

  TIME_PROFILE_DEV(sphere_cast);

  TMatrix fromTm = TMatrix::IDENT;
  fromTm.setcol(3, from);
  TMatrix toTm = TMatrix::IDENT;
  toTm.setcol(3, to);

  sphere_cast_shape->setSphereShapeRad(rad);

  return shape_cast_ex(sphere_cast_shape, fromTm, toTm, rad, out, cast_mat_id, ignore_objs, handle, mask, hmap_step);
}

bool dacoll::is_debug_draw_forced() { return debugDrawerForced; }

struct ScopeSetForceDraw
{
  bool prevForceDraw;
  ScopeSetForceDraw(bool s)
  {
    prevForceDraw = debugDrawerForced;
    debugDrawerForced = s;
  }
  ~ScopeSetForceDraw() { debugDrawerForced = prevForceDraw; }
};

bool dacoll::box_cast_ex(const TMatrix &from, const TMatrix &to, Point3 dimensions, dacoll::ShapeQueryOutput &out, int cast_mat_id,
  dag::ConstSpan<CollisionObject> ignore_objs, const TraceMeshFaces *handle, int mask, int hmap_step)
{
  if (!phys_world || !phys_world->getScene() || !box_cast_shape)
    return false;

  box_cast_shape->setBoxShapeExtents(dimensions);

  const float rad = max(dimensions.x, max(dimensions.y, dimensions.z));
  return shape_cast_ex(box_cast_shape, from, to, rad, out, cast_mat_id, ignore_objs, handle, mask, hmap_step);
}

void dacoll::force_debug_draw(bool flag)
{
  if (debugDrawerInited)
    physdbg::setBufferedForcedDraw(phys_world, debugDrawerForced = flag);
}

void dacoll::draw_phys_body(const PhysBody *body, unsigned col)
{
  if (!debugDrawerInited)
    return;
  ScopeSetForceDraw flagGuard(true);
  physdbg::renderOneBody(phys_world, body, physdbg::body_dbg_flags(), col);
}

void dacoll::draw_collision_object(const CollisionObject &obj)
{
  if (!debugDrawerInited)
    return;
  ScopeSetForceDraw flagGuard(true);
  if (obj.body)
    physdbg::renderOneBody(phys_world, obj.body);
}

void dacoll::draw_collision_object(const CollisionObject &obj, const TMatrix &tm)
{
  if (!debugDrawerInited)
    return;
  ScopeSetForceDraw flagGuard(true);
  physdbg::renderOneBody(phys_world, obj.body, tm);
}

void dacoll::set_obj_motion(CollisionObject obj, const TMatrix &tm, const Point3 &vel, const Point3 &omega)
{
#if ENABLE_APEX == 1
  if (enable_apex)
    physx_set_obj_motion(obj.physxObject, tm, vel, omega);
#endif

  obj.body->setTm(tm);
  obj.body->setVelocity(vel);
  obj.body->setAngularVelocity(tm % omega);
  if (lengthSq(vel) > 1e-5f || lengthSq(omega) > 1e-10f)
    obj.body->wakeUp();
#if defined(USE_BULLET_PHYSICS)
  const float physDt = 1.f / 10.f;
  const float minDistThresholdSq = 0.15f * 0.15f;
  const float distThresholdSq = max(minDistThresholdSq, lengthSq(vel) * sqr(physDt));
  btRigidBody *rigid = obj.body->getActor();
  if (rigid && lengthSq(to_point3(rigid->getCenterOfMassPosition()) - tm.getcol(3)) > distThresholdSq)
    rigid->setCenterOfMassTransform(to_btTransform(tm));
#endif
}

void dacoll::set_obj_active(CollisionObject &coll_obj, bool active, bool is_kinematic)
{
  // TODO: physx/apex

  if (!coll_obj)
    return;

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  // We need to remove it anyway, just to be sure.
  if (coll_obj.body->isInWorld())
  {
    physWorld->removeBody(coll_obj.body);
  }
  if (active)
  {
    if (is_kinematic)
      physWorld->fetchSimRes(true);
    physWorld->addBody(coll_obj.body, is_kinematic, is_kinematic);
    coll_obj.body->activateBody(true);
  }
}

bool dacoll::is_obj_active(const CollisionObject &coll_obj) { return coll_obj.body ? coll_obj.body->isInWorld() : false; }

void dacoll::disable_obj_deactivation(CollisionObject &coll_obj)
{
  if (auto *body = coll_obj.body)
    body->disableDeactivation();
}

void dacoll::obj_apply_impulse(CollisionObject obj, const Point3 &impulse, const Point3 &rel_pos)
{
  if (obj)
#if defined(USE_BULLET_PHYSICS)
    obj.body->getActor()->applyImpulse(to_btVector3(impulse), to_btVector3(rel_pos));
#else
    obj.body->addImpulse(rel_pos, impulse, false);
#endif
}

void dacoll::get_coll_obj_tm(CollisionObject obj, TMatrix &tm)
{
  if (obj)
    obj.body->getTm(tm);
}
