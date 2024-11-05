// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionLib.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_framemem.h>
#include <gameRes/dag_collisionResource.h>
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
#include <sceneRay/dag_sceneRay.h>

#include <landMesh/lmeshManager.h>
#include <landMesh/landRayTracer.h>
#include <heightmap/heightmapHandler.h>

#include <gamePhys/collision/contactResultWrapper.h>

#include <util/dag_lookup.h>
#include <util/dag_roNameMap.h>

#include <scene/dag_physMat.h>
#include <physMap/physMap.h>
#include <math/dag_shapeInertia.h>

#include "collisionGlobals.h"

static bool enable_apex = false;
static PhysWorld *phys_world = NULL;
static Tab<PhysBody *> frtObj(midmem);
static PhysBody *sphere_cast_shape = nullptr;
static PhysBody *box_cast_shape = nullptr;
static bool debugDrawerForced = false, debugDrawerInited = false;
static DeserializedStaticSceneRayTracer *scene_ray_tracer = NULL;
static BuildableStaticSceneRayTracer *water_ray_tracer = NULL;
static SmallTab<unsigned char, MidmemAlloc> pmid;
static bool scene_ray_owned = false;
static PhysBody *hmapObj = NULL;
static LandMeshManager *lmeshMgr = NULL;
static PhysMat::MatID lmesh_mat_id = -1;
static PhysMap *phys_map = NULL;
static CollisionObject sphere_collision;
static CollisionObject capsule_collision;
static CollisionObject box_collision;
static Tab<PhysBody *> lmeshObj(midmem);
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
  phys_world = new PhysWorld(0.9f, 0.7f, 0.4f, 1.f);
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
  if (scene_ray_owned)
    del_it(scene_ray_tracer);
  scene_ray_tracer = NULL;
  for (int i = 0; i < frtObj.size(); ++i)
    del_it(frtObj[i]);
  del_it(hmapObj);
  for (int i = 0; i < lmeshObj.size(); ++i)
    del_it(lmeshObj[i]);
  clear_and_shrink(lmeshObj);
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

const PhysMap *dacoll::get_lmesh_phys_map() { return phys_map; }


DeserializedStaticSceneRayTracer *dacoll::get_frt() { return scene_ray_tracer; }

BuildableStaticSceneRayTracer *dacoll::get_water_tracer() { return water_ray_tracer; }

dag::ConstSpan<unsigned char> dacoll::get_pmid() { return pmid; }


void dacoll::add_static_collision_frt(DeserializedStaticSceneRayTracer *frt, const char *name, dag::ConstSpan<unsigned char> *in_pmid)
{
  scene_ray_tracer = frt;
  scene_ray_owned = false;
  if (!phys_world || !phys_world->getScene())
    return;

#if ENABLE_APEX == 1
  if (enable_apex)
    physx_add_static_collision_frt(frt, name);
#else
  (void)name;
#endif

  debug("frt: %p,%d  %p,%d", &frt->verts(0), frt->getVertsCount(), frt->faces(0).v, frt->getFacesCount() * 3);
  PhysTriMeshCollision shape(make_span_const(&frt->verts(0), frt->getVertsCount()),
    make_span_const(frt->faces(0).v, frt->getFacesCount() * 3));

  PhysBodyCreationData pbcd;
  pbcd.useMotionState = false;
  pbcd.materialId = -1;
  pbcd.friction = 1.f;
  pbcd.autoMask = false;
  pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);
  PhysBody *body = new PhysBody(dacoll::get_phys_world(), 0.f, &shape, TMatrix::IDENT, pbcd);
  frtObj.push_back(body);

  if (in_pmid)
  {
    G_ASSERT(in_pmid->size() == frt->getFacesCount());
    pmid = *in_pmid;
  }
}

