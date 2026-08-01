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
#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <generic/dag_initOnDemand.h>
#include <gameRes/dag_resourceNameResolve.h>
#include <shaders/dag_dynSceneRes.h>
#include <render/dynmodelRenderer/animcharDisintegration.h>
#include <daFracture/render/renderMesh.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/collision/physLayers.h>
#include <gamePhys/props/atmosphere.h>


using namespace gamephys;


namespace destructables
{
struct DestructableObjectDeleter
{
  void operator()(gamephys::DestructableObject *object);
};

struct Context
{
  DestructablesConfig config;
  // note: allocator declaration before list, so DestructableObjectDeleter will be called before allocator is destroyed
  FixedBlockAllocator destructablesListAllocator;
  dag::Vector<eastl::unique_ptr<DestructableObject, DestructableObjectDeleter>> destructablesList;
  int numAliveBodies = 0;
  int numFloatable = 0;
  float overflowReportTimeout = 0.0f;

  int removeChecksNextIdx = 0;

  Context() { destructablesListAllocator.init(sizeof(DestructableObject), (4096 * 2 - 16) / sizeof(DestructableObject)); }
};

static InitOnDemand<Context, false> g_context;

void DestructableObjectDeleter::operator()(DestructableObject *object)
{
  interlocked_increment(*(volatile int *)&object->gen); // Cast to disable DSE
  object->~DestructableObject();
  g_context->destructablesListAllocator.freeOneBlock(object);
}
} // namespace destructables


FracturePhysObject::~FracturePhysObject() = default;


DestructableObject::DestructableObject(destructables::DestructableCreationParams &&params, PhysWorld *phys_world, //-V730
  float scale_dt) :
  scaleDt(scale_dt),
  physObj(params.physObjData ? DynamicPhysObject::create(params.physObjData, phys_world, params.tm,
                                 destructables::get_config().defaultFGroup, destructables::get_config().defaultFMask)
                             : nullptr),
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

  float timeToKinematic = 2.f;
  float ttl = 65.f;
  if (params.timeToLive >= 0.0f)
    ttl = params.timeToLive;
  if (params.inactiveTimeBeforeSink >= 0.0f)
    inactiveTimeBeforeSink = params.inactiveTimeBeforeSink;
  if (params.timeToKinematic >= 0.0f)
    timeToKinematic = params.timeToKinematic;
  if (params.timeToSinkUnderground > 0.0f)
    timeToSinkUnderground = params.timeToSinkUnderground;
  if (params.timeToStartDisintegration >= 0.0f)
    disintegrationTime = -params.timeToStartDisintegration;
  if (params.disintegrationDuration >= 0.0f)
    disintegrationParameters.duration = params.disintegrationDuration;
  constexpr float BASE_DISINTEGRATION_SCALE = 50.f; // This value looks good, so this will be the base value
  if (params.disintegrationScale >= 0.0f)
    disintegrationParameters.scale = params.disintegrationScale * BASE_DISINTEGRATION_SCALE;

  fracturePhysObjects = eastl::move(params.fracturePhysObjects);

  clear_and_resize(pieces, physObj ? physObj->getPhysSys()->getBodyCount() : fracturePhysObjects.size());
  for (int i = 0; i < pieces.size(); i++)
  {
    Piece &piece = pieces[i];
    piece.body = physObj ? physObj->getPhysSys()->getBody(i) : fracturePhysObjects[i].body.get();
    piece.timeToLive = ttl;
    piece.timeToKinematic = timeToKinematic;

    TMatrix bodyTm;
    piece.body->getTm(bodyTm);
    piece.visualLoc.fromTM(bodyTm);
    piece.prevLoc = piece.visualLoc;
    piece.body->getShapeAabb(piece.localPhysBBox[0], piece.localPhysBBox[1]);
  }
  alivePieceCnt = pieces.size();

  rendData.reset(destructables::init_rend_data(physObj.get(), params.isDestroyedByExplosion));
}

bool DestructableObject::Piece::isInteractable() const { return body && body->getInteractionLayer() != 0 && body->isInWorld(); }

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

void DestructableObject::setTimeToFloat(float time)
{
  const bool wasFloatEnabled = isFloatEnabled();
  for (Piece &piece : pieces)
    piece.timeToFloat = time;
  floatEnabled = floatEnabled || time >= 0.f;
  G_ASSERT_RETURN(destructables::g_context, );
  if (isFloatEnabled() && !wasFloatEnabled)
    destructables::g_context->numFloatable++;
}

void DestructableObject::addImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit)
{
  exec_or_add_after_phys_action<DestructableObjectAddImpulse>(pw, *this, pos, impulse, speedLimit, omegaLimit);
}

