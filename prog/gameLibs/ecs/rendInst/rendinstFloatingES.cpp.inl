#include <gamePhys/phys/rendinstFloating.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/delayedAct/actInThread.h>
#include <math/dag_vecMathCompatibility.h>
#include <gamePhys/props/atmosphere.h>
#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <fftWater/fftWater.h>
#include <util/dag_convar.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug3d.h>
#include <3d/dag_render.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/gpuObjects.h>
#include <math/dag_noise.h>
#include <landMesh/lmeshManager.h>

static CONSOLE_BOOL_VAL("rendinst", render_debug_floating_volumes, false);
static CONSOLE_BOOL_VAL("rendinst", render_debug_floating_phys_ripples, false);

static constexpr float RANDOM_WAVES_MAX_AT_WATER_STRENGTH = 0.5f;

namespace dacoll
{
extern LandMeshManager *get_lmesh();
}

struct Obstacle
{
  Point3 pos;
  Point3 dir;
  float size = 0.0f;
};

ECS_REGISTER_RELOCATABLE_TYPE(rendinstfloating::PhysFloatingModel, nullptr);
ECS_AUTO_REGISTER_COMPONENT(rendinstfloating::PhysFloatingModel, "floatingRiGroup__riPhysFloatingModel", nullptr, 0);

ECS_DECLARE_BOXED_TYPE(FFTWater);

template <typename Callable>
inline void get_camera_pos_ecs_query(Callable c);
template <typename Callable>
inline void get_obstacles_ecs_query(Callable c);
template <typename Callable>
static void find_floatable_group_for_res_ecs_query(Callable c);
template <typename Callable>
inline void update_floating_rendinst_instances_ecs_query(Callable c);
template <typename Callable>
inline void construct_floating_volumes_ecs_query(Callable c);
template <typename Callable>
inline void draw_rendinst_floating_volumes_ecs_query(Callable c);
template <typename Callable>
inline void draw_floating_phys_ripples_ecs_query(Callable c);

static float frac(float v) { return v - floorf(v); }

// We probably have such function somewhere, but couldn't find.
static float remap_range_clamp(float v, float from_min, float from_max, float to_min, float to_max)
{
  return saturate((v - from_min) / (from_max - from_min)) * (to_max - to_min) + to_min;
}

// Mass is distributed across volumes proportionally to their volumes.
static Point3 calculate_center_of_gravity(BSphere3 *volumes, int count)
{
  float massProportionSum = 0.0f;
  Point3 centerOfGravity(0, 0, 0);
  for (int i = 0; i < count; ++i)
  {
    float massProportion = volumes[i].r * volumes[i].r2;
    massProportionSum += massProportion;
    centerOfGravity += volumes[i].c * massProportion;
  }
  centerOfGravity /= massProportionSum;
  return centerOfGravity;
}

static dag::Vector<Obstacle, framemem_allocator> get_obstacles(vec3f cam_pos, float max_dist_sq)
{
  static int reserveCnt = 0; // TODO: move to floating_rendinst_res_group component
  dag::Vector<Obstacle, framemem_allocator> obstacles;
  obstacles.reserve(reserveCnt); // Reduce number of allocations by reserving a maximum between frames.
  get_obstacles_ecs_query([&](const TMatrix &transform, const Point3 &deform_bbox__bmin, const Point3 &deform_bbox__bmax) {
    if (v_test_vec_x_gt(v_length3_sq_x(v_sub(cam_pos, v_ldu(transform.m[3]))), v_set_x(max_dist_sq)))
      return;

    BBox3 box(deform_bbox__bmin, deform_bbox__bmax);
    Point3 boxCenter = box.center();
    Point3 boxSize = box.width();
    Obstacle obstacle;
    obstacle.pos = transform * boxCenter;
    obstacle.dir = boxCenter + orthonormalized_inverse(transform).getcol(1);
    obstacle.size = 0.5f * sqrtf(boxSize.x * boxSize.x + boxSize.z * boxSize.z);
    obstacles.push_back(obstacle);
  });
  reserveCnt = max(reserveCnt, int(obstacles.size()));
  return obstacles;
}

static float get_water_ripples_noise(Point3 pos)
{
  return 2.0f * perlin_noise::noise3(&pos.x); // noise3() returns values in about [-0.5, 0.5] range.
}

static inline vec3f get_camera_pos()
{
  bool found = false;
  vec3f ret;
  get_camera_pos_ecs_query([&](const TMatrix &transform ECS_REQUIRE(eastl::true_type camera__active, ecs::Tag camera_view)) {
    ret = v_ldu(transform.m[3]);
    found = true;
  });
  return found ? ret : v_splats(VERY_BIG_NUMBER);
}

static dag::Vector<Point3> extract_sphere_coords(const ecs::Object &volume_presets, int volumes_count = -1,
  const ecs::string &preset_name = "")
{
  dag::Vector<Point3> result;

  for (auto &preset : volume_presets)
  {
    const ecs::Array &volumePositions = preset.second.get<ecs::Array>();
    if (volumePositions.size() == volumes_count || preset.first == preset_name)
    {
      for (const auto &elem : volumePositions)
        result.push_back(elem.get<Point3>());
      break;
    }
  }
  if (result.empty())
    result.push_back(Point3(0, 0, 0));

  return result;
}

static float get_water_level()
{
  if (FFTWater *water = dacoll::get_water())
    return fft_water::get_level(water);
  else
    return 0.0f;
}

