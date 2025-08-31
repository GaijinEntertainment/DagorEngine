// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/destructableRendObject.h>
#include <EASTL/algorithm.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_geomTree.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <phys/dag_physics.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <gameRes/dag_resourceNameResolve.h>
#include <shaders/dag_dynSceneRes.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/physLayers.h>
#include <gamePhys/props/atmosphere.h>

using namespace gamephys;

namespace destructables
{
static int default_fgroup = dacoll::EPL_DEFAULT;
static constexpr int default_fmask = dacoll::EPL_ALL;
} // namespace destructables

DestructableObject::DestructableObject(const destructables::DestructableCreationParams &params, PhysWorld *phys_world,
  float scale_dt) :
  scaleDt(scale_dt),
  physObj(
    DynamicPhysObject::create(params.physObjData, phys_world, params.tm, destructables::default_fgroup, destructables::default_fmask)),
  resIdx(params.resIdx)
{
  mat44f tm44;
  v_mat44_make_from_43cu_unsafe(tm44, params.tm.array);
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm44);
  v_stu(&intialTmAndHash[0].x, m43.row0);
  v_stu(&intialTmAndHash[1].x, m43.row1);
  v_stu(&intialTmAndHash[2].x, m43.row2);
  memcpy(&intialTmAndHash[3].x, &params.hashVal, sizeof(params.hashVal));

  if (params.timeToLive >= 0.0f)
    ttl = params.timeToLive;
  if (params.defaultTimeToLive >= 0.0f)
    defaultTimeToLive = params.defaultTimeToLive;
  if (params.timeToKinematic >= 0.0f)
    timeToKinematic = params.timeToKinematic;
  if (params.timeToSinkUnderground > 0.0f)
    timeToSinkUnderground = params.timeToSinkUnderground;

  BBox3 bbox;
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); i++)
  {
    Point3 bodyMin, bodyMax;
    TMatrix bodyTm;
    auto *body = physObj->getPhysSys()->getBody(i);
    body->getTm(bodyTm);
    body->getShapeAabb(bodyMin, bodyMax);
    bbox += bodyTm * bodyMin;
    bbox += bodyTm * bodyMax;
  }
  bboxHeight = bbox.width().y;

  clear_and_resize(ttlBodies, physObj->getPhysSys()->getBodyCount());
  for (int i = 0; i < ttlBodies.size(); ++i)
    ttlBodies[i] = defaultTimeToLive;
  rendData.reset(destructables::init_rend_data(physObj.get(), params.isDestroyedByExplosion));
}

int DestructableObject::getNumActiveBodies() const { return ttlBodies.size(); }

bool DestructableObject::hasInteractableBodies() const
{
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    if (body->getInteractionLayer() != 0 && body->isInWorld())
      return true;
  }
  return false;
}

struct gamephys::DestructableObjectAddImpulse final : public AfterPhysUpdateAction
{
  // Note: it's okay to use direct ref since it would be pointing to pool's memory (`destructablesListAllocator`)
  DestructableObject &dobj;
  int gen;
  float speedLimit, omegaLimit;
  Point3 pos, impulse;

  DestructableObjectAddImpulse(DestructableObject &dobj_, const Point3 &p, const Point3 &i, float sl, float ol) :
    dobj(dobj_), gen(dobj_.gen), pos(p), impulse(i), speedLimit(sl), omegaLimit(ol)
  {}

  void doAction(PhysWorld &, bool) override
  {
    if (gen == dobj.gen) // Wasn't destroyed?
      dobj.doAddImpulse(pos, impulse, speedLimit, omegaLimit);
  }
};

void DestructableObject::addImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit)
{
  exec_or_add_after_phys_action<DestructableObjectAddImpulse>(pw, *this, pos, impulse, speedLimit, omegaLimit);
}