bool dacoll::load_static_collision_frt(IGenLoad *crd)
{
  int versionId = crd->readInt();
  (void)versionId;
  int start_pos = crd->tell();
  DeserializedStaticSceneRayTracer *sceneRayTracer = new (midmem) DeserializedStaticSceneRayTracer;
  bool res = sceneRayTracer->serializedLoad(*crd);

  // load pmid
  clear_and_resize(pmid, sceneRayTracer->getFacesCount());
  crd->readTabData(pmid);

  // align on 4
  start_pos = crd->tell() - start_pos;
  if (start_pos & 3)
    crd->seekrel(4 - (start_pos & 3));

  // read mat names
  int nm_sz = crd->readInt();
  Tab<uint8_t> mem(framemem_ptr());
  mem.resize(nm_sz);
  crd->read(mem.data(), nm_sz);

  RoNameMap &nm = *(RoNameMap *)mem.data();
  nm.patchData(mem.data());

  // resolve phys materials
  Tab<uint8_t> matid(framemem_ptr());
  matid.resize(nm.map.size());
  for (int i = 0; i < nm.map.size(); ++i)
  {
    const char *matname = nm.map[i];
    int id = PhysMat::getMaterialId(matname);
    if (id == PHYSMAT_INVALID)
    {
      logwarn("physmat %s not found; setting as default", matname);
      id = PHYSMAT_DEFAULT;
    }
    G_ASSERT(id >= 0 && id <= 255);
    matid[i] = id;
  }

  // remap phys mat id for faces
  for (int i = 0; i < pmid.size(); ++i)
  {
    G_ASSERT(pmid[i] < nm.map.size());
    pmid[i] = matid[pmid[i]];
  }

  dacoll::add_static_collision_frt(sceneRayTracer, crd->getTargetName());
  scene_ray_owned = true;
  return res;
}

void dacoll::set_water_tracer(BuildableStaticSceneRayTracer *tracer) { water_ray_tracer = tracer; }

int dacoll::get_hmap_step() { return hmapObj ? phys_body_get_hmap_step(hmapObj) : -1; }

int dacoll::set_hmap_step(int step) { return hmapObj ? phys_body_set_hmap_step(hmapObj, step) : -1; }

void dacoll::add_collision_hmap_custom(const Point3 &collision_pos, const BBox3 &collision_box, const Point2 &hmap_offset,
  float hmap_scale, int hmap_step)
{
  LandMeshManager *land = get_lmesh();
  if (!phys_world || !phys_world->getScene() || !land || !land->getLandTracer())
    return;

  if (land->getHmapHandler())
  {
    PhysHeightfieldCollision coll(land->getHmapHandler(), hmap_offset, collision_box, hmap_scale, hmap_step, land->getHolesManager());
    PhysBodyCreationData pbcd;
    pbcd.useMotionState = false;
    pbcd.materialId = -1;
    pbcd.friction = 1.f;
    pbcd.autoMask = false;
    pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, collision_pos);
    frtObj.push_back(new PhysBody(dacoll::get_phys_world(), 0.f, &coll, tm, pbcd));
  }
}