static bool is_ri_on_water(int res_idx, int inst_idx)
{
  rendinst::riex_handle_t riId = rendinst::make_handle(res_idx, inst_idx);
  vec4f riBSph = rendinst::getRIGenExtraBSphere(riId);

  Point3 traceStart = as_point3(&riBSph);
  traceStart.y += v_extract_w(riBSph);
  Point3 traceDir(0, -1, 0);
  real t = traceStart.y - get_water_level();

  if (t < 0.0f)
    return true;
  // Pass riId as skip_riex_handle to not get intersection with itself.
  bool isOnWater =
    !dacoll::rayhit_normalized(traceStart, traceDir, t, dacoll::ETF_FRT | dacoll::ETF_LMESH | dacoll::ETF_RI, -1, nullptr, riId);

  return isOnWater;
}

static bool is_ri_on_ground(int res_idx, int inst_idx, float dist_tolerance)
{
  LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  if (!lmeshMgr)
    return false;

  rendinst::riex_handle_t riId = rendinst::make_handle(res_idx, inst_idx);
  const bbox3f *resCollBox = rendinst::getRIGenExtraCollBb(res_idx);

  mat44f riTm;
  rendinst::getRIGenExtra44(riId, riTm);
  bbox3f riBbox;
  v_bbox3_init(riBbox, riTm, *resCollBox);

  vec3f bottomCenter = v_mul(v_add(riBbox.bmin, v_perm_xbzw(riBbox.bmax, riBbox.bmin)), V_C_HALF);
  Point2 bottomCenterXZ(v_extract_x(bottomCenter), v_extract_z(bottomCenter));
  float bottomHeight = v_extract_y(bottomCenter);

  float groundHeight = 0.0f;
  bool tracingResult = lmeshMgr->getHeight(bottomCenterXZ, groundHeight, nullptr);

  return tracingResult && (groundHeight > bottomHeight - dist_tolerance);
}

static int remove_invalid_floating_phys_instances(eastl::vector<rendinstfloating::RiFloatingPhys> &instances,
  dag::ConstSpan<mat43f> ri_tm)
{
  auto newEnd = eastl::remove_if(instances.begin(), instances.end(), [&ri_tm](const auto &ri_floating_phys) {
    return ri_floating_phys.riInstId >= ri_tm.size() || !rendinst::riex_is_instance_valid(ri_tm[ri_floating_phys.riInstId]);
  });
  int removedCnt = instances.end() - newEnd;
  instances.resize(newEnd - instances.begin());
  return removedCnt;
}

static void init_floating_phys_bbox(rendinstfloating::RiFloatingPhys &floating_ri, const TMatrix &tm, const BBox3 &bbox)
{
  Point3 bboxCenter = bbox.center();
  BBox3 centeredBbox(bbox.lim[0] - bboxCenter, bbox.lim[1] - bboxCenter);

  Matrix3 rot;
  rot.setcol(0, tm.getcol(0));
  rot.setcol(1, tm.getcol(1));
  rot.setcol(2, tm.getcol(2));

  Point3 bboxPositions[8];
  Point2 bboxPositionsProjected[8];
  for (int i = 0; i < 8; ++i)
  {
    bboxPositions[i] = rot * Point3(centeredBbox.lim[i / 4].x, centeredBbox.lim[i / 2 % 2].y, centeredBbox.lim[i % 2].z);
    bboxPositionsProjected[i] = Point2(bboxPositions[i].x, bboxPositions[i].z);
  }

  Point2 edgesProjected[3] = {bboxPositionsProjected[1] - bboxPositionsProjected[0],
    bboxPositionsProjected[2] - bboxPositionsProjected[0], bboxPositionsProjected[4] - bboxPositionsProjected[0]};

  float edgeDistsSq[3] = {
    dot(edgesProjected[0], edgesProjected[0]), dot(edgesProjected[1], edgesProjected[1]), dot(edgesProjected[2], edgesProjected[2])};

  int maxEdge = edgeDistsSq[0] > edgeDistsSq[1] && edgeDistsSq[0] > edgeDistsSq[2] ? 0 : (edgeDistsSq[1] > edgeDistsSq[2] ? 1 : 2);

  Point2 xDir = normalize(edgesProjected[maxEdge]);
  Point2 zDir = Point2(xDir.y, -xDir.x);

  float halfLenX = 0.0f, halfLenZ = 0.0f, halfHeight = 0.0;
  for (int i = 0; i < 8; ++i)
  {
    halfLenX = max(dot(bboxPositionsProjected[i], xDir), halfLenX);
    halfLenZ = max(dot(bboxPositionsProjected[i], zDir), halfLenZ);
    halfHeight = max(bboxPositions[i].y, halfHeight);
  }

  floating_ri.bboxHalfSize = Point3(halfLenX, halfHeight, halfLenZ);
  floating_ri.xDir = xDir;
}

static void restore_floating_volumes(const dag::Vector<Point3> &spheres_coords, float spheres_radius, const BBox3 &phys_bbox,
  const rendinstfloating::RiFloatingPhys &phys, BSphere3 *out_volumes)
{
  G_ASSERTF(spheres_coords.size() <= gamephys::floating_volumes::MAX_VOLUMES,
    "The maximum amount of floating volumes %d is exceeded with %d volumes in some preset", gamephys::floating_volumes::MAX_VOLUMES,
    spheres_coords.size());

  Point3 localBoxCenter = phys_bbox.center();
  Quat invStaticOrient = inverse(phys.staticLoc.O.getQuat());

  for (int i = 0; i < spheres_coords.size(); ++i)
  {
    const Point3 &centerMulSize = mul(spheres_coords[i], phys.bboxHalfSize);
    Point3 volCenter(centerMulSize.x * phys.xDir.x + centerMulSize.z * phys.xDir.y, centerMulSize.y,
      centerMulSize.x * phys.xDir.y - centerMulSize.z * phys.xDir.x);
    out_volumes[i] = BSphere3(invStaticOrient * volCenter + localBoxCenter, spheres_radius);
  }
}