void DestructableObject::doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit)
{
  float impulseLen = length(impulse);
  const Point3 impulseDir = impulse * safeinv(impulseLen);

  const float minImpulseLen = 15.;
  if (impulseLen > 0. && impulseLen < minImpulseLen)
    impulseLen = minImpulseLen;

  const float maxImpulseMul = 15; // we use mass for calculating impulse mul. Because big parts isn't react for min bullets
  const float maxDirCorrection = 0.8;
  const float dirCorrectionPerMeter = 0.3; // we change angle for a more beautiful scattering of fragments
  const float impulseMulPerMeter = 15;     // we use invarian for calculating momentum divergence (more inpulse for close parts to pos)
  const float maxRadius = 1.5;
  const float radius = cvt(impulseLen, 0., 300., 0., maxRadius); // For larger impulses, we change the position of applying the impulse
                                                                 // relative to body

  for (int i = 0, n = physObj->getPhysSys()->getBodyCount(); i < n; ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);

    float mass = body->getMass(); // we use mass for calculating umpulse mul
    float impulseMul = min(mass, maxImpulseMul);

    TMatrix bodyTm;
    body->getTm(bodyTm); // better use center of mass

    Point3 dirToBody = bodyTm.getcol(3) - pos;
    float distToBody = length(dirToBody);

    float dirCorrection = min(dirCorrectionPerMeter * distToBody, maxDirCorrection);
    Point3 partImpulseDir = impulseDir + (dirCorrection * (dirToBody * safeinv(distToBody)));
    float impulseForce = impulseLen * safeinv(impulseMulPerMeter * distToBody) * impulseMul;
    float relRad = 1.f - cvt(distToBody, radius, maxRadius, 0.1, 0.9);

    Point3 applyImpulsePos = pos + dirToBody * relRad;
    body->addImpulse(applyImpulsePos, partImpulseDir * impulseForce);

    const Point3 omega = body->getAngularVelocity(); // clamp omega and velocity for parts
    const float omegaMagnitude = omega.length();
    if (omegaMagnitude > omegaLimit)
      body->setAngularVelocity(omega * safediv(omegaLimit, omegaMagnitude));

    const Point3 velocity = body->getVelocity();
    const float velMagnitude = velocity.length();
    if (velMagnitude > speedLimit)
      body->setVelocity(velocity * safediv(speedLimit, velMagnitude));
  }
}

void DestructableObject::keepFloatable(float dt, float at_time)
{
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    float mass = body->getMass();
    TMatrix bodyTm;
    body->getTm(bodyTm);
    Point3 pos = bodyTm.getcol(3);

    bool underwater = false;
    float waterHt = dacoll::traceht_water_at_time(pos, 0.5f, at_time, underwater);
    if (!dacoll::is_valid_water_height(waterHt))
      continue;

    float waterDist = pos.y - waterHt;
    if (waterDist < 0.0f && timeToFloat > 2.0f)
    {
      const float defDensityRatio = 1.2f;
      float fa = min(sqr(waterDist), sqr(-1.0f)) * defDensityRatio;
      Point3 impulse = Point3(0.f, fa * mass * gamephys::atmosphere::g() * dt, 0.f);
      body->addImpulse(pos, impulse);
    }
  }
}

bool DestructableObject::update(float dt, float dt_scale, bool force_update_ttl)
{
  TIME_PROFILE(DestructableObject__update);

  float updateDt = dt * scaleDt;
  bool forceUpdateTtl = force_update_ttl && scaleDt > 1.f || ttl < defaultTimeToLive + timeToSinkUnderground;
  for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
  {
    PhysBody *body = physObj->getPhysSys()->getBody(i);
    if (!body->isInWorld())
      continue;
    if (body->isActive() && ttlBodies[i] >= 0.f && !forceUpdateTtl)
    {
      ttlBodies[i] = defaultTimeToLive;
    }
    else
    {
      if (updateDt > ttlBodies[i] && ttlBodies[i] >= 0.f)
      {
        // calculate acceleration, required to fully sink object bounding box
        const float sinkUndergroundGravity =
          eastl::max(0.1f, bboxHeight / sqr(timeToSinkUnderground / (scaleDt * dt_scale)) * 2.0f / /* adjust for damping */ 0.8f);
        // Disable physics, just play animation
        body->setGroupAndLayerMask(0, 0);
        body->setGravity(Point3(0.f, -sinkUndergroundGravity, 0.f));
      }
      if (ttlBodies[i] < 0.f)
        body->activateBody(true);
      ttlBodies[i] -= updateDt;
    }
  }
  if (timeToKinematic >= 0.f)
  {
    timeToKinematic -= updateDt;
    if (timeToKinematic < 0.f)
    {
      for (int i = 0; i < physObj->getPhysSys()->getBodyCount(); ++i)
      {
        PhysBody *body = physObj->getPhysSys()->getBody(i);
        if (body->getInteractionLayer() && body->isInWorld())
          body->setGroupAndLayerMask(destructables::default_fgroup, destructables::default_fmask ^ dacoll::EPL_KINEMATIC);
      }
    }
  }
  ttl -= updateDt;
  if (timeToFloat >= 0.f)
    timeToFloat -= updateDt;
  return isAlive();
}