void DestructableObject::Piece::addImpulse(const Point3 &pos, const Point3 &impulseDir, float impulseLen, float speedLimit,
  float omegaLimit)
{
  G_ASSERT_RETURN(isAlive(), );
  const float maxImpulseMul = 15; // we use mass for calculating impulse mul. Because big parts isn't react for min bullets
  const float maxDirCorrection = 0.8;
  const float dirCorrectionPerMeter = 0.3; // we change angle for a more beautiful scattering of fragments
  const float impulseMulPerMeter = 15;     // we use invarian for calculating momentum divergence (more inpulse for close parts to pos)
  const float maxRadius = 1.5;
  const float radius = cvt(impulseLen, 0., 300., 0., maxRadius); // For larger impulses, we change the position of applying the impulse
                                                                 // relative to body

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

void DestructableObject::doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit)
{
  float impulseLen = length(impulse);
  const Point3 impulseDir = impulse * safeinv(impulseLen);

  const float minImpulseLen = 15.;
  if (impulseLen > 0. && impulseLen < minImpulseLen)
    impulseLen = minImpulseLen;

  for (Piece &piece : pieces)
    if (piece.isAlive())
      piece.addImpulse(pos, impulseDir, impulseLen, speedLimit, omegaLimit);
}

void DestructableObject::setupInitialPhysState(const TMatrix &query_tm, const BBox3 &query_box, const Point3 &impact_pos,
  const Point3 &impact_impulse)
{
  TIME_PROFILE(destructable_init_phys)
  struct DestructablePhysSettings
  {
    const float directVelFalloff = 1.f;
    const float radialVelBaseRadius = 0.5f;
    const float pieceImpactSpeed = 5.f;  // m/s along the impact direction at the impact point
    const float pieceRadialSpeed = 5.f;  // m/s of crater burst away from the impact point
    const float piecePushoutSpeed = 2.f; // m/s along the pushout direction for pieces that had to be pushed
    const float pieceSpeedJitter = 0.4f; // random isotropic speed
    const float pieceTumbleFactor = 1.f; // fraction of the orbital omega implied by v around the impact point
    const float pieceRandomOmega = 3.f;  // rad/s of random tumble for a 1m piece, scaled by 1/size
    const float pieceMaxSpeed = 10.f;
    const float pieceMaxOmega = 10.f;

    const float riContactGatherMarginXZ = 2.5f;
    const float pieceContactGatherMargin = 0.1f; // lateral inflation of the swept bbox (never along the sweep axis)
    const float pieceMaxPushoutBboxPart = 0.6f;  // cap the applied move to this fraction of the bbox extent along the axis
    const float pieceMaxPushoutAbs = 1.f;        // absolute cap on the applied move, meters
    const float piecePushoutSearchDist = 2.5f;   // how far the sweep looks for a point-free spot when pricing directions
  } settings;

  int seed = grnd();
  PhysWorld *physWorld = dacoll::get_phys_world();

  Tab<gamephys::CollisionContactData> contacts;
  if (!query_box.isempty())
  {
    TIME_PROFILE(destructable_query_bbox)
    PhysBodyCreationData pbcd;
    pbcd.addToWorld = false;
    pbcd.group = destructables::get_config().defaultFGroup;
    pbcd.mask = destructables::get_config().defaultFMask;
    // translate so query box is centered at origin
    TMatrix queryCenteredTm = TMatrix::IDENT;
    queryCenteredTm.setcol(3, query_box.center());
    queryCenteredTm = query_tm * queryCenteredTm;
    BBox3 queryCenteredBox(-query_box.width() * 0.5f, query_box.width() * 0.5f);
    queryCenteredBox.inflateXZ(settings.riContactGatherMarginXZ);
    PhysBoxCollision queryColl(queryCenteredBox.width().x, queryCenteredBox.width().y, queryCenteredBox.width().z);
    PhysBody queryBody(physWorld, 1.f, &queryColl, queryCenteredTm, pbcd);
    dacoll::test_collision_ri(CollisionObject(&queryBody, nullptr), queryCenteredBox, contacts, true, -1.f, nullptr, PHYSMAT_DEFAULT,
      false);
  }

  Tab<Point3> localCloud(framemem_ptr());
  localCloud.reserve(contacts.size());
  for (auto &piece : pieces)
  {
    PhysBody *body = piece.body;
    TMatrix bodyTm, bodyModifiedTm;
    body->getTm(bodyTm);
    bodyModifiedTm = bodyTm;
    BBox3 pieceBbox = piece.localPhysBBox;

    const Point3 rImpact = bodyTm.getcol(3) - impact_pos;
    const float dist = length(rImpact);

    const TMatrix bodyItm = inverse(bodyTm);
    BBox3 gateBbox = pieceBbox;
    gateBbox.inflate(settings.pieceContactGatherMargin);

    // piece-local point cloud (transformed once, shared by all candidate directions)
    localCloud.clear();
    for (const auto &contact : contacts)
      localCloud.push_back(bodyItm * contact.wposB);

    bool anyPointInside = false;
    for (const Point3 &pl : localCloud)
      if (gateBbox & pl)
      {
        anyPointInside = true;
        break;
      }

    Point3 piecePushout(0.f, 0.f, 0.f);
    if (anyPointInside)
    {
      float bestDist = FLT_MAX;
      Point3 bestDirW(0.f, 0.f, 0.f);
      for (int axis = 0; axis < 3; axis++)
        for (int sign = -1; sign <= 1; sign += 2)
        {
          const Point3 dirW = normalize(query_tm.getcol(axis)) * float(sign);
          if (fabsf(dirW.y) > 0.7f)
            continue;
          const Point3 v = -(bodyItm % dirW);
          BBox3 sweepBbox = pieceBbox;
          for (int c = 0; c < 3; c++)
          {
            const float infl = settings.pieceContactGatherMargin * (1.f - fabsf(v[c]));
            sweepBbox.lim[0][c] -= infl;
            sweepBbox.lim[1][c] += infl;
          }
          float t = 0.f;
          for (int pass = 0; pass < 16 && t < bestDist && t <= settings.piecePushoutSearchDist; pass++)
          {
            bool changed = false;
            for (const Point3 &pl : localCloud)
            {
              float tEnter = -FLT_MAX, tExit = FLT_MAX;
              bool empty = false;
              for (int c = 0; c < 3 && !empty; c++)
              {
                if (fabsf(v[c]) > 1e-6f)
                {
                  float t0 = (sweepBbox.lim[0][c] - pl[c]) / v[c];
                  float t1 = (sweepBbox.lim[1][c] - pl[c]) / v[c];
                  if (t0 > t1)
                    eastl::swap(t0, t1);
                  tEnter = max(tEnter, t0);
                  tExit = min(tExit, t1);
                  empty = tEnter > tExit;
                }
                else
                  empty = pl[c] < sweepBbox.lim[0][c] || pl[c] > sweepBbox.lim[1][c];
              }
              if (empty)
                continue;
              if (t >= tEnter - 1e-4f && t < tExit)
              {
                t = tExit + 1e-4f;
                changed = true;
              }
            }
            if (!changed)
              break;
          }
          if (t < bestDist)
          {
            bestDist = t;
            bestDirW = dirW;
          }
        }
      const Point3 bestV = bodyItm % bestDirW;
      const Point3 bboxWidth = pieceBbox.width();
      const float extentAlong = fabsf(bestV.x) * bboxWidth.x + fabsf(bestV.y) * bboxWidth.y + fabsf(bestV.z) * bboxWidth.z;
      piecePushout = bestDirW * min(bestDist, min(settings.pieceMaxPushoutBboxPart * extentAlong, settings.pieceMaxPushoutAbs));
      bodyModifiedTm.setcol(3, bodyModifiedTm.getcol(3) + piecePushout);
    }

    const Point3 impulseNorm = impact_impulse / max(0.05f, length(impact_impulse));

    // velocity = [impact direction push + radial burst] + pushout speed + jitter, first two terms are clamped against pushout dir
    const Point3 radialDir = rImpact * safeinv(dist);
    const float impactFalloff = expf(-dist * settings.directVelFalloff);
    const float radialFalloff = sqrtf(settings.radialVelBaseRadius / (dist + settings.radialVelBaseRadius));
    const Point3 pushoutDir = piecePushout * safeinv(length(piecePushout));
    Point3 vel = impulseNorm * (settings.pieceImpactSpeed * impactFalloff) + radialDir * (settings.pieceRadialSpeed * radialFalloff);
    const float velAgainstPushout = dot(vel, pushoutDir);
    if (velAgainstPushout < 0.f)
      vel -= pushoutDir * velAgainstPushout;
    vel += pushoutDir * settings.piecePushoutSpeed + Point3(_srnd(seed), _srnd(seed), _srnd(seed)) * settings.pieceSpeedJitter;
    const float velLen = length(vel);
    if (velLen > settings.pieceMaxSpeed)
      vel *= settings.pieceMaxSpeed / velLen;

    // omega: the rotation the velocity implies around the impact point (as if hinged there), so nearby
    // pieces tumble consistently with the burst, plus size-scaled random tumble
    Point3 omega = (rImpact % vel) * (settings.pieceTumbleFactor / max(sqr(0.2f), lengthSq(rImpact)));
    const float pieceSize = max(float(pieceBbox.width().length()), 0.2f);
    omega += Point3(_srnd(seed), _srnd(seed), _srnd(seed)) * (settings.pieceRandomOmega / pieceSize);
    const float omegaLen = length(omega);
    if (omegaLen > settings.pieceMaxOmega)
      omega *= settings.pieceMaxOmega / omegaLen;

    // changing TM here will count towards initial jump error and smoothed
    body->setTm(bodyModifiedTm);
    body->setVelocity(vel);
    body->setAngularVelocity(omega);
    body->activateBody(true);
  }
}