static rendinstfloating::RiFloatingPhys create_ri_floating_phys(mat43f_cref ri_tm, int inst_idx, const BBox3 &bbox)
{
  rendinstfloating::RiFloatingPhys floatingRi;

  TMatrix4_vec4 m4;
  v_mat43_transpose_to_mat44((mat44f &)m4, ri_tm);
  TMatrix tm = tmatrix(m4);

  Point3 scale;
  scale.x = tm.getcol(0).length();
  scale.y = tm.getcol(1).length();
  scale.z = tm.getcol(2).length();
  tm.orthonormalize();

  floatingRi.riInstId = inst_idx;
  floatingRi.location.fromTM(tm);
  floatingRi.prevLoc = floatingRi.prevVisualLoc = floatingRi.visualLoc = floatingRi.location;
  floatingRi.staticLoc = floatingRi.location;
  floatingRi.scale = scale;
  floatingRi.omega = Point3(0.f, 0.f, 0.f);
  floatingRi.velocity = Point3(0.f, 0.f, 0.f);

  init_floating_phys_bbox(floatingRi, tm, bbox);

  return floatingRi;
}

static void update_floating_phys(float at_time, int &cur_tick, int interaction_type, float inv_mass, float elasticity,
  float phys_update_dt, float max_shift_dist, Point3 inv_moment_of_inertia, float spheres_radius, float spheres_viscosity,
  const dag::Vector<Point3> &spheres_coords, BBox3 phys_bbox, const Obstacle *obstacles, int obstacles_cnt, float waves_amplitude,
  float waves_inv_length, float waves_inv_period, Point2 waves_velocity, rendinstfloating::RiFloatingPhys &phys)
{
  const float dt = phys_update_dt;
  static const float MAX_ADD_INTERACTION_VELOCITY = 10.0f;
  if (cur_tick <= 0)
    cur_tick = gamephys::nearestPhysicsTickNumber(at_time + dt, dt);

  gamephys::Loc &location = phys.location;
  gamephys::Loc &prevLoc = phys.prevLoc;
  Point3 &velocity = phys.velocity;
  Point3 &omega = phys.omega;

  int volumeCount = spheres_coords.size();
  BSphere3 volumes[gamephys::floating_volumes::MAX_VOLUMES];
  Point3 center_of_gravity(0, 0, 0);
  float curTime = float(cur_tick) * dt;
  if (curTime < at_time)
  {
    restore_floating_volumes(spheres_coords, spheres_radius, phys_bbox, phys, volumes);
    center_of_gravity = calculate_center_of_gravity(volumes, volumeCount);
  }
  for (; curTime < at_time; ++cur_tick, curTime = float(cur_tick) * dt)
  {
    TMatrix tm = TMatrix::IDENT;
    location.toTM(tm);
    Quat invOrient = inverse(location.O.getQuat());
    Point3 pos = tm.getcol(3);

    float waterDists[gamephys::floating_volumes::MAX_VOLUMES];
    for (int i = 0; i < volumeCount; ++i)
    {
      Point3 volumeWorldPos = tm * volumes[i].c;
      bool underwater;
      float shallowHt = volumes[i].r + 0.1f;
      float waterHeight = dacoll::traceht_water_at_time(volumeWorldPos, 200.f, curTime, underwater, shallowHt);
      if (waves_amplitude != 0.0f)
      {
        Point2 wavesShift = curTime * waves_velocity;
        float noise = get_water_ripples_noise(Point3((volumeWorldPos.x - wavesShift.x) * waves_inv_length,
          (volumeWorldPos.z - wavesShift.y) * waves_inv_length, curTime * waves_inv_period));
        waterHeight += noise * waves_amplitude;
      }
      waterDists[i] = volumeWorldPos.y - waterHeight;
    }

    gamephys::floating_volumes::VolumeParams params;
    params.invMass = inv_mass;
    params.invMomOfInertia = inv_moment_of_inertia;
    params.invOrient = invOrient;
    params.canTraceWorld = true;
    params.location = location;
    params.velocity = velocity;
    params.omega = omega;
    params.gravityCenter = center_of_gravity;

    DPoint3 addVel = DPoint3(0, 0, 0);
    DPoint3 addOmega = DPoint3(0, 0, 0);
    gamephys::floating_volumes::update(volumes, volumeCount, spheres_viscosity, dt, params, waterDists, addVel, addOmega);
    Point3 gVel = Point3(0.f, -gamephys::atmosphere::g(), 0.f) * dt;
    velocity += location.O.getQuat() * addVel;
    velocity += gVel;
    omega += addOmega;
    Point3 orientationInc = omega * dt;
    Quat quatInc = Quat(orientationInc, orientationInc.length());
    prevLoc = location;

    // Rotate relative to the gravity center.
    location.P += location.O.getQuat() * dpoint3(params.gravityCenter);
    location.O.setQuat(normalize(location.O.getQuat() * quatInc));
    location.P -= location.O.getQuat() * dpoint3(params.gravityCenter);

    if (interaction_type != rendinstfloating::NO_INTERACTION)
    {
      Point3 addInteractVel(0, 0, 0);
      Point3 width = phys_bbox.width();
      float size = 0.5f * ::sqrtf(width.x * width.x + width.z * width.z);
      if (interaction_type == rendinstfloating::ELASTIC_STRING)
      {
        Point3 dx = -pos + phys.staticLoc.P;
        dx.y = 0;
        // magic formula for dx. It is no physically correct, just empirical
        float ldx = length(dx);
        static const float BIAS = 1e-4;
        if (ldx > BIAS)
        {
          dx = elasticity * (1 + ::sqr(::sqr(clamp(ldx - 1, 0.0f, 10.0f))) / ldx) * dx;
          pos += dx;
          addInteractVel += dx / dt;
        }
      }
      for (int i = 0; i < obstacles_cnt; ++i)
      {
        const Obstacle &obstacle = obstacles[i];
        Point3 r = pos - obstacle.pos;
        r.y = 0.0f;
        float len = length(r);
        float maxInteractionDist = obstacle.size + size;
        if (len < maxInteractionDist)
        {
          Point3 cr = cross(obstacle.dir, Point3(0, 1, 0));
          if (dot(cr, r) < 0)
            cr = -cr;
          pos += cr;
          addInteractVel += cr / dt;
          Point3 delta = ((maxInteractionDist - len) / len) * r;
          addInteractVel += delta / dt;
        }
      }
      addInteractVel.y = 0.0; // We want interactions to be only horizontal.
      float addInteractVelAmount = length(addInteractVel);
      addInteractVel =
        addInteractVel / (addInteractVelAmount + 1e-6f) * clamp(addInteractVelAmount, 0.0f, MAX_ADD_INTERACTION_VELOCITY);
      velocity += addInteractVel;
    }
    location.P += velocity * dt;
    if (interaction_type == rendinstfloating::ELASTIC_STRING)
    {
      DPoint3 shift = location.P - phys.staticLoc.P;
      double shiftDist = length(shift);
      location.P = phys.staticLoc.P + shift * min(1.0, double(max_shift_dist) / (shiftDist + 1e-6 /* prevent zero-division */));
    }
  }

  const float alpha = clamp((at_time - (phys.curTick - 1) * dt) / dt, 0.f, 1.f);
  gamephys::Loc loc;
  loc.interpolate(prevLoc, location, alpha);
  velocity.x = velocity.z = 0.f;
  phys.visualLoc = loc;
}