void dacoll::add_collision_hmap(LandMeshManager *land, float restitution, float margin)
{
  if (!phys_world || !phys_world->getScene() || !land || !land->getLandTracer())
    return;

#if ENABLE_APEX == 1
  if (enable_apex)
    physx_add_static_collision_heightmap(land);
#endif

  if (land->getHmapHandler())
  {
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
  else
    hmapObj = NULL;
}

void dacoll::add_collision_landmesh(LandMeshManager *land, const char *name, float restitution)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  if (!physWorld || !physWorld->getScene() || !land || !land->getLandTracer())
    return;

#if ENABLE_APEX == 1
  if (enable_apex)
    physx_add_static_collision_landmesh(land, name);
#else
  (void)name;
#endif

  LandRayTracer *lray = land->getLandTracer();
  lmeshObj.resize(lray->getCellCount());
  TMatrix tm = TMatrix::IDENT;
  PhysBodyCreationData pbcd;
  pbcd.useMotionState = false;
  pbcd.materialId = -1;
  pbcd.friction = 1.f;
  pbcd.restitution = restitution;
  pbcd.autoMask = false;
  pbcd.group = EPL_STATIC, pbcd.mask = EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC);

  for (int i = 0; i < lray->getCellCount(); i++)
  {
    // debug("landmesh: cell[%d] %p,%d  %p,%d", i, lray->getCellVerts(i).data(), lray->getCellVerts(i).size(),
    // lray->getCellFaces(i).data(), lray->getCellFaces(i).size());
    if (lray->getCellFaces(i).size())
    {
      const float *ofs = lray->getCellPackOffsetF(i);
      tm.setcol(3, Point3(ofs[0], ofs[1], ofs[2]));
      PhysTriMeshCollision shape(lray->getCellVerts(i), lray->getCellFaces(i), lray->getCellPackScaleF(i), true);
      lmeshObj[i] = new PhysBody(dacoll::get_phys_world(), 0.f, &shape, tm, pbcd);
    }
    else
      lmeshObj[i] = NULL;
  }
  lmesh_mat_id = PhysMat::physMatCount() > 0 ? PhysMat::getMaterialId("horLandMesh") : PHYSMAT_INVALID;
  lmeshMgr = land;
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
  bool auto_mask, int group_filter, int mask, const TMatrix *wtm)
{
  PhysBodyCreationData pbcd;
  pbcd.materialId = -1;
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

static PhysCollision *create_shape_from_collision_node(const char *shape_type, const CollisionNode *node, float scale,
  TMatrix &child_tm, Point3 *out_center, SmallTab<Point3_vec4, TmpmemAlloc> &vertices /*stor*/)
{
  if (!shape_type)
    return nullptr;

  DynColShapeType nodeType = DynColShapeType(lup(shape_type, dynCollShapeTypeNames, countof(dynCollShapeTypeNames), CST_MESH));

  if (nodeType == CST_MESH || nodeType == CST_CONVEX_HULL || out_center)
    vertices = node->vertices;
  if (out_center)
  {
    out_center->zero();

    for (int i = 0; i < vertices.size(); i++)
      (*out_center) += vertices[i];

    (*out_center) *= 1.f / (float)vertices.size();

    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, -(*out_center));
    (*out_center) = tm.getcol(3);

    for (int i = 0; i < vertices.size(); i++)
      vertices[i] = scale * tm * vertices[i];
  }

  if (nodeType == CST_BOX)
  {
    child_tm.setcol(3, child_tm * node->modelBBox.center());
    Point3 width = node->modelBBox.width();
    return new PhysBoxCollision(width.x, width.y, width.z);
  }

  if (nodeType == CST_MESH)
  {
    G_ASSERTF_RETURN(node->indices.size() / 3 < 300 * 1024, nullptr,
      "Too much triangles in mesh: %d verts and %d faces! 300k is too much already!", vertices.size(), node->indices.size() / 3);
    return new PhysTriMeshCollision(vertices, node->indices, nullptr, false, false /*reverse normals*/);
  }

  if (nodeType == CST_CONVEX_HULL)
    return new PhysConvexHullCollision(vertices, false);

  if (nodeType == CST_SPHERE)
    return new PhysSphereCollision(scale * node->boundingSphere.r);

  return nullptr;
}