void DestructableObject::Piece::updateFloatable(float dt, float at_time)
{
  G_ASSERT_RETURN(isAlive(), );
  if (timeToFloat < 0.f)
    return;

  float mass = body->getMass();
  TMatrix bodyTm;
  body->getTm(bodyTm);
  Point3 pos = bodyTm.getcol(3);

  bool underwater = false;
  float waterHt = dacoll::traceht_water_at_time(pos, 0.5f, at_time, underwater);
  if (!dacoll::is_valid_water_height(waterHt))
    return;

  float waterDist = pos.y - waterHt;
  if (waterDist < 0.0f && timeToFloat > 2.0f)
  {
    const float defDensityRatio = 1.2f;
    float fa = min(sqr(waterDist), sqr(-1.0f)) * defDensityRatio;
    Point3 impulse = Point3(0.f, fa * mass * gamephys::atmosphere::g() * dt, 0.f);
    body->addImpulse(pos, impulse);
  }
}

static Point3 quat_to_scaled_angle_axis(Quat q, float eps = 1e-8f)
{
  const Point3 v(q.x, q.y, q.z);
  const float length = v.length();
  if (length < eps)
    return v * 2.f;
  return v * (2.f * acosf(min(1.f, fabs(q.w))) / length);
}

static Quat quat_from_scaled_angle_axis(const Point3 &v, float eps = 1e-8f)
{
  const Point3 halfv = v * 0.5f;
  const float halfangle = halfv.length();
  if (halfangle < eps)
    return normalize(Quat(halfv.x, halfv.y, halfv.z, 1.f));
  const Point3 xyz = halfv * (sinf(halfangle) / halfangle);
  return Quat(xyz.x, xyz.y, xyz.z, cosf(halfangle));
}