static ecs::EntityId find_floatable_group_for_res(const char *res_name, ecs::EntityId exclude_eid = ecs::INVALID_ENTITY_ID)
{
  ecs::EntityId result = ecs::INVALID_ENTITY_ID;
  find_floatable_group_for_res_ecs_query([&](ecs::EntityId eid, const ecs::string &floatingRiGroup__resName) {
    if (eid != exclude_eid && floatingRiGroup__resName == res_name)
      result = eid;
  });
  return result;
}

template <typename F>
static inline void for_each_floatable_ri_blk(const DataBlock &ri_config, bool use_subblock_name, const F cb)
{
  for (int j = 0, n = ri_config.blockCount(), nid = ri_config.getNameId("dmg"); j < n; j++)
  {
    if (!use_subblock_name && (ri_config.getBlock(j)->getBlockNameId() != nid))
      continue;

    const DataBlock *b = ri_config.getBlock(j);
    if (!b->getBool("floatable", false))
      continue;

    const char *riResName = use_subblock_name ? b->getBlockName() : b->getStr("name", "");
    if (rendinst::getRIGenExtraResIdx(riResName) < 0)
      continue;

    if (!cb(riResName, b))
      break;
  }
}

static void init_floating_ri_res_groups_impl(const DataBlock &ri_config, bool use_subblock_name)
{
  for_each_floatable_ri_blk(ri_config, use_subblock_name, [&](const char *riResName, const DataBlock *b) {
    if (find_floatable_group_for_res(riResName) != ecs::INVALID_ENTITY_ID)
      return true; // Floating groups, created through level entities, have higher priority.

    int interactionType = b->getInt("interactionType", 0);

    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, floatingRiGroup__resName, ecs::string(riResName));
    ECS_INIT(attrs, floatingRiGroup__inertiaMult, b->getPoint3("inertiaMult", Point3(80.f, 0.1f, 80.f)));
    ECS_INIT(attrs, floatingRiGroup__volumesCount, b->getInt("volumesCount", -1));
    ECS_INIT(attrs, floatingRiGroup__volumePresetName, ecs::string(b->getStr("volumePresetName", "")));
    ECS_INIT(attrs, floatingRiGroup__wreckageFloatDuration, b->getReal("ttf", 15.f));
    ECS_INIT(attrs, floatingRiGroup__updateDistSq, sqr(b->getReal("updateDist", 1000.f)));
    ECS_INIT(attrs, floatingRiGroup__interactionType, interactionType);
    ECS_INIT(attrs, floatingRiGroup__interactionDistSq,
      (interactionType == rendinstfloating::NO_INTERACTION) ? 0.0f : sqr(b->getReal("interationDist", 50.f)));
    ECS_INIT(attrs, floatingRiGroup__elasticity, b->getReal("elasticity", 0.1f));
    ECS_INIT(attrs, floatingRiGroup__physUpdateDt, 1.0f / max(b->getReal("physUpdateRate", 24.0f), 1.0f));
    ECS_INIT(attrs, floatingRiGroup__maxShiftDist, max(0.0f, b->getReal("maxShiftDist", 1000.0f)));
    ECS_INIT(attrs, floatingRiGroup__density, max(0.0f, b->getReal("density", 500.0f)));
    ECS_INIT(attrs, floatingRiGroup__densityRandRange, max(0.0f, b->getReal("densityRandRange", 0.0f)));
    ECS_INIT(attrs, floatingRiGroup__useBoxInertia, b->getBool("useBoxInertia", false));
    ECS_INIT(attrs, floatingRiGroup__viscosity, max(0.0f, b->getReal("viscosity", 0.57f)));
    ECS_INIT(attrs, floatingRiGroup__minDistToGround, b->getReal("minDistToGround", -1e6f));

    g_entity_mgr->createEntitySync("floating_rendinst_res_group", eastl::move(attrs));

    return true;
  });
}