CollisionObject dacoll::add_simple_dynamic_collision_from_coll_resource(const DataBlock &props, const CollisionResource *resource,
  GeomNodeTree *tree, float margin, float scale, Point3 &out_center, TMatrix &out_tm, TMatrix &out_tm_in_model, bool add_to_world)
{
  if (!phys_world || !phys_world->getScene() || !resource)
    return CollisionObject();

  PhysCompoundCollision shape;
  SmallTab<Point3_vec4, TmpmemAlloc> vertices_stor;
  shape.setMargin(margin);

  const CollisionNode *rootNode = nullptr;
  TMatrix rootTm = TMatrix::IDENT;

  for (int i = 0; i < props.paramCount(); ++i)
  {
    const char *shapeType = props.getStr(i);
    if (!shapeType)
      continue;

    const char *nodeName = props.getParamName(i);
    for (const CollisionNode *meshNode = resource->meshNodesHead; meshNode; meshNode = meshNode->nextNode)
    {
      if (!meshNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;

      if (::strcmp(nodeName, meshNode->name.str()) != 0)
        continue;

      TMatrix nodeTm;
      resource->getCollisionNodeTm(meshNode, TMatrix::IDENT, tree, nodeTm);

      TMatrix childTm = nodeTm * inverse(rootTm);

      if (!rootNode)
        childTm.setcol(3, ZERO<Point3>());

      Point3 outCenter = ZERO<Point3>();
      PhysCollision *childShape =
        create_shape_from_collision_node(shapeType, meshNode, scale, childTm, rootNode ? nullptr : &outCenter, vertices_stor);
      if (!childShape)
        continue;

      childShape->setMargin(margin);
      shape.addChildCollision(childShape, childTm);

      if (!rootNode)
      {
        rootNode = meshNode;
        rootTm = nodeTm;
        rootTm.setcol(3, rootTm.getcol(3) - childTm * outCenter);
        out_center = childTm * outCenter;
        out_tm_in_model = nodeTm;
        out_tm = nodeTm;
        out_tm.setcol(3, childTm * outCenter);
      }

      break;
    }
  }

  return dacoll::create_coll_obj_from_shape(shape, nullptr, /*kinematic*/ true, /* add to world */ add_to_world,
    /* auto mask */ false);
}

void dacoll::build_dynamic_collision_from_coll_resource(const CollisionResource *coll_resource, PhysCompoundCollision *compound,
  PhysBodyProperties &out_properties)
{
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

  // TODO: add per-node collision infer and build collision such as convex hull with convex hull computers
  return dacoll::create_coll_obj_from_shape(shape, nullptr, true, add_to_world, /* auto mask */ false, phys_layer, mask);
}

CollisionObject dacoll::add_dynamic_collision_from_coll_resource(const DataBlock *props, const CollisionResource *coll_resource,
  void *user_ptr, int flags, int phys_layer, int mask, const TMatrix *wtm)
{
  if (!phys_world || !phys_world->getScene() || !coll_resource ||
      (!coll_resource->meshNodesHead && !coll_resource->boxNodesHead && !coll_resource->sphereNodesHead &&
        !coll_resource->capsuleNodesHead))
    return CollisionObject();

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
  if (collapseConvexes)
  {
    // go through all convexes and collapse them into one
    bool haveNonConvex = coll_resource->boxNodesHead || coll_resource->sphereNodesHead || coll_resource->capsuleNodesHead;
    Tab<Point3> &convexVerts = vertices_stor.push_back();
    dag::set_allocator(convexVerts, dag::get_allocator(vertices_stor));
    for (const CollisionNode *meshNode = coll_resource->meshNodesHead; meshNode; meshNode = meshNode->nextNode)
    {
      if (!meshNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;

      if (meshNode->type == COLLISION_NODE_TYPE_CONVEX)
      {
        convexVerts.reserve(meshNode->vertices.size());
        for (const Point3_vec4 &vert : meshNode->vertices)
          convexVerts.push_back(meshNode->tm * vert); // TODO: vectorize
      }
      haveNonConvex |= meshNode->type != COLLISION_NODE_TYPE_CONVEX;
    }
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

  for (const CollisionNode *meshNode = coll_resource->meshNodesHead; meshNode; meshNode = meshNode->nextNode)
  {
    if (!meshNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;

    const char *nodeTypeName = props ? props->getStr(meshNode->name.str(), NULL) : NULL;
    if (props && !nodeTypeName)
      continue;

    if (collapseConvexes && meshNode->type == COLLISION_NODE_TYPE_CONVEX)
      continue;

    DynColShapeType nodeType =
      (flags & ACO_BOXIFY)
        ? CST_BOX
        : ((meshNode->type == COLLISION_NODE_TYPE_CONVEX || (flags & ACO_FORCE_CONVEX_HULL)) ? CST_CONVEX_HULL : CST_MESH);
    if (nodeTypeName)
      nodeType = DynColShapeType(lup(nodeTypeName, dynCollShapeTypeNames, countof(dynCollShapeTypeNames), nodeType));
    switch (nodeType)
    {
      case CST_BOX:
      {
        TMatrix tm = meshNode->tm;
        tm.setcol(3, tm * meshNode->modelBBox.center());
        Point3 width = meshNode->modelBBox.width();
        shape.addChildCollision(new PhysBoxCollision(width.x, width.y, width.z), tm);
      }
      break;

      case CST_MESH:
      {
        G_ASSERTF_AND_DO(meshNode->indices.size() / 3 < 300 * 1024, break,
          "Too much triangles in mesh: %d verts and %d faces! 300k is too much already!", meshNode->vertices.size(),
          meshNode->indices.size() / 3);
        auto trim = new PhysTriMeshCollision(meshNode->vertices, meshNode->indices, nullptr, false, false /*reverse normals*/);
        trim->setDebugName(meshNode->name.c_str());
        shape.addChildCollision(trim, meshNode->tm);
      }
      break;

      case CST_CONVEX_HULL:
      {
        Tab<Point3> &vertices = vertices_stor.push_back();
        dag::set_allocator(vertices, dag::get_allocator(vertices_stor));
        for (const Point3 &vert : meshNode->vertices)
          vertices.push_back(meshNode->tm * vert);
        shape.addChildCollision(new PhysConvexHullCollision(vertices, false), TMatrix::IDENT);
      }
      break;

      default: break;
    };
  }

  for (const CollisionNode *boxNode = coll_resource->boxNodesHead; boxNode; boxNode = boxNode->nextNode)
  {
    if (!boxNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;
    if (props && !props->getBool(boxNode->name.str(), false))
      continue;
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, boxNode->modelBBox.center());
    Point3 width = boxNode->modelBBox.width();
    shape.addChildCollision(new PhysBoxCollision(width.x, width.y, width.z), tm);
  }

  for (const CollisionNode *sphereNode = coll_resource->sphereNodesHead; sphereNode; sphereNode = sphereNode->nextNode)
  {
    if (!sphereNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;
    if (props && !props->getBool(sphereNode->name.str(), false))
      continue;
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, sphereNode->boundingSphere.c);
    shape.addChildCollision(new PhysSphereCollision(sphereNode->boundingSphere.r), tm);
  }

  for (const CollisionNode *capNode = coll_resource->capsuleNodesHead; capNode; capNode = capNode->nextNode)
  {
    if (!capNode->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      continue;
    if (props && !props->getBool(capNode->name.str(), false))
      continue;
    TMatrix tm = capNode->tm;
    Point3 scale = Point3(length(tm.getcol(0)), length(tm.getcol(1)), length(tm.getcol(2)));
    Point3 boxWidth = capNode->modelBBox.width();
    boxWidth.x *= scale.x;
    boxWidth.y *= scale.y;
    boxWidth.z *= scale.z;
    float capHt = max(boxWidth.x, max(boxWidth.y, boxWidth.z));
    float capRad = min(boxWidth.x, min(boxWidth.y, boxWidth.z)) * 0.5f;
    tm.setcol(3, tm * capNode->modelBBox.center());
    tm.orthonormalize();
    int ax = 0;
    if (boxWidth.x > boxWidth.y && boxWidth.x > boxWidth.z)
      ax = 0 /*X*/;
    else if (boxWidth.y > boxWidth.x && boxWidth.y > boxWidth.z)
      ax = 1 /*Y*/;
    else
      ax = 2 /*Z*/;
    shape.addChildCollision(new PhysCapsuleCollision(capRad, capHt + capRad * 2, ax), tm);
  }

  if (props)
  {
    if (const DataBlock *customCollPropsBlk = props->getBlockByNameEx("customSphereCollisions", nullptr))
    {
      for (int i = 0; i < customCollPropsBlk->paramCount(); ++i)
      {
        Point4 sph = customCollPropsBlk->getPoint4(i);
        TMatrix tm = TMatrix::IDENT;
        tm.setcol(3, Point3(sph.x, sph.y, sph.z));
        shape.addChildCollision(new PhysSphereCollision(sph.w), tm);
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
          shape.addChildCollision(new PhysBoxCollision(box.width().x, box.width().y, box.width().z), tm);
        }
      }
    }
  }

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

CollisionObject dacoll::add_dynamic_collision_convex_from_coll_resource(dag::ConstSpan<const CollisionNode *> coll_nodes,
  const Point3 &offset, void *user_ptr, int node_type_flags, bool kinematic, bool add_to_world, int phys_layer, int mask,
  const TMatrix *wtm)
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
        convexVerts.resize(prevCount + node.vertices.size());
        for (int i = 0; i < node.vertices.size(); ++i)
          convexVerts[prevCount + i] = node.tm * node.vertices[i] + offset;
        break;
      }
      case COLLISION_NODE_TYPE_BOX:
      case COLLISION_NODE_TYPE_SPHERE:
        convexVerts.resize(prevCount + 8);
        for (int i = 0; i < 8; ++i)
          convexVerts[prevCount + i] = node.tm * node.modelBBox.point(i) + offset;
        break;
      case COLLISION_NODE_TYPE_CAPSULE:
      case COLLISION_NODE_TYPE_CONVEX:
      case COLLISION_NODE_TYPE_POINTS:
      default:
        convexVerts.push_back(node.tm.getcol(3) + offset);
        break;
        ;
    }
  }
  if (convexVerts.size() >= 3) // can't create convex with less then 3 vertexes
  {
    PhysConvexHullCollision convexShape(convexVerts, true);
    CollisionObject result =
      dacoll::create_coll_obj_from_shape(convexShape, user_ptr, kinematic, add_to_world, /* auto mask */ false, phys_layer, mask, wtm);
    return result;
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
    BBox3 box(Point3(0.f, 0.f, 0.f), bounding_rad);
    dacoll::test_collision_ri(co, box, out_contacts, trace_cache);
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
    BBox3 box(Point3(0.f, 0.f, 0.f), bounding_rad);
    dacoll::test_collision_ri(co, box, out_contacts, trace_cache);
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
  dacoll::set_collision_object_tm(box_collision, tm);
  return dacoll::test_collision_world(box_collision, out_contacts, mat_id, group, mask);
}

bool dacoll::test_collision_lmesh(const CollisionObject &co, const TMatrix &tm, float max_rad, int def_mat_id,
  Tab<gamephys::CollisionContactData> &out_contacts, int mat_id)
{
  if (!phys_world || !lmeshMgr)
    return false;
  phys_world->fetchSimRes(true);
  int prev_cont = out_contacts.size();
  WrapperContactResultCB collCb(out_contacts, mat_id, dacoll::get_collision_object_collapse_threshold(co));
  collCb.collMatId = def_mat_id;
  int land_idx[256];
  for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), tm[3][0], tm[3][2], max_rad); j < je;
       j++)
    if (lmeshObj[land_idx[j]])
      phys_world->contactTestPair(co.body, lmeshObj[land_idx[j]], collCb);
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

  for (int i = 0; i < co_a.size(); ++i)
  {
    const CollisionObject &coA = co_a[i];
    if (!coA.isValid())
      continue;
    else if ((cof_a & (1ull << i)) == 0)
      continue;
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
        coA.body->setTmInstant(tm_a);
        coB.body->setTmInstant(tm_b);
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

  for (int i = 0; i < co_a.size(); ++i)
  {
    const CollisionObject &coA = co_a[i];
    if (!coA.isValid())
      continue;
    else if ((cof_a & (1ull << i)) == 0)
      continue;
    for (int j = 0; j < co_b.size(); ++j)
    {
      const CollisionObject &coB = co_b[j];
      if (!coB.isValid())
        continue;
      else if ((cof_b & (1ull << j)) == 0)
        continue;

      if (!coA || !coB)
        continue;
      coA.body->setTmInstant(tm_a);
      coB.body->setTmInstant(tm_b);

      if (!coA.body->isInWorld() || !coB.body->isInWorld())
        continue;

      const int prevCountPair = out_contacts.size();
      WrapperContactResultCB collCb(out_contacts, -1, dacoll::get_collision_object_collapse_threshold(coA));
      physWorld->contactTestPair(coA.body, coB.body, collCb);

      if (out_contacts.size() == prevCountPair)
      {
        TMatrix tmAPrev = tm_b * (inverse(tm_b_prev) * tm_a_prev);
        const Point3 dir = tm_a.getcol(3) - tmAPrev.getcol(3);
        if (dir.lengthSq() < distMinSq)
          continue;

        coB.body->setTmInstant(tm_b);

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

PhysBody *dacoll::get_convex_shape(CollisionObject co) { return co.body && co.body->isConvexCollisionShape() ? co.body : nullptr; }


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
  if (maxRad < 0.5f || !lmeshMgr)
    return;

  int land_idx[256];
  StaticTab<PhysBody *, 64> land_body;
  for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), from[3][0], from[3][2], maxRad); j < je;
       j++)
    if (auto *b = lmeshObj[land_idx[j]])
      land_body.push_back(b);
  phys_world->shapeQuery(shape, from, to, land_body, out);
}
void dacoll::shape_query_lmesh(const PhysSphereCollision &shape, const TMatrix &from, const TMatrix &to, dacoll::ShapeQueryOutput &out)
{
  float maxRad = length(from.getcol(3) - to.getcol(3));
  if (maxRad < 0.5f || !lmeshMgr)
    return;

  int land_idx[256];
  StaticTab<PhysBody *, 64> land_body;
  for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), from[3][0], from[3][2], maxRad); j < je;
       j++)
    if (auto *b = lmeshObj[land_idx[j]])
      land_body.push_back(b);
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
  if (lmeshMgr)
  {
    float px = (from.x + to.x) / 2.f;
    float pz = (from.z + to.z) / 2.f;
    const float maxRad = length(Point2::xz(to - from)) / 2.f + rad;
    int land_idx[256];
    for (int j = 0, je = lmeshMgr->getLandTracer()->getCellIdxNear(land_idx, countof(land_idx), px, pz, maxRad); j < je; j++)
      if (lmeshObj[land_idx[j]])
        phys_world->shapeQuery(shape_body, from_tm, to_tm, make_span_const(&lmeshObj[land_idx[j]], 1), out);
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

void dacoll::draw_phys_body(const PhysBody *body)
{
  if (!debugDrawerInited)
    return;
  ScopeSetForceDraw flagGuard(true);
  physdbg::renderOneBody(phys_world, body);
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

  obj.body->setVelocity(vel);
  obj.body->setAngularVelocity(tm % omega);
  obj.body->setTm(tm);
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