namespace destructables
{
struct DestructableObjectDeleter
{
  void operator()(gamephys::DestructableObject *object);
};
static dag::Vector<eastl::unique_ptr<DestructableObject, DestructableObjectDeleter>> destructablesList;
static FixedBlockAllocator destructablesListAllocator;
void DestructableObjectDeleter::operator()(DestructableObject *object)
{
  interlocked_increment(*(volatile int *)&object->gen); // Cast to disable DSE
  object->~DestructableObject();
  destructablesListAllocator.freeOneBlock(object);
}
int maxNumberOfDestructableBodies;
static int numActiveBodies;
static int numOfDestrBodiesForScaleDt;
static float distForScaleDtSq;
static float maxScaleDt;
float minDestrRadiusSq;
static float overflowReportTimeout = 0.0f;
#if DAGOR_DBGLEVEL > 0
static bool errorOnBodiesOverflow = false;
static unsigned minBodyCountForOverflowError = 10;
#else
static constexpr bool errorOnBodiesOverflow = false;
static constexpr unsigned minBodyCountForOverflowError = ~0u;
#endif

void init(const DataBlock *blk, int fgroup)
{
  const DataBlock *destrBlk = blk->getBlockByNameEx("destructables");
  maxNumberOfDestructableBodies = destrBlk->getInt("maxNumberOfDestructableBodies", 100);
  numOfDestrBodiesForScaleDt = destrBlk->getInt("numOfDestrBodiesForScaleDt", maxNumberOfDestructableBodies * 0.5);
  distForScaleDtSq = sqr(destrBlk->getReal("distForScaleDt", 100.f));
  maxScaleDt = destrBlk->getReal("maxScaleDt", 3.f);
  minDestrRadiusSq = sqr(destrBlk->getReal("minDestrRadius", 40.f));
#if DAGOR_DBGLEVEL > 0
  errorOnBodiesOverflow = destrBlk->getBool("errorOnBodiesOverflow", false);
  minBodyCountForOverflowError = destrBlk->getInt("minBodyCountForOverflowError", 10);
#endif
  default_fgroup = fgroup;
  if (!destructablesListAllocator.isInited())
    destructablesListAllocator.init(sizeof(DestructableObject), (4096 * 2 - 16) / sizeof(DestructableObject));
}

destructables::id_t addDestructable(gamephys::DestructableObject **out_destr, const DestructableCreationParams &params,
  PhysWorld *phys_world)
{
  G_ASSERT_RETURN(params.physObjData, nullptr);

  float scaleDt = params.scaleDt >= 0.0f ? params.scaleDt : 1.f;
  if (params.scaleDt < 0.0f && distForScaleDtSq > 0.f) //-V1051
  {
    float distSq = (params.camPos - params.tm.getcol(3)).lengthSq();
    if (distSq > distForScaleDtSq)
      scaleDt = clamp(sqrtf(distSq / distForScaleDtSq), 1.f, maxScaleDt);
  }

  void *mem = destructablesListAllocator.allocateOneBlock();
  DestructableObject *obj = nullptr;
  {
    TIME_PROFILE(addDestructable__DestructableObject_ctor);
    obj = new (mem, _NEW_INPLACE) DestructableObject(params, phys_world, scaleDt);
  }
  destructablesList.emplace_back(obj);
  if (out_destr)
    *out_destr = obj;
  return obj; // To consider: we can use high 16 bit of pointer for storing generation (for safety)
}

id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx, uint32_t hash_val, const DataBlock *blk)
{
  DestructableCreationParams params;
  params.physObjData = po_data;
  params.tm = tm;
  params.camPos = cam_pos;
  params.resIdx = res_idx;
  params.hashVal = hash_val;
  if (blk)
  {
    params.timeToLive = blk->getReal("timeToLive", -1.0f);
    params.defaultTimeToLive = blk->getReal("timeForBodies", -1.0f);
    params.timeToKinematic = blk->getReal("timeToKinematic", -1.0f);
  }
  return addDestructable(out_destr, params, phys_world);
}