void rendinstfloating::init_floating_ri_res_groups(const DataBlock *ri_config, bool use_subblock_name)
{
  if (!ri_config)
    return;

  for_each_floatable_ri_blk(*ri_config, use_subblock_name, [&](const char *, const DataBlock *) {
    DataBlock cfg = *ri_config;
    delayed_call([cfg = eastl::move(cfg), use_subblock_name]() { init_floating_ri_res_groups_impl(cfg, use_subblock_name); });
    return false;
  });
}

ECS_AFTER(start_async_phys_sim_es) // after start_async_phys_sim_es to start phys sim job earlier
ECS_TAG(gameClient)
static void update_floating_rendinsts_es(const ParallelUpdateFrameDelayed &info, float floatingRiSystem__randomWavesAmplitude,
  float floatingRiSystem__randomWavesLength, float floatingRiSystem__randomWavesPeriod, Point2 floatingRiSystem__randomWavesVelocity)
{
  if (!dacoll::get_water())
    return;

  bool prepared = false;
  vec3f viewPos = get_camera_pos();
  static float maxUpdateDistSq = 0.0f; // Will be one frame delay, but it's ok. TODO: move to floating_rendinst_res_group component
  dag::Vector<Obstacle, framemem_allocator> obstacles;
  float wavesAmplitude = 0.0f, wavesInvLength = 1.0f, wavesInvPeriod = 1.0f, waterLevel = 0.0f;
  Point2 wavesVelocity(0, 0);

  constexpr float MIN_ANGLE_DIFF_SQ = PI * 1e-6f;
  constexpr float MIN_POS_DIFF_SQ = 1e-6f;
  update_floating_rendinst_instances_ecs_query([&](float floatingRiGroup__wreckageFloatDuration, float floatingRiGroup__updateDistSq,
                                                 int floatingRiGroup__interactionType, float floatingRiGroup__interactionDistSq,
                                                 float floatingRiGroup__elasticity, float floatingRiGroup__physUpdateDt,
                                                 float floatingRiGroup__maxShiftDist, float floatingRiGroup__viscosity,
                                                 float floatingRiGroup__minDistToGround,
                                                 rendinstfloating::PhysFloatingModel &floatingRiGroup__riPhysFloatingModel) {
    int resIdx = floatingRiGroup__riPhysFloatingModel.resIdx;
    if (resIdx < 0)
      return;

    maxUpdateDistSq = max(maxUpdateDistSq, floatingRiGroup__updateDistSq);

    int removedCnt;
    {
      rendinst::ScopedRIExtraReadLock rlock;
      dag::ConstSpan<mat43f> riTm = rendinst::riex_get_instance_matrices(resIdx);
      int &processedRiTmCount = floatingRiGroup__riPhysFloatingModel.processedRiTmCount;
      if (riTm.size() != processedRiTmCount)
      {
        if (EASTL_LIKELY(!processedRiTmCount))
        {
          bool foundClose = false;
          vec4f udsq = v_set_x(floatingRiGroup__updateDistSq);
          // Note: can be further optimized by iterating like
          // for (int riInstId = dagor_frame_no() % 3; rendinst < riTm.size(); riInstId += 3)
          for (const mat43f &m : riTm)
          {
            vec3f ripos = v_perm_xyab(v_perm_xaxa(v_splat_w(m.row0), v_splat_w(m.row1)), v_splat_w(m.row2));
            if (v_test_vec_x_le(v_length3_sq_x(v_sub(viewPos, ripos)), udsq))
            {
              foundClose = true;
              break;
            }
          }
          if (!foundClose)
            return;
        }

        for (int riInstId = processedRiTmCount; riInstId < riTm.size(); ++riInstId)
        {
          if (rendinst::riex_is_instance_valid(riTm[riInstId]) && is_ri_on_water(resIdx, riInstId) &&
              !is_ri_on_ground(resIdx, riInstId, floatingRiGroup__minDistToGround))
          {
            floatingRiGroup__riPhysFloatingModel.instances.push_back(
              create_ri_floating_phys(riTm[riInstId], riInstId, floatingRiGroup__riPhysFloatingModel.physBbox));
          }
        }

        processedRiTmCount = riTm.size();
      }
      removedCnt = remove_invalid_floating_phys_instances(floatingRiGroup__riPhysFloatingModel.instances, riTm);
    }

    if (EASTL_UNLIKELY(!prepared))
    {
      obstacles = get_obstacles(viewPos, maxUpdateDistSq);
      waterLevel = get_water_level();

      wavesAmplitude = floatingRiSystem__randomWavesAmplitude;
      wavesInvLength = 1.0f / floatingRiSystem__randomWavesLength;
      wavesInvPeriod = 1.0f / floatingRiSystem__randomWavesPeriod;
      wavesVelocity = floatingRiSystem__randomWavesVelocity;

      prepared = true;
    }

    for (auto &riFloatingPhys : floatingRiGroup__riPhysFloatingModel.instances)
    {
      int riInstId = riFloatingPhys.riInstId;
      Point3 pos = riFloatingPhys.location.P;
      enum
      {
        HASH_RES_IDX_BITS = 12,
        HASH_RES_IDX_MASK = (1 << HASH_RES_IDX_BITS) - 1
      };
      float instNoise = float(double(hash_int((uint32_t(riInstId) << HASH_RES_IDX_BITS) + (resIdx & HASH_RES_IDX_MASK))) / UINT32_MAX);

      // Randomly shift curTime within dt to balance updates between frames.
      float localObjectAtTime = info.curTime + instNoise * floatingRiGroup__physUpdateDt;
      vec3f viewDistSq = v_length3_sq_x(v_sub(v_ldu(&pos.x), viewPos));
      if (v_test_vec_x_gt(viewDistSq, v_set_x(floatingRiGroup__updateDistSq)))
      {
        // Skip time to continue simulation from the last state and prevent
        // simulating through the whole inactive time.
        riFloatingPhys.curTick = gamephys::nearestPhysicsTickNumber(localObjectAtTime, floatingRiGroup__physUpdateDt) + 1;
        continue;
      }
      int interactionType = v_test_vec_x_lt(viewDistSq, v_set_x(floatingRiGroup__interactionDistSq))
                              ? floatingRiGroup__interactionType
                              : rendinstfloating::NO_INTERACTION;

      float mass = lerp(floatingRiGroup__riPhysFloatingModel.randMassMin, floatingRiGroup__riPhysFloatingModel.randMassMax, instNoise);
      float invMass = 1.0f / mass;

      update_floating_phys(localObjectAtTime, riFloatingPhys.curTick, interactionType, invMass, floatingRiGroup__elasticity,
        floatingRiGroup__physUpdateDt, floatingRiGroup__maxShiftDist,
        floatingRiGroup__riPhysFloatingModel.invMomentOfInertiaCoeff * invMass, floatingRiGroup__riPhysFloatingModel.spheresRad,
        floatingRiGroup__viscosity, floatingRiGroup__riPhysFloatingModel.spheresCoords, floatingRiGroup__riPhysFloatingModel.physBbox,
        obstacles.data(), obstacles.size(), wavesAmplitude, wavesInvLength, wavesInvPeriod, wavesVelocity, riFloatingPhys);

      // Moving rendinst is too expensive, so check here if it was really moved.
      Quat orientDiffQuat = riFloatingPhys.visualLoc.O.getQuat() - riFloatingPhys.prevVisualLoc.O.getQuat();
      Point3 orientDiff(orientDiffQuat.x, orientDiffQuat.y, orientDiffQuat.z);
      Point3 posDiff = riFloatingPhys.visualLoc.P - riFloatingPhys.prevVisualLoc.P;
      if (dot(orientDiff, orientDiff) < MIN_ANGLE_DIFF_SQ && dot(posDiff, posDiff) < MIN_POS_DIFF_SQ)
        continue;

      TMatrix tm;
      riFloatingPhys.visualLoc.toTM(tm);
      tm.setcol(0, tm.getcol(0) * riFloatingPhys.scale.x);
      tm.setcol(1, tm.getcol(1) * riFloatingPhys.scale.y);
      tm.setcol(2, tm.getcol(2) * riFloatingPhys.scale.z);
      TMatrix4_vec4 m4 = TMatrix4_vec4(tm);
      rendinst::moveRIGenExtra44(rendinst::make_handle(resIdx, riInstId), (mat44f &)m4, /*moved*/ true, /*do_not_lock*/ false);
      riFloatingPhys.prevVisualLoc = riFloatingPhys.visualLoc;
    }

    if (removedCnt)
    {
      for (auto destr : destructables::getDestructableObjects())
      {
        if (destr->resIdx == resIdx && !destr->isFloatable())
        {
          if (destr->isAlive())
          {
            destr->setTimeToFloat(floatingRiGroup__wreckageFloatDuration);
            gamephys::DestructableObject::numFloatable++;
            rendinstdestr::remove_restorable_by_destructable_id(destr->getId());
          }
          break;
        }
      }
    }
  });
}