static void spring_cd(Point3 &val, Point3 &vel, const Point3 &target, float omega, float dt)
{
  const float d = expf(-omega * dt);
  const Point3 err = val - target;
  const Point3 b = vel + err * omega;
  val = target + (err + b * dt) * d;
  vel = (vel - b * (omega * dt)) * d;
}

static void spring_cd_quat(Quat &q, Point3 &vel, float omega, float dt)
{
  Point3 r = quat_to_scaled_angle_axis(q);
  spring_cd(r, vel, Point3(0.f, 0.f, 0.f), omega, dt);
  q = quat_from_scaled_angle_axis(r);
}


void DestructableObject::Piece::update(DestructableObject &parent, float dt, float scaled_dt, bool force_inactive_timer)
{
  G_ASSERT_RETURN(isAlive(), );
  const bool isBodyInWorld = body->isInWorld();
  const bool isPhysActive = isBodyInWorld && body->isActive() && body->getInteractionLayer() != 0;

  if (isBodyInWorld)
  {
    TMatrix wtm;
    body->getTm(wtm);
    gamephys::Loc curLoc;
    curLoc.fromTM(wtm);

    const destructables::DestructablesConfig &cfg = destructables::get_config();

    // accumulate visual error
    if (lifeTime < cfg.visualErrorAccumulateTime)
    {
      const Point3 vel = body->getVelocity();
      const Point3 omega = body->getAngularVelocity();
      gamephys::Loc predictedLoc;
      constexpr float PHYS_DT = 1.f / 60.f;
      // round up to predicted amount of phys world substeps to avoid accumulating
      const float predictDt = ceilf(dt / PHYS_DT) * PHYS_DT;
      predictedLoc.P = prevLoc.P + DPoint3::xyz(vel) * predictDt;
      predictedLoc.O.setQuat(quat_from_scaled_angle_axis(omega * predictDt) * prevLoc.O.getQuat());
      gamephys::Loc deviationLoc;
      deviationLoc.substract(predictedLoc, curLoc);
      gamephys::Loc addErr;
      addErr.substract(prevLoc, curLoc);

      // only accumulate error, if deviation from prediction passes threshold (which is ~1 tick worth of current velocity/omega)
      const float thresholdTolerance = 1.05f;
      const float minPosThreshold = 0.05f;
      const float minRotThreshold = 0.5f * DEG_TO_RAD;
      const float posDeviationLen = length(Point3::xyz(deviationLoc.P));
      const float posDeviationThreshold = length(vel) * predictDt;
      if (posDeviationLen < thresholdTolerance * max(posDeviationThreshold, minPosThreshold))
        addErr.P = ZERO<DPoint3>();
      const Point3 rotDeviationR = quat_to_scaled_angle_axis(deviationLoc.O.getQuat());
      const float rotDeviationThreshold = length(omega) * predictDt;
      if (length(rotDeviationR) < thresholdTolerance * max(rotDeviationThreshold, minRotThreshold))
        addErr.O.setQuatToIdentity();
      visualLocErr.add(addErr, visualLocErr);
    }

    prevLoc = visualLoc = curLoc;

    // apply and decay visual error
    if (visualLocErr.P != ZERO<DPoint3>() || Point3::xyz(visualLocErr.O.getQuat()) != Point3::ZERO)
    {
      const float omegaS = 2.f / max(cfg.visualErrorSmoothTime, VERY_SMALL_NUMBER);
      Point3 errP = visualLocErr.P;
      spring_cd(errP, posErrVel, Point3(0.f, 0.f, 0.f), omegaS, dt);
      visualLocErr.P = errP;
      Quat errO = visualLocErr.O.getQuat();
      spring_cd_quat(errO, rotErrVel, omegaS, dt);
      visualLocErr.O.setQuat(errO);
      visualLoc.add(curLoc, visualLocErr);
      // snap to zero
      if (visualLocErr.P.lengthSq() < sqr(1e-4))
        visualLocErr.P = ZERO<DPoint3>(), posErrVel = Point3::ZERO;
      if (Point3::xyz(visualLocErr.O.getQuat()).lengthSq() < sqr(1e-3f))
        visualLocErr.O.setQuatToIdentity(), rotErrVel = Point3::ZERO;
    }
  }

  outOfViewTime = visibleFrames < VISIBLE_FRAMES_THRESHOLD ? outOfViewTime + dt : 0.f;
  inactiveTime = isPhysActive && !force_inactive_timer ? 0.f : inactiveTime + scaled_dt;

  const float keepAliveMaxHeightAboveGround = destructables::get_config().keepAliveMaxHeightAboveGround;
  if (const float checkKeepAliveAtTime = parent.inactiveTimeBeforeSink / 2.f;
      keepAliveMaxHeightAboveGround > 0.f && inactiveTime >= checkKeepAliveAtTime && checkKeepAliveAtTime > inactiveTime - scaled_dt)
  {
    TMatrix wtm;
    body->getTm(wtm);
    BBox3 bbox = wtm * localPhysBBox;
    float height = 10.f;
    keepAlive = dacoll::tracedown_normalized(Point3::xVz(wtm.getcol(3), bbox.lim[1].y), height, nullptr, nullptr) &&
                height < keepAliveMaxHeightAboveGround;
  }

  if (!keepAlive && (inactiveTime > parent.inactiveTimeBeforeSink || timeToLive <= parent.timeToSinkUnderground))
  {
    if (body->getInteractionLayer() != 0)
    {
      TMatrix wtm;
      body->getTm(wtm);
      BBox3 bbox = wtm * localPhysBBox;
      float sinkDist = 10.f;
      dacoll::tracedown_normalized(Point3::xVz(wtm.getcol(3), bbox.lim[1].y), sinkDist, nullptr, nullptr);
      sinkDist = max(sinkDist, 0.25f);
      const float sinkUndergroundGravity =
        eastl::max(0.1f, sinkDist / sqr(max(parent.timeToSinkUnderground - 0.5f, 0.1f) / safediv(scaled_dt, dt)) * 2.0f /
                           /* adjust for damping */ 0.75f);
      // disable all collision and sink
      body->setGroupAndLayerMask(0, 0);
      body->setGravity(Point3(0.f, -sinkUndergroundGravity, 0.f));
      body->setVelocity(Point3::ZERO);
      body->setAngularVelocity(Point3::ZERO);
      // remove piece after sinking
      timeToLive = parent.timeToSinkUnderground;
    }
    body->activateBody(true);
    if (outOfViewTime > 0.5f) // sinking out of view - just destroy
    {
      destroy();
      return;
    }
  }

  if (timeToKinematic >= 0.f)
  {
    timeToKinematic -= scaled_dt;
    if (timeToKinematic < 0.f)
      makeKinematic();
  }

  if (timeToFloat >= 0.f)
    timeToFloat -= scaled_dt;
  lifeTime += scaled_dt;

  if (!keepAlive)
    timeToLive -= scaled_dt;
  if (timeToLive < 0)
    destroy();
}