void clear()
{
  clear_and_shrink(destructablesList);
  destructablesListAllocator.clear();
}

void removeDestructableById(id_t id)
{
  if (id != INVALID_ID && eastl::find(destructablesList.begin(), destructablesList.end(), id,
                            [](auto &rec, id_t id) { return rec.get() == id; }) != destructablesList.end())
    static_cast<DestructableObject *>(id)->markForDelete();
}

static void overflow_handler()
{
  if (!errorOnBodiesOverflow || destructablesList.size() > minBodyCountForOverflowError) //-V560
  {
    logwarn("destructables::update: too many destructable bodies %d, max - %d", numActiveBodies, maxNumberOfDestructableBodies);
    return;
  }
#if DAGOR_DBGLEVEL > 0
  String msg(framemem_ptr());
  msg.printf(0, "destructables::update: too many destructable bodies %d, max - %d", numActiveBodies, maxNumberOfDestructableBodies);
  for (const auto &i : destructablesList)
  {
    const auto *physObj = i->getPhysObj();
    G_ASSERT_CONTINUE(physObj);

    for (int n = 0; n < physObj->getModelCount(); ++n)
    {
      DynamicRenderableSceneInstance *inst = physObj->getModel(n);
      DynamicRenderableSceneLodsResource *lods = inst->getLodsResource();

      String name;
      String resMsg(framemem_ptr());
      resolve_game_resource_name(name, lods);
      resMsg.printf(0, "\n  res: %s, active bodies: %d", name.c_str(), i->getNumActiveBodies());
      msg.append(resMsg);
    }
  }
  logerr("%s", msg.c_str());
#endif
}

void update(float dt)
{
  TIME_PROFILE(destructables_update);

  const float dtUpdateScale = cvt(numActiveBodies, numOfDestrBodiesForScaleDt, maxNumberOfDestructableBodies, 1.f, maxScaleDt);
  const float dtUpdate = dt * dtUpdateScale;
  bool forceUpdateTtl = numOfDestrBodiesForScaleDt > 0 && numActiveBodies > numOfDestrBodiesForScaleDt;
  numActiveBodies = 0;
  int deleteLimit = DESTRUCTABLES_DELETE_MAX_PER_FRAME;
  destructablesList.erase(eastl::remove_if(destructablesList.begin(), destructablesList.end(),
                            [&](auto &object) {
                              if (!object->isAlive())
                              {
                                // delete objects marked at previous frames
                                if (deleteLimit-- > 0)
                                  return true; // ~18 deletions at 1 msec on AMD 5800H. Move to thread?
                              }
                              else if (object->update(dtUpdate, dtUpdateScale, forceUpdateTtl))
                                numActiveBodies += object->getNumActiveBodies();
                              // else it already considered marked for deletion
                              return false;
                            }),
    destructablesList.end());

  overflowReportTimeout -= dt;
  if (numActiveBodies > maxNumberOfDestructableBodies)
  {
    if (overflowReportTimeout <= 0.0f)
    {
      overflowReportTimeout = 1.0f;
      overflow_handler();
    }

    // Cleanup first those bodies that are falling through, then all old ones
    for (auto &object : destructablesList)
    {
      if (object->isAlive())
      {
        bool shouldDelete = !object->hasInteractableBodies() && numActiveBodies > maxNumberOfDestructableBodies;
        if (shouldDelete)
        {
          numActiveBodies -= object->getNumActiveBodies();
          object->markForDelete();
        }
      }
    }

    // First ones are oldest ones, as they were added in this order.
    if (numActiveBodies > maxNumberOfDestructableBodies)
    {
      for (auto &object : destructablesList)
      {
        if (object->isAlive())
        {
          bool shouldDelete = numActiveBodies > maxNumberOfDestructableBodies;
          if (shouldDelete)
          {
            numActiveBodies -= object->getNumActiveBodies();
            object->markForDelete();
          }
        }
      }
    }
  }
}

dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects()
{
  G_STATIC_ASSERT(sizeof(typename decltype(destructablesList)::value_type) == sizeof(gamephys::DestructableObject *));
  return dag::ConstSpan<gamephys::DestructableObject *>((gamephys::DestructableObject **)destructablesList.data(), //-V1032
    destructablesList.size());
}

} // namespace destructables