ECS_BEFORE(start_async_phys_sim_es)
ECS_TAG(gameClient)
static void keep_floatable_destructables_es(const ParallelUpdateFrameDelayed &info)
{
  if (!gamephys::DestructableObject::numFloatable)
    return;
  int numFloatable = 0;
  for (auto destr : destructables::getDestructableObjects())
  {
    if (destr->isAlive() && destr->isFloatable())
    {
      destr->keepFloatable(info.dt, info.curTime);
      numFloatable++;
    }
  }
  gamephys::DestructableObject::numFloatable = numFloatable;
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(floatingRiGroup__resName, floatingRiGroup__inertiaMult, floatingRiGroup__volumesCount, floatingRiGroup__volumePresetName,
  floatingRiGroup__density, floatingRiGroup__densityRandRange, floatingRiGroup__useBoxInertia)
ECS_TAG(gameClient)
static __forceinline void init_floating_rendinst_res_group_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  const ecs::string &floatingRiGroup__resName, const Point3 &floatingRiGroup__inertiaMult, int floatingRiGroup__volumesCount,
  ecs::string floatingRiGroup__volumePresetName, float floatingRiGroup__density, float floatingRiGroup__densityRandRange,
  bool floatingRiGroup__useBoxInertia, rendinstfloating::PhysFloatingModel &floatingRiGroup__riPhysFloatingModel)
{
  if (ecs::EntityId otherEid = find_floatable_group_for_res(floatingRiGroup__resName.c_str(), eid))
    g_entity_mgr->destroyEntity(otherEid); // There must be no more than one group for each resource.

  g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("floating_rendinst_system"));

  int resIdx = rendinst::getRIGenExtraResIdx(floatingRiGroup__resName.c_str());
  if (resIdx < 0)
  {
    resIdx = rendinst::addRIGenExtraResIdx(floatingRiGroup__resName.c_str(), -1, -1,
      rendinst::AddRIFlag::UseShadow | rendinst::AddRIFlag::Dynamic);
    if (resIdx < 0)
    {
      logerr("Failed to create floating rendinst group, rendinst with name %s isn't presented", floatingRiGroup__resName.c_str());
      g_entity_mgr->destroyEntity(eid);
      return;
    }
  }
  floatingRiGroup__riPhysFloatingModel.resIdx = resIdx;

  auto *rendInstRes = rendinst::getRIGenExtraRes(resIdx);
  const BBox3 *riFloatingBox = rendInstRes ? rendInstRes->getFloatingBox() : nullptr;

  BBox3 box;
  if (riFloatingBox)
  {
    box = *riFloatingBox;
  }
  else
  {
    bbox3f lbb = rendinst::riex_get_lbb(resIdx);
    v_stu_bbox3(box, lbb);
    box.lim[0].y = -1; // For backward compatibility. To be replaced after assets are setted up.
    box.lim[1].y = 1;
  }
  floatingRiGroup__riPhysFloatingModel.physBbox = box;

  construct_floating_volumes_ecs_query([&](const ecs::Object &floatingRiSystem__volumePresets) {
    floatingRiGroup__riPhysFloatingModel.spheresCoords =
      extract_sphere_coords(floatingRiSystem__volumePresets, floatingRiGroup__volumesCount, floatingRiGroup__volumePresetName);
  });
  const int nVolumes = floatingRiGroup__riPhysFloatingModel.spheresCoords.size();

  constexpr float waterDensity = 1000.0f;
  constexpr float initialDensityMin = 0.0f;
  constexpr float initialDensityMax = waterDensity;

  // For physics stability.
  // And to be able to make object stand on water at 0 density and be just underwater on 1000 density
  // (that's what artists actually want and they don't care about physical meanings).
  constexpr float addRadius = 0.15f;

  constexpr float sphereVolumeCf = 4.0 / 3.0 * PI;

  float rInit = (box.lim[1].y - box.lim[0].y) * 0.5f;
  float rExt = rInit + addRadius;
  float hMin = rExt - rInit;
  float hMax = rExt + rInit;

  float rExt3 = pow(rExt, 3);
  // We have to recalculate acceptable density range to preserve object position on water.
  float extendedDensityMin = waterDensity * hMin * hMin * (3.0f * rExt - hMin) / (4.0f * rExt3);
  float extendedDensityMax = waterDensity * hMax * hMax * (3.0f * rExt - hMax) / (4.0f * rExt3);

  float sphereVolume = sphereVolumeCf * rExt3;
  float totalVolume = sphereVolume * nVolumes;
  float randDensityMin = remap_range_clamp(floatingRiGroup__density - floatingRiGroup__densityRandRange, initialDensityMin,
    initialDensityMax, extendedDensityMin, extendedDensityMax);
  float randDensityMax = remap_range_clamp(floatingRiGroup__density + floatingRiGroup__densityRandRange, initialDensityMin,
    initialDensityMax, extendedDensityMin, extendedDensityMax);

  floatingRiGroup__riPhysFloatingModel.randMassMin = totalVolume * randDensityMin;
  floatingRiGroup__riPhysFloatingModel.randMassMax = totalVolume * randDensityMax;
  floatingRiGroup__riPhysFloatingModel.spheresRad = rExt;

  Point3 momentOfInertiaCoeff = Point3(1, 1, 1);
  if (floatingRiGroup__useBoxInertia)
  {
    Point3 bboxSize = box.width();
    momentOfInertiaCoeff = Point3(1.0f / 12.0f * (sqr(bboxSize.y) + sqr(bboxSize.z)) * floatingRiGroup__inertiaMult.x,
      1.0f / 12.0f * (sqr(bboxSize.z) + sqr(bboxSize.x)) * floatingRiGroup__inertiaMult.y,
      1.0f / 12.0f * (sqr(bboxSize.x) + sqr(bboxSize.y)) * floatingRiGroup__inertiaMult.z);
  }
  else
  {
    float height = box.lim[1].y - box.lim[0].y;
    float radius = rendinst::ries_get_bsph_rad(resIdx);
    momentOfInertiaCoeff = Point3(floatingRiGroup__inertiaMult.x * (3.f * sqr(radius) + sqr(height)) / 12.f,
      floatingRiGroup__inertiaMult.y * sqr(radius) * 0.5f, floatingRiGroup__inertiaMult.z * (3.f * sqr(radius) + sqr(height)) / 12.f);
  }
  floatingRiGroup__riPhysFloatingModel.invMomentOfInertiaCoeff =
    Point3(safeinv(momentOfInertiaCoeff.x), safeinv(momentOfInertiaCoeff.y), safeinv(momentOfInertiaCoeff.z));
}