void DestructableObject::Piece::makeKinematic()
{
  G_ASSERT_RETURN(isAlive(), );
  if (body->getInteractionLayer() && body->isInWorld())
    body->setGroupAndLayerMask(destructables::get_config().defaultFGroup,
      destructables::get_config().defaultFMask ^ dacoll::EPL_KINEMATIC);
}

void DestructableObject::Piece::destroy()
{
  G_ASSERT_RETURN(isAlive(), );
  if (body->isInWorld())
    body->getPhysWorld()->removeBody(body);
  body = nullptr;
}

bool DestructableObject::update(float dt, float cur_dt_scale, bool force_inactive_timer)
{
  if (alivePieceCnt < 0)
    return false;
  TIME_PROFILE(DestructableObject__update);

  const bool forceInactiveTimer = force_inactive_timer && scaleDt > 1.f;
  float scaledDt = cur_dt_scale * scaleDt * dt;
  alivePieceCnt = 0;
  for (Piece &piece : pieces)
  {
    if (piece.isAlive())
      piece.update(*this, dt, scaledDt, forceInactiveTimer);
    alivePieceCnt += piece.isAlive();
  }
  if (disintegrationTime < disintegrationParameters.duration)
    disintegrationTime += scaledDt;
  return isAlive();
}

Point4 DestructableObject::getDisintegrationParams() const
{
  return animchar_disintegration::get_additional_data(disintegrationTime, disintegrationParameters);
}