ECS_ON_EVENT(on_appear)
ECS_TAG(gameClient)
static void init_floating_volume_presets_es_event_handler(const ecs::Event &, ecs::Object &floatingRiSystem__volumePresets,
  ecs::Object &floatingRiSystem__userVolumePresets)
{
  for (auto &preset : floatingRiSystem__userVolumePresets)
    floatingRiSystem__volumePresets.addMember(preset.first.c_str(), preset.second.get<ecs::Array>());
  floatingRiSystem__userVolumePresets.clear();
}

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void rendinst_floating_render_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (!render_debug_floating_volumes.get())
    return;

  begin_draw_cached_debug_lines();
  draw_rendinst_floating_volumes_ecs_query([&](const rendinstfloating::PhysFloatingModel &floatingRiGroup__riPhysFloatingModel) {
    int resIdx = floatingRiGroup__riPhysFloatingModel.resIdx;
    if (resIdx < 0)
      return;

    BSphere3 volumes[gamephys::floating_volumes::MAX_VOLUMES];
    for (const auto &instance : floatingRiGroup__riPhysFloatingModel.instances)
    {
      TMatrix tm;
      instance.visualLoc.toTM(tm);

      restore_floating_volumes(floatingRiGroup__riPhysFloatingModel.spheresCoords, floatingRiGroup__riPhysFloatingModel.spheresRad,
        floatingRiGroup__riPhysFloatingModel.physBbox, instance, volumes);

      for (const auto &volume : volumes)
        draw_cached_debug_sphere(tm * volume.c, volume.r, E3DCOLOR_MAKE(0, 128, 255, 255));

      Point3 boxSize = floatingRiGroup__riPhysFloatingModel.physBbox.width();
      Point3 boxOrigin = tm * floatingRiGroup__riPhysFloatingModel.physBbox.boxMin();
      Point3 boxAx = tm.getcol(0) * boxSize.x;
      Point3 boxAy = tm.getcol(1) * boxSize.y;
      Point3 boxAz = tm.getcol(2) * boxSize.z;
      draw_cached_debug_box(boxOrigin, boxAx, boxAy, boxAz, E3DCOLOR_MAKE(128, 128, 128, 255));
    }
  });
  end_draw_cached_debug_lines();
}

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void floating_phys_ripples_render_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (!render_debug_floating_phys_ripples.get())
    return;

  begin_draw_cached_debug_lines();
  draw_floating_phys_ripples_ecs_query([&](float floatingRiSystem__randomWavesAmplitude, float floatingRiSystem__randomWavesLength,
                                         float floatingRiSystem__randomWavesPeriod, Point2 floatingRiSystem__randomWavesVelocity) {
    float curTime = float(get_time_msec()) * 1e-3f;
    vec3f vViewPos = get_camera_pos();
    float waterLevel = get_water_level();

    float amplitude = floatingRiSystem__randomWavesAmplitude;
    float invLength = 1.0f / floatingRiSystem__randomWavesLength;
    float invPeriod = 1.0f / floatingRiSystem__randomWavesPeriod;

    constexpr float dist = 10.0f;
    constexpr int nPoints = 100;
    constexpr float step = dist / float(nPoints);

    Point2 shiftXZ = floatingRiSystem__randomWavesVelocity * curTime;
    Point2 subShiftXZ = Point2(frac(shiftXZ.x / step), frac(shiftXZ.y / step)) * step;
    Point2 centerXZ = Point2(floor(v_extract_x(vViewPos) / step) * step, floor(v_extract_z(vViewPos) / step) * step) - subShiftXZ;

    for (int x = -nPoints; x <= nPoints; ++x)
    {
      for (int y = -nPoints; y <= nPoints; ++y)
      {
        Point2 p0 = centerXZ + Point2(x, y) * step;
        float noise0 =
          get_water_ripples_noise(Point3((p0.x + shiftXZ.x) * invLength, (p0.y + shiftXZ.y) * invLength, curTime * invPeriod));
        if (x < nPoints)
        {
          Point2 p1 = centerXZ + Point2(x + 1, y) * step;
          float noise1 =
            get_water_ripples_noise(Point3((p1.x + shiftXZ.x) * invLength, (p1.y + shiftXZ.y) * invLength, curTime * invPeriod));
          draw_cached_debug_line(Point3(p0.x, waterLevel + noise0 * amplitude, p0.y),
            Point3(p1.x, waterLevel + noise1 * amplitude, p1.y), E3DCOLOR_MAKE(0, 255, 128, 255));
        }
        if (y < nPoints)
        {
          Point2 p2 = centerXZ + Point2(x, y + 1) * step;
          float noise2 =
            get_water_ripples_noise(Point3((p2.x + shiftXZ.x) * invLength, (p2.y + shiftXZ.y) * invLength, curTime * invPeriod));
          draw_cached_debug_line(Point3(p0.x, waterLevel + noise0 * amplitude, p0.y),
            Point3(p2.x, waterLevel + noise2 * amplitude, p2.y), E3DCOLOR_MAKE(0, 255, 128, 255));
        }
      }
    }
  });
  end_draw_cached_debug_lines();
}