bool DestructableObject::hasDisintegrationAnimation() const { return disintegrationParameters.duration > 0; }


namespace destructables
{
const DestructablesConfig &get_config()
{
  G_ASSERT(g_context);
  return g_context->config;
}
DestructablesConfig &get_mutable_config()
{
  G_ASSERT(g_context);
  return g_context->config;
}

void DestructablesConfig::loadFromBlk(const DataBlock *destr_blk)
{
  maxNumberOfDestructableBodies = destr_blk->getInt("maxNumberOfDestructableBodies", maxNumberOfDestructableBodies);
  numOfDestrBodiesForScaleDt = destr_blk->getInt("numOfDestrBodiesForScaleDt", maxNumberOfDestructableBodies / 2);
  const float distForScaleDt = destr_blk->getReal("distForScaleDt", 100.f);
  distForScaleDtSq = distForScaleDt > 0.f ? sqr(distForScaleDt) : distForScaleDt;
  maxScaleDt = destr_blk->getReal("maxScaleDt", maxScaleDt);
  minDestrRadiusSq = sqr(destr_blk->getReal("minDestrRadius", 40.f));
  const DataBlock *outOfViewBlk = destr_blk->getBlockByNameEx("outOfViewDisappear");
  outOfViewDisappear.softHardBodyLimit = outOfViewBlk->getIPoint2("softHardBodyLimit", outOfViewDisappear.softHardBodyLimit);
  outOfViewDisappear.minSizeTime = outOfViewBlk->getPoint2("minSizeTime", outOfViewDisappear.minSizeTime);
  outOfViewDisappear.maxSizeTime = outOfViewBlk->getPoint2("maxSizeTime", outOfViewDisappear.maxSizeTime);
  outOfViewDisappear.maxRelSize = outOfViewBlk->getPoint2("maxRelSize", outOfViewDisappear.maxRelSize);
  keepAliveMaxHeightAboveGround = destr_blk->getReal("keepAliveMaxHeightAboveGround", keepAliveMaxHeightAboveGround);
  visualErrorSmoothTime = destr_blk->getReal("visualErrorSmoothTime", visualErrorSmoothTime);
  visualErrorAccumulateTime = destr_blk->getReal("visualErrorAccumulateTime", visualErrorAccumulateTime);
  warnOnBodiesOverflow = destr_blk->getBool("warnOnBodiesOverflow", warnOnBodiesOverflow);
#if DAGOR_DBGLEVEL > 0
  errorOnBodiesOverflow = destr_blk->getBool("errorOnBodiesOverflow", errorOnBodiesOverflow);
  minBodyCountForOverflowError = destr_blk->getInt("minBodyCountForOverflowError", minBodyCountForOverflowError);
#endif
}


void init(const DataBlock *blk, int fgroup)
{
  g_context.demandInit();
  DestructablesConfig &config = g_context->config;

  const DataBlock *destrBlk = blk->getBlockByNameEx("destructables");
  config.loadFromBlk(destrBlk);

#if _TARGET_PC || _TARGET_APPLE
  // legacy override by graphics preset
  if (const DataBlock *destrBlkPresets = destrBlk->getBlockByNameEx("maxNumberOfDestructableBodiesByPresets"))
  {
    config.maxNumberOfDestructableBodies = destrBlkPresets->getInt(
      dgs_get_settings()->getBlockByNameEx("graphics")->getStr("preset", "medium"), config.maxNumberOfDestructableBodies);
    if (destrBlk->findParam("numOfDestrBodiesForScaleDt") < 0)
      config.numOfDestrBodiesForScaleDt = config.maxNumberOfDestructableBodies / 2;
  }
#endif

  config.defaultFGroup = fgroup;
}

void close() { g_context.demandDestroy(); }

destructables::id_t addDestructable(gamephys::DestructableObject **out_destr, DestructableCreationParams &&params,
  PhysWorld *phys_world)
{
  G_ASSERT_RETURN(g_context, INVALID_ID);
  G_ASSERT_RETURN(params.physObjData || !params.fracturePhysObjects.empty(), nullptr);
  Context &ctx = *g_context;

  float scaleDt = 1.f;
  if (ctx.config.distForScaleDtSq > 0.f)
  {
    float distSq = (params.camPos - params.tm.getcol(3)).lengthSq();
    if (distSq > ctx.config.distForScaleDtSq)
      scaleDt = clamp(sqrtf(distSq / ctx.config.distForScaleDtSq), 1.f, ctx.config.maxScaleDt);
  }

  void *mem = ctx.destructablesListAllocator.allocateOneBlock();
  DestructableObject *obj = nullptr;
  {
    TIME_PROFILE(addDestructable__DestructableObject_ctor);
    obj = new (mem, _NEW_INPLACE) DestructableObject(eastl::move(params), phys_world, scaleDt);
  }
  ctx.destructablesList.emplace_back(obj);
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
    params.inactiveTimeBeforeSink = blk->getReal("timeForBodies", -1.0f);
    params.timeToKinematic = blk->getReal("timeToKinematic", -1.0f);
    params.disintegrationDuration = blk->getReal("disintegrationDuration", -1.0f);
    params.timeToStartDisintegration = blk->getReal("timeToDisintegration", -1.0f);
    params.disintegrationScale = blk->getReal("disintegrationScale", -1.0f);
  }
  return addDestructable(out_destr, eastl::move(params), phys_world);
}


void clear()
{
  G_ASSERT_RETURN(g_context, );
  clear_and_shrink(g_context->destructablesList);
  g_context->destructablesListAllocator.clear();
}

void removeDestructableById(id_t id)
{
  G_ASSERT_RETURN(g_context, );
  auto &destructablesList = g_context->destructablesList;
  if (id != INVALID_ID && eastl::find(destructablesList.begin(), destructablesList.end(), id,
                            [](auto &rec, id_t id) { return rec.get() == id; }) != destructablesList.end())
    static_cast<DestructableObject *>(id)->markForDelete();
}

static void overflow_handler()
{
  Context &ctx = *g_context;
  const char overflow_msg[] = "destructables::update: too many destructable bodies %d, max - %d";
  if (!ctx.config.errorOnBodiesOverflow || ctx.destructablesList.size() > ctx.config.minBodyCountForOverflowError) //-V560
  {
    if (ctx.config.warnOnBodiesOverflow)
      logwarn(overflow_msg, ctx.numAliveBodies, ctx.config.maxNumberOfDestructableBodies);
    else
      debug(overflow_msg, ctx.numAliveBodies, ctx.config.maxNumberOfDestructableBodies);
    return;
  }
#if DAGOR_DBGLEVEL > 0
  String msg(framemem_ptr());
  msg.printf(0, overflow_msg, ctx.numAliveBodies, ctx.config.maxNumberOfDestructableBodies);
  for (const auto &i : ctx.destructablesList)
  {
    const auto *physObj = i->getPhysObj();
    if (!physObj)
      continue;

    for (int n = 0; n < physObj->getModelCount(); ++n)
    {
      DynamicRenderableSceneInstance *inst = physObj->getModel(n);
      DynamicRenderableSceneLodsResource *lods = inst->getLodsResource();

      String name;
      String resMsg(framemem_ptr());
      resolve_game_resource_name(name, lods);
      resMsg.printf(0, "\n  res: %s, active bodies: %d", name.c_str(), i->alivePieceCnt);
      msg.append(resMsg);
    }
  }
  logerr("%s", msg.c_str());
#endif
}

void update(float dt, const Point3 &view_pos)
{
  G_ASSERT_RETURN(g_context, );
  TIME_PROFILE(destructables_update);
  Context &ctx = *g_context;
  const DestructablesConfig &config = ctx.config;

  const float dtUpdateScale = config.numOfDestrBodiesForScaleDt > 0 ? cvt(ctx.numAliveBodies, config.numOfDestrBodiesForScaleDt,
                                                                        config.maxNumberOfDestructableBodies, 1.f, config.maxScaleDt)
                                                                    : 1.f;
  const bool forceInactiveTimer = config.numOfDestrBodiesForScaleDt > 0 && ctx.numAliveBodies > config.numOfDestrBodiesForScaleDt;
  ctx.numAliveBodies = 0;
  int deleteLimit = DESTRUCTABLES_DELETE_MAX_PER_FRAME;
  ctx.destructablesList.erase(eastl::remove_if(ctx.destructablesList.begin(), ctx.destructablesList.end(),
                                [&](auto &object) {
                                  if (!object->isAlive())
                                  {
                                    // delete objects marked at previous frames
                                    if (deleteLimit-- > 0)
                                      return true; // ~18 deletions at 1 msec on AMD 5800H. Move to thread?
                                  }
                                  else if (object->update(dt, dtUpdateScale, forceInactiveTimer))
                                    ctx.numAliveBodies += object->alivePieceCnt;
                                  // else it already considered marked for deletion
                                  return false;
                                }),
    ctx.destructablesList.end());
  DA_PROFILE_TAG(destructables_update, "pieces:%d", ctx.numAliveBodies);

  // gradual out of view disappear
  {
    const DestructablesConfig::OutOfViewDisappear &oovCfg = config.outOfViewDisappear;
    const float scale = cvt(ctx.numAliveBodies, oovCfg.softHardBodyLimit.x, oovCfg.softHardBodyLimit.y, 0.f, 1.f);
    const float minSizeTime = lerp(oovCfg.minSizeTime.x, oovCfg.minSizeTime.y, scale);
    const float maxSizeTime = lerp(oovCfg.maxSizeTime.x, oovCfg.maxSizeTime.y, scale);
    const float relSizeCap = lerp(oovCfg.maxRelSize.x, oovCfg.maxRelSize.y, scale);
    const bool isOverflow = ctx.numAliveBodies > config.maxNumberOfDestructableBodies;
    const int maxDeletedPieces = isOverflow ? INT_MAX : 1, maxCheckedObjects = isOverflow ? ctx.destructablesList.size() : 2;
    const float minInactiveTime = scale > 0.5f ? 0.f : 2.5f; // if not urgent, give bodies some time to settle
    for (int steps = 0, deletedPieces = 0;
         steps < maxCheckedObjects && deletedPieces < maxDeletedPieces && !ctx.destructablesList.empty(); steps++)
    {
      if (ctx.removeChecksNextIdx >= ctx.destructablesList.size())
        ctx.removeChecksNextIdx = 0;
      DestructableObject &destr = *ctx.destructablesList[ctx.removeChecksNextIdx++];
      if (!destr.isAlive())
        continue;
      for (DestructableObject::Piece &piece : destr.pieces)
      {
        if (!piece.isAlive() || piece.visibleFrames != 0 || piece.inactiveTime < minInactiveTime)
          continue;
        const float maxRelSize = cvt(piece.outOfViewTime, minSizeTime, maxSizeTime, 0.f, relSizeCap);
        if (piece.localPhysBBox.width().lengthSq() < sqr(maxRelSize) * lengthSq(Point3(piece.visualLoc.P) - view_pos))
        {
          piece.destroy();
          ctx.numAliveBodies--;
          if (++deletedPieces >= maxDeletedPieces)
            break;
        }
      }
    }
  }

  // overflow handle
  ctx.overflowReportTimeout -= dt;
  if (ctx.numAliveBodies > config.maxNumberOfDestructableBodies)
  {
    TIME_PROFILE(destructables_overflow)
    if (ctx.overflowReportTimeout <= 0.0f)
    {
      ctx.overflowReportTimeout = 1.0f;
      overflow_handler();
    }
    for (int pass = 0; pass < 2; pass++)
    {
      const bool checkOutOfView = pass == 0;
      for (const auto &destrPtr : ctx.destructablesList)
      {
        if (ctx.numAliveBodies <= config.maxNumberOfDestructableBodies)
          break;
        DestructableObject &destr = *destrPtr;
        if (!destr.isAlive())
          continue;
        for (DestructableObject::Piece &piece : destr.pieces)
          if (piece.isAlive() && (piece.visibleFrames == 0 || !checkOutOfView))
          {
            piece.destroy();
            ctx.numAliveBodies--;
          }
      }
    }
  }
}

dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects()
{
  G_ASSERT_RETURN(g_context, {});
  auto &destructablesList = g_context->destructablesList;
  G_STATIC_ASSERT(sizeof(decltype(Context::destructablesList)::value_type) == sizeof(gamephys::DestructableObject *));
  return dag::ConstSpan<gamephys::DestructableObject *>((gamephys::DestructableObject **)destructablesList.data(), //-V1032
    destructablesList.size());
}

void update_floatable(float dt, float cur_time)
{
  G_ASSERT_RETURN(g_context, );
  Context &ctx = *g_context;
  if (!ctx.numFloatable)
    return;
  int numFloatable = 0;
  for (auto &object : ctx.destructablesList)
  {
    if (object->isAlive() && object->isFloatEnabled())
    {
      for (DestructableObject::Piece &piece : object->pieces)
        if (piece.isAlive())
          piece.updateFloatable(dt, cur_time);
      numFloatable++;
    }
  }
  ctx.numFloatable = numFloatable;
}

} // namespace destructables
