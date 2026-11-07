// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/destructableProps.h>
#include <gamePhys/phys/destructableRendObject.h>
#include <EASTL/algorithm.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <anim/dag_simpleNodeAnim.h>
#include <math/dag_geomTree.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <math/dag_noise.h>
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

#include "netDestrPhys.h"


using namespace gamephys;


static Point3 quat_to_scaled_angle_axis(Quat q, float eps = 1e-8f)
{
  if (q.w < 0.f)
    q = Quat(-q.x, -q.y, -q.z, -q.w);
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

  Point3 viewPos = Point3(1e6f, 1e6f, 1e6f);
  float curTime = 0.f;
  dag::Vector<vec4f> traceSpheres; // x,y,z,r2
  dag::Vector<PieceRef> tracePieces;

  Context() { destructablesListAllocator.init(sizeof(DestructableObject), (4096 * 2 - 16) / sizeof(DestructableObject)); }
};

static InitOnDemand<Context, false> g_context;

static void make_phys_group_and_mask(InteractionFlags flags, int &out_group, int &out_mask)
{
  out_group = flags & InteractionFlag::Debris ? dacoll::EPL_DEFAULT : flags & InteractionFlag::Static ? dacoll::EPL_DEBRIS : 0;
  out_mask = 0;
  if (flags & InteractionFlag::Static)
    out_mask |= dacoll::EPL_STATIC;
  if (flags & InteractionFlag::Debris)
    out_mask |= dacoll::EPL_DEFAULT;
  if (flags & InteractionFlag::Player)
    out_mask |= dacoll::EPL_KINEMATIC | dacoll::EPL_CHARACTER; // walkers & vehicles | ragdolls
}

void DestructableObjectDeleter::operator()(DestructableObject *object)
{
  interlocked_increment(*(volatile int *)&object->gen); // Cast to disable DSE
  object->~DestructableObject();
  g_context->destructablesListAllocator.freeOneBlock(object);
}
} // namespace destructables


namespace destructables
{

struct DestrAnimState
{
  const DestrAnimData *animData = nullptr;
  Ptr<PhysicsResource> physRes;
  struct PieceAnim
  {
    SimpleNodeAnim anim;
    int tMin = 0, tMax = 0;
  };
  TMatrix baseTm;
  dag::Vector<PieceAnim> pieces;

  DestrAnimState() = default;
  ~DestrAnimState()
  {
    if (animData)
      animData->delRef();
  }

  void init(const DestrAnimData *anim_data, const TMatrix &base_tm, PhysicsResource *phys_res)
  {
    G_ASSERT_RETURN(anim_data, );
    G_ASSERT_RETURN(phys_res, );
    physRes = phys_res;
    animData = anim_data;
    animData->addRef();
    baseTm = base_tm;
    const auto &bodies = phys_res->getBodies();
    pieces.resize(bodies.size());
    if (!animData->anim)
      return; // no anim data loaded, init empty
    bool hasAny = false;
    for (int i = 0; i < bodies.size(); i++)
    {
      auto &piece = pieces[i];
      if (!piece.anim.init(animData->anim, bodies[i].name))
        continue;
      hasAny = true;
    }
    if (!hasAny)
      logerr("destr anim <%s> has no matching bodies in phys obj", animData->animRes.c_str());
  }

  void initPiece(int piece_id, PhysBody *body)
  {
    G_ASSERT_RETURN(body, );
    if (!pieces[piece_id].anim.isValid())
      return;
    auto &piece = pieces[piece_id];
    piece.anim.calcTimeMinMax(piece.tMin, piece.tMax);
    if (piece.tMax == 0)
      return;
    body->setLockedStatic(true, 0, Point3::ZERO);
  }

  bool updateAndMovePiece(int piece_id, PhysBody *body, float time, float /*dt*/)
  {
    auto &piece = pieces[piece_id];
    if (piece.tMax == 0)
      return false;
    const auto calcTm = [&](int t) {
      TMatrix animTm;
      piece.anim.calcAnimTm(animTm, t);
      return baseTm * animTm;
    };
    const int curT = clamp(piece.tMin + int(time * float(AnimV20::TIME_TicksPerSec)), piece.tMin, piece.tMax);
    TMatrix curTm = calcTm(curT);
    body->setTm(curTm);
    if (curT == piece.tMax) // ended
    {
      constexpr float sampleDt = 0.1f;
      const TMatrix prevTm = calcTm(piece.tMax - int(sampleDt * float(AnimV20::TIME_TicksPerSec)));
      body->setVelocity((curTm.getcol(3) - prevTm.getcol(3)) / sampleDt);
      Quat deltaRot = Quat(curTm) * inverse(Quat(prevTm));
      body->setAngularVelocity(quat_to_scaled_angle_axis(deltaRot) / sampleDt);
      const auto &physResBody = physRes->getBodies()[piece_id];
      body->setLockedStatic(false, physResBody.mass, physResBody.momj);
      piece.tMax = 0;
    }
    return true;
  }
};

} // namespace destructables


FracturePhysObject::~FracturePhysObject() = default;


static const destructables::DestrObjProps default_destr_obj_props;

DestructableObject::DestructableObject(destructables::DestructableCreationParams &&params, PhysWorld *phys_world, //-V730
  float scale_dt) :
  scaleDt(scale_dt),
  physObj(params.physObjData ? DynamicPhysObject::create(params.physObjData, phys_world, params.tm, destructables::DEFAULT_PHYS_GROUP,
                                 destructables::DEFAULT_PHYS_MASK, params.addToWorld)
                             : nullptr),
  resIdx(params.resIdx)
{
  using namespace destructables;

  destrObjProps = params.destrObjProps ? params.destrObjProps : &default_destr_obj_props;
  destrObjProps->addRef();

  netSyncKey = params.netSyncKey;
  if (netSyncKey != INVALID_NET_SYNC_KEY)
    netPhys = eastl::make_unique<destructables::DestrNetPhys>();

  mat44f tm44;
  v_mat44_make_from_43cu_unsafe(tm44, params.tm.array);
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm44);
  v_stu(&intialTmAndHash[0].x, m43.row0);
  v_stu(&intialTmAndHash[1].x, m43.row1);
  v_stu(&intialTmAndHash[2].x, m43.row2);
  memcpy(&intialTmAndHash[3].x, &params.hashVal, sizeof(params.hashVal));
  disintegrationTime = -destrObjProps->timeToStartDisintegration;

  fracturePhysObjects = eastl::move(params.fracturePhysObjects);

  // for physobj assign rendinst material to bodies, that missing material in the resource
  if (params.physObjData && physObj && params.riPhysMatId >= 0)
  {
    const int physBodyMatId = PhysMat::getPhysBodyMaterial(params.riPhysMatId);
    const dag::ConstSpan<PhysicsResource::Body> bodyDecls = params.physObjData->physRes->getBodies();
    for (int i = 0; i < bodyDecls.size(); i++)
    {
      const auto &decl = bodyDecls[i];
      if (!decl.materialName.empty())
        continue;
      PhysBody *body = physObj->getPhysSys()->getBody(i);
      G_ASSERT_CONTINUE(body);
      body->setMaterialId(physBodyMatId);
    }
  }

  clear_and_resize(pieces, physObj ? physObj->getPhysSys()->getBodyCount() : fracturePhysObjects.size());
  if (netPhys)
    netPhys->pieces.resize(pieces.size());
  if (params.addToWorld)
    for (int i = 0; i < pieces.size(); i++)
      initPiece(i);

  rendData.reset(init_rend_data(physObj.get(), params.isDestroyedByExplosion));

  if (params.destrAnim)
    startDestrAnim(params.destrAnim, params.tm);
}

DestructableObject::~DestructableObject() { destrObjProps->delRef(); }

void DestructableObject::initPiece(int piece_id)
{
  using namespace destructables;
  const DestructablesConfig &cfg = get_config();

  Piece &piece = pieces[piece_id];
  G_ASSERT_RETURN(!piece.initialized, );
  PhysBody *body = physObj ? physObj->getPhysSys()->getBody(piece_id) : fracturePhysObjects[piece_id].body.get();
  G_ASSERT_RETURN(body, );
  piece.body = body;
  piece.timeToLive = destrObjProps->timeToLive;
  piece.initialized = true;

  TMatrix bodyTm;
  piece.body->getTm(bodyTm);
  piece.visualLoc.fromTM(bodyTm);
  piece.prevLoc = piece.visualLoc;
  piece.body->getShapeAabb(piece.localPhysBBox[0], piece.localPhysBBox[1]);

  const float pieceSize = piece.localPhysBBox.width().length();
  const Point3 bboxMaxAbs = max(abs(piece.localPhysBBox.lim[0]), abs(piece.localPhysBBox.lim[1]));
  piece.boundingRadSq = bboxMaxAbs.lengthSq();

  InteractionFlags flags = InteractionFlag::Static | InteractionFlag::Player;
  if (destrObjProps->collideWithDebris)
    flags |= InteractionFlag::Debris;
  if (pieceSize > cfg.projectileCollisionMinSize)
    flags |= InteractionFlag::Projectile;
  if (body->getInteractionLayer() == 0)
  {
    flags = InteractionFlag::None;
    // force lifetime, but give it some timeout to fall through under gravity
    piece.timeToLive = max(destrObjProps->timeToSinkUnderground, 5.f);
  }
  piece.interactionFlags = InteractionFlag::None;
  piece.setInteractionFlags(flags);

  alivePieceCnt++;
}

void DestructableObject::addPiecesToWorld(dag::ConstSpan<int> piece_ids, bool drop_joints_to_inactive)
{
  G_ASSERT_RETURN(physObj, );
  G_ASSERT_RETURN(!piece_ids.empty(), );
  physObj->getPhysSys()->addBodiesToWorld(piece_ids, drop_joints_to_inactive);
  for (int pieceId : piece_ids)
    initPiece(pieceId);
}

void DestructableObject::startDestrAnim(const destructables::DestrAnimData *anim_data, const TMatrix &base_tm,
  dag::ConstSpan<int> piece_ids)
{
  G_ASSERT_RETURN(anim_data, );
  G_ASSERT_RETURN(physObj, );
  G_ASSERT_RETURN(physObj->getPhysSys()->getPhysicsResource(), );
  destrAnimState.reset(new destructables::DestrAnimState);
  destrAnimState->init(anim_data, base_tm, physObj->getPhysSys()->getPhysicsResource());
  for (int pieceI = 0, pieceCnt = piece_ids.empty() ? pieces.size() : piece_ids.size(); pieceI < pieceCnt; pieceI++)
  {
    const int i = piece_ids.empty() ? pieceI : piece_ids[pieceI];
    destrAnimState->initPiece(i, pieces[i].body);
  }
}

bool DestructableObject::Piece::isInteractable() const { return body && body->getInteractionLayer() != 0 && body->isInWorld(); }

void DestructableObject::Piece::setInteractionFlags(destructables::InteractionFlags flags)
{
  G_ASSERT_RETURN(isAlive(), );
  if (interactionFlags == flags)
    return;
  interactionFlags = flags;
  int group = 0, mask = 0;
  destructables::make_phys_group_and_mask(flags, group, mask);
  if (int(body->getGroupMask()) != group || int(body->getInteractionLayer()) != mask)
    body->setGroupAndLayerMask(group, mask);
}

void DestructableObject::Piece::changeInteractionFlags(destructables::InteractionFlags flags, bool enable)
{
  if (interactionFlags)
    setInteractionFlags(enable ? interactionFlags | flags : interactionFlags & ~flags);
}

struct gamephys::DestructableObjectAddImpulse final : public AfterPhysUpdateAction
{
  // Note: it's okay to use direct ref since it would be pointing to pool's memory (`destructablesListAllocator`)
  DestructableObject &dobj;
  int gen;
  TMatrix queryTm;
  BBox3 queryBox;
  Point3 pos, impulse;
  bool simpleImpulse;
  float speedLimit, omegaLimit;
  dag::Vector<int> pieceIds;

  DestructableObjectAddImpulse(DestructableObject &dobj_, const TMatrix &query_tm, const BBox3 &query_box, const Point3 &p,
    const Point3 &i, bool simple_impulse, float speed_limit, float omega_limit, dag::ConstSpan<int> piece_ids = {}) :
    dobj(dobj_),
    gen(dobj_.gen),
    queryTm(query_tm),
    queryBox(query_box),
    pos(p),
    impulse(i),
    simpleImpulse(simple_impulse),
    speedLimit(speed_limit),
    omegaLimit(omega_limit),
    pieceIds(piece_ids.begin(), piece_ids.end())
  {}

  void doAction(PhysWorld &, bool) override
  {
    if (gen != dobj.gen) // Was destroyed?
      return;
    if (simpleImpulse)
      dobj.doAddImpulse(pos, impulse, speedLimit, omegaLimit, pieceIds);
    else
      dobj.setupInitialPhysState(queryTm, queryBox, pos, impulse, pieceIds);
  }
};

void DestructableObject::addImpulseSimple(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speed_limit,
  float omega_limit)
{
  exec_or_add_after_phys_action<DestructableObjectAddImpulse>(pw, *this, TMatrix::IDENT, BBox3(), pos, impulse, true, speed_limit,
    omega_limit);
}

void DestructableObject::applyInitialImpulse(PhysWorld &pw, const TMatrix &query_tm, const BBox3 &query_box, const Point3 &pos,
  const Point3 &impulse, dag::ConstSpan<int> piece_ids)
{
  exec_or_add_after_phys_action<DestructableObjectAddImpulse>(pw, *this, query_tm, query_box, pos, impulse,
    destructables::get_config().simpleInitialImpulse, 7.f, 5.f, piece_ids);
}

void DestructableObject::resetVisualLocToBodies()
{
  for (Piece &piece : pieces)
  {
    if (!piece.isAlive())
      continue;
    TMatrix bodyTm;
    piece.body->getTm(bodyTm);
    piece.visualLoc.fromTM(bodyTm);
    piece.prevLoc = piece.visualLoc;
    piece.visualLocErr.P = ZERO<DPoint3>();
    piece.visualLocErr.O.setQuatToIdentity();
    piece.posErrVel = piece.rotErrVel = Point3::ZERO;
  }
}

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

void DestructableObject::doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit,
  dag::ConstSpan<int> piece_ids)
{
  float impulseLen = length(impulse);
  const Point3 impulseDir = impulse * safeinv(impulseLen);

  const float minImpulseLen = 15.;
  if (impulseLen > 0. && impulseLen < minImpulseLen)
    impulseLen = minImpulseLen;

  for (int pieceI = 0, pieceCnt = piece_ids.empty() ? pieces.size() : piece_ids.size(); pieceI < pieceCnt; pieceI++)
  {
    Piece &piece = pieces[piece_ids.empty() ? pieceI : piece_ids[pieceI]];
    if (piece.isAlive())
      piece.addImpulse(pos, impulseDir, impulseLen, speedLimit, omegaLimit);
  }
}


void DestructableObject::setupInitialPhysState(const TMatrix &, const BBox3 &, const Point3 &impact_pos, const Point3 &impact_impulse,
  dag::ConstSpan<int> piece_ids)
{
  static constexpr int MAX_BURST_SAMPLES_PER_AXIS = 8;

  TIME_PROFILE(destructable_init_phys)
  const destructables::DestructablesConfig::DestructionBurst &burst = destructables::get_config().destructionBurst;

  // setup burst parameters
  int seed = grnd();
  const float impulseLen = length(impact_impulse);
  const Point3 impulseNorm = impact_impulse / max(0.05f, impulseLen);
  const float intensity =
    min(1.f + burst.impulseIntensityGain * powf(impulseLen / max(burst.refImpulse, VERY_SMALL_NUMBER), burst.impulseIntensityExponent),
      burst.maxIntensity);
  const float directVelBias = burst.maxIntensity > 1.f ? cvt(intensity, 1.f, burst.maxIntensity, 1.f, burst.maxDirectVelBias) : 1.f;
  const float radialIntensity = sqrtf(intensity);
  const float penDepth = burst.maxIntensity > 1.f
                           ? cvt(intensity, 1.f, burst.maxIntensity, burst.penetrationBase, burst.penetrationMax)
                           : burst.penetrationBase;
  const float maxSpeed = burst.hardMaxSpeed;

  const float cellSize = max(burst.samplingScale * safeinv(burst.directVelFalloff), 0.1f);
  const int maxSamplesPerAxis = clamp(burst.maxSamplesPerAxis, 2, MAX_BURST_SAMPLES_PER_AXIS);
  const float linVelSampleWeightExp = burst.linVelSampleWeightExp;

  // setup velocity field
  struct BurstVelocityField
  {
    Point3 impactPos = Point3::ZERO;
    Point3 impulseNorm = Point3::ZERO;
    float impactSpeed = 0.f;
    float lateralFalloff = 0.f;
    float invPenDepth = 0.f;
    float radialSpeed = 0.f;
    float radialBaseRadius = 0.f;
    Point3 noiseOffset = Point3::ZERO;
    float noiseAmplitude = 0.f;
    float invNoiseScale = 0.f;
    float noiseLateralFrac = 0.f;

    static __forceinline Point3 sampleNoise(const Point3 &p)
    {
      return Point3(perlin_noise::noise3(p), perlin_noise::noise3(Point3(p.y + 19.7f, p.z + 33.4f, p.x + 47.2f)),
               perlin_noise::noise3(Point3(p.z + 71.3f, p.x + 3.9f, p.y + 15.6f))) *
             2.f;
    }

    Point3 sample(const Point3 &pos) const
    {
      const Point3 rImpact = pos - impactPos;
      const float dist = length(rImpact);
      const float depth = dot(rImpact, impulseNorm);
      const float lateral = invPenDepth > 0.f ? length(rImpact - impulseNorm * depth) : dist;
      const float impactFalloff = expf(-lateral * lateralFalloff - max(depth, 0.f) * invPenDepth);
      const float radialFalloff = sqrtf(radialBaseRadius / (dist + radialBaseRadius));
      const Point3 vDet = impulseNorm * (impactSpeed * impactFalloff) + rImpact * (safeinv(dist) * radialSpeed * radialFalloff);
      const Point3 np = (pos + noiseOffset) * invNoiseScale;
      const Point3 noise = sampleNoise(np) + sampleNoise(np * 4.f) * 0.5f;
      const Point3 vDir = vDet * safeinv(length(vDet));
      const Point3 noisePar = vDir * dot(noise, vDir);
      return vDet + (noisePar + (noise - noisePar) * noiseLateralFrac) * noiseAmplitude;
    }
  };
  BurstVelocityField field;
  field.impactPos = impact_pos;
  field.impulseNorm = impulseNorm;
  field.impactSpeed = burst.impactSpeed * intensity * directVelBias;
  field.lateralFalloff = burst.directVelFalloff;
  field.invPenDepth = penDepth > 0.f ? 1.f / penDepth : 0.f;
  field.radialSpeed = burst.radialSpeed * radialIntensity;
  field.radialBaseRadius = burst.radialVelBaseRadius;
  field.noiseAmplitude = burst.noiseScale > 0.f ? burst.noiseAmplitude : 0.f;
  field.invNoiseScale = safeinv(burst.noiseScale);
  field.noiseLateralFrac = clamp(burst.noiseLateralFrac, 0.f, 1.f);
  field.noiseOffset = Point3(_srnd(seed), _srnd(seed), _srnd(seed)) * 100.f;

  for (int pieceI = 0, pieceCnt = piece_ids.empty() ? pieces.size() : piece_ids.size(); pieceI < pieceCnt; pieceI++)
  {
    Piece &piece = pieces[piece_ids.empty() ? pieceI : piece_ids[pieceI]];
    if (!piece.isAlive() || piece.body->getInteractionLayer() == 0)
      continue;
    PhysBody *body = piece.body;
    TMatrix bodyTm;
    body->getTm(bodyTm);
    BBox3 pieceBbox = piece.localPhysBBox;
    const TMatrix bodyItm = inverse(bodyTm);
    const Point3 pieceExtents = pieceBbox.width();
    const float pieceSize = max(float(pieceExtents.length()), 0.2f);

    // sample and fit velocity field
    float sampleOffsets[3][MAX_BURST_SAMPLES_PER_AXIS];
    int sampleCount[3];
    for (int axis = 0; axis < 3; axis++)
    {
      const float spread = max(pieceExtents[axis], burst.minSampleSpread);
      const int count = clamp(int(ceilf(spread / cellSize)), 2, maxSamplesPerAxis);
      sampleCount[axis] = count;
      for (int i = 0; i < count; i++)
        sampleOffsets[axis][i] = spread * ((float(i) + 0.5f) / float(count) - 0.5f);
    }
    const Point3 bboxCenter = pieceBbox.center();
    Point3 velSum = Point3::ZERO, torque = Point3::ZERO, inertiaDiag = Point3::ZERO;
    float weightSum = 0.f;
    for (int ix = 0; ix < sampleCount[0]; ix++)
      for (int iy = 0; iy < sampleCount[1]; iy++)
        for (int iz = 0; iz < sampleCount[2]; iz++)
        {
          const Point3 r(sampleOffsets[0][ix], sampleOffsets[1][iy], sampleOffsets[2][iz]);
          const Point3 v = bodyItm % field.sample(bodyTm * (bboxCenter + r));
          const float speed = length(v);
          const float weight = powf(speed, linVelSampleWeightExp);
          velSum += v * weight;
          weightSum += weight;
          torque += r % v;
          inertiaDiag += Point3(sqr(r.y) + sqr(r.z), sqr(r.z) + sqr(r.x), sqr(r.x) + sqr(r.y));
        }
    const Point3 velLocal = weightSum > VERY_SMALL_NUMBER ? velSum / weightSum : Point3::ZERO;
    Point3 omegaLocal(torque.x / max(inertiaDiag.x, VERY_SMALL_NUMBER), torque.y / max(inertiaDiag.y, VERY_SMALL_NUMBER),
      torque.z / max(inertiaDiag.z, VERY_SMALL_NUMBER));

    // apply max tip speed and small piece jitter
    Point3 halfDiagPerp;
    for (int axis = 0; axis < 3; axis++)
    {
      const float other0 = pieceExtents[(axis + 1) % 3], other1 = pieceExtents[(axis + 2) % 3];
      halfDiagPerp[axis] = max(0.5f * sqrtf(sqr(other0) + sqr(other1)), 0.05f);
    }
    const float fadeT = max(burst.smallOmegaSizeThreshold, VERY_SMALL_NUMBER);
    const float fadeU = clamp(pieceSize / fadeT - 0.5f, 0.f, 0.5f) * 2.f; // 0..1 over [T/2, T]
    const float residualOmega = burst.smallPieceRandomOmega * (1.f - fadeU * fadeU * (3.f - 2.f * fadeU));
    const Point3 randomTumble = Point3(_srnd(seed), _srnd(seed), _srnd(seed)) * residualOmega;
    for (int axis = 0; axis < 3; axis++)
    {
      omegaLocal[axis] = (omegaLocal[axis] * burst.tumbleFactor + randomTumble[axis]);
      float maxOmega = burst.hardMaxOmega;
      if (burst.maxTipSpeed > 0.f)
        maxOmega = min(maxOmega, burst.maxTipSpeed / halfDiagPerp[axis]);
      omegaLocal[axis] = clamp(omegaLocal[axis], -maxOmega, maxOmega);
    }

    // convert to world and apply to body
    Point3 vel = bodyTm % velLocal;
    Point3 omega = bodyTm % omegaLocal;
    // velLocal is at the bbox center, vel is at body center
    vel += omega % (bodyTm.getcol(3) - bodyTm * bboxCenter);
    const float velLen = length(vel);
    if (velLen > maxSpeed)
      vel *= maxSpeed / velLen;
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

void DestructableObject::Piece::update(DestructableObject &parent, float dt, float scaled_dt, bool force_inactive_timer)
{
  using namespace destructables;

  G_ASSERT_RETURN(isAlive(), );
  const bool isBodyInWorld = body->isInWorld();
  bool isPlayingAnim = false;

  if (isBodyInWorld)
  {
    if (DAGOR_UNLIKELY(parent.destrAnimState))
      isPlayingAnim = parent.destrAnimState->updateAndMovePiece(this - parent.pieces.data(), body, lifeTime, dt);

    TMatrix wtm;
    body->getTm(wtm);
    gamephys::Loc curLoc;
    curLoc.fromTM(wtm);

    const destructables::DestructablesConfig &cfg = destructables::get_config();

    // accumulate visual error
    if (lifeTime < cfg.visualErrorAccumulateTime && !isPlayingAnim)
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
      addErr.O.setQuat(prevLoc.O.getQuat() * inverse(curLoc.O.getQuat())); // world-frame delta

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
      visualLocErr.add(visualLocErr, addErr);
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
      visualLoc.add(visualLocErr, curLoc);
      // snap to zero
      if (visualLocErr.P.lengthSq() < sqr(1e-4))
        visualLocErr.P = ZERO<DPoint3>(), posErrVel = Point3::ZERO;
      if (Point3::xyz(visualLocErr.O.getQuat()).lengthSq() < sqr(1e-3f))
        visualLocErr.O.setQuatToIdentity(), rotErrVel = Point3::ZERO;
    }

    if (parent.netPhys)
    {
      DestrNetPhys::PhysBodyState state;
      state.loc = curLoc;
      state.vel = body->getVelocity();
      state.omega = body->getAngularVelocity();
      parent.netPhys->pieces[this - parent.pieces.data()].record(state, lifeTime);
    }
  }

  const bool isPhysActive = isBodyInWorld && body->isActive() && body->getInteractionLayer() != 0 || isPlayingAnim;

  outOfViewTime = visibleFrames < VISIBLE_FRAMES_THRESHOLD ? outOfViewTime + dt : 0.f;
  inactiveTime = isPhysActive && !force_inactive_timer ? 0.f : inactiveTime + scaled_dt;
  bool isSinking = interactionFlags == InteractionFlag::None;

  const DestructablesConfig &cfg = get_config();
  const DestrObjProps &props = *parent.destrObjProps;

  if (isPhysActive)
    elevationAboveGround = FLT_MAX; // body in move - keep not measured

  // update elevation
  if (!isSinking && (cfg.keepAliveMaxHeightAboveGround > 0.f))
  {
    if (const float groundCheckAtTime = props.inactiveTimeBeforeSink / 2.f;
        inactiveTime >= groundCheckAtTime && groundCheckAtTime > inactiveTime - scaled_dt)
    {
      TMatrix wtm;
      body->getTm(wtm);
      BBox3 bbox = wtm * localPhysBBox;
      float height = 10.f;
      const bool traced = dacoll::tracedown_normalized(Point3::xVz(wtm.getcol(3), bbox.lim[1].y), height, nullptr, nullptr);
      elevationAboveGround = traced ? height : FLT_MAX;
    }
  }

  // update interaction flags
  if (!isSinking)
  {
    const bool playerCollision = props.playerCollisionTime > lifeTime || props.playerCollisionTime < 0.f;
    changeInteractionFlags(InteractionFlag::Player, playerCollision);
  }

  // start sinking
  const bool canSink = props.timeToLive > 0.f && elevationAboveGround > cfg.keepAliveMaxHeightAboveGround;
  if (canSink && !isSinking && timeToLive > 0.f &&
      (inactiveTime > props.inactiveTimeBeforeSink || timeToLive <= props.timeToSinkUnderground))
  {
    TMatrix wtm;
    body->getTm(wtm);
    BBox3 bbox = wtm * localPhysBBox;
    float sinkDist = 10.f;
    dacoll::tracedown_normalized(Point3::xVz(wtm.getcol(3), bbox.lim[1].y), sinkDist, nullptr, nullptr);
    sinkDist = max(sinkDist, 0.25f);
    const float sinkUndergroundGravity =
      eastl::max(0.1f, sinkDist / sqr(max(props.timeToSinkUnderground - 0.5f, 0.1f) / safediv(scaled_dt, dt)) * 2.0f /
                         /* adjust for damping */ 0.75f);
    // disable all collision and sink
    setInteractionFlags(InteractionFlag::None);
    body->setGravity(Point3(0.f, -sinkUndergroundGravity, 0.f));
    body->setVelocity(Point3::ZERO);
    body->setAngularVelocity(Point3::ZERO);
    // remove piece after sinking
    timeToLive = props.timeToSinkUnderground;
    isSinking = true;
  }

  // keep sinking
  if (isSinking)
  {
    body->activateBody(true);
    if (outOfViewTime > 0.5f && lifeTime > props.inactiveTimeBeforeSink && !props.isPersistent)
    {
      destroy(); // sinking out of view - just destroy
      return;
    }
  }

  // update time to live & other timers
  if (canSink == (timeToLive < 0.f) && !isSinking)
    timeToLive = canSink ? props.timeToLive : -1.f;
  if (timeToLive >= 0.f)
  {
    timeToLive -= scaled_dt;
    if (timeToLive <= 0.f)
      destroy();
  }

  lifeTime += scaled_dt;
  if (timeToFloat >= 0.f)
    timeToFloat -= scaled_dt;
}

void DestructableObject::Piece::destroy()
{
  G_ASSERT_RETURN(isAlive(), );
  if (body->isInWorld())
    body->getPhysWorld()->removeBody(body);
  body = nullptr;
  interactionFlags = destructables::InteractionFlag::None;
}

bool DestructableObject::update(float dt, float cur_dt_scale, bool force_inactive_timer)
{
  using namespace destructables;

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
  if (get_config().projectileCollisionMinSize < VERY_BIG_NUMBER)
  {
    Context &ctx = *g_context;
    PieceRef pieceRef{getId(), gen, 0};
    for (const Piece &piece : pieces)
    {
      if (!piece.isAlive() || !(piece.interactionFlags & InteractionFlag::Projectile))
        continue;
      const Point3 pos = Point3::xyz(piece.visualLoc.P);
      ctx.traceSpheres.push_back(v_make_vec4f(pos.x, pos.y, pos.z, piece.boundingRadSq));
      pieceRef.pieceIdx = int(&piece - pieces.data());
      ctx.tracePieces.push_back(pieceRef);
    }
  }
  if (disintegrationTime < destrObjProps->disintegrationDuration)
    disintegrationTime += scaledDt;
  return isAlive();
}

Point4 DestructableObject::getDisintegrationParams() const
{
  constexpr float BASE_DISINTEGRATION_SCALE = 50.f; // This value looks good, so this will be the base value
  return animchar_disintegration::get_additional_data(disintegrationTime,
    {.duration = destrObjProps->disintegrationDuration, .scale = destrObjProps->disintegrationScale * BASE_DISINTEGRATION_SCALE});
}

bool DestructableObject::hasDisintegrationAnimation() const { return destrObjProps->disintegrationDuration > 0; }


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

  projectileCollisionMinSize = destr_blk->getReal("projectileCollisionMinSize", projectileCollisionMinSize);

  simpleInitialImpulse = destr_blk->getBool("simpleInitialImpulse", simpleInitialImpulse);
  const DataBlock *burstBlk = destr_blk->getBlockByNameEx("destructionBurst");
  // baseline values and scaling
  destructionBurst.impactSpeed = burstBlk->getReal("impactSpeed", destructionBurst.impactSpeed);
  destructionBurst.radialSpeed = burstBlk->getReal("radialSpeed", destructionBurst.radialSpeed);
  destructionBurst.directVelFalloff = burstBlk->getReal("directVelFalloff", destructionBurst.directVelFalloff);
  destructionBurst.radialVelBaseRadius =
    max(burstBlk->getReal("radialVelBaseRadius", destructionBurst.radialVelBaseRadius), VERY_SMALL_NUMBER);
  destructionBurst.maxDirectVelBias = max(burstBlk->getReal("maxDirectVelBias", destructionBurst.maxDirectVelBias), 1.f);
  destructionBurst.penetrationBase = burstBlk->getReal("penetrationBase", destructionBurst.penetrationBase);
  destructionBurst.penetrationMax = burstBlk->getReal("penetrationMax", destructionBurst.penetrationMax);
  destructionBurst.tumbleFactor = burstBlk->getReal("tumbleFactor", destructionBurst.tumbleFactor);
  // intensity scaling
  destructionBurst.refImpulse = max(burstBlk->getReal("refImpulse", destructionBurst.refImpulse), VERY_SMALL_NUMBER);
  destructionBurst.impulseIntensityGain = burstBlk->getReal("impulseIntensityGain", destructionBurst.impulseIntensityGain);
  destructionBurst.impulseIntensityExponent = burstBlk->getReal("impulseIntensityExponent", destructionBurst.impulseIntensityExponent);
  destructionBurst.maxIntensity = max(burstBlk->getReal("maxIntensity", destructionBurst.maxIntensity), 1.f);
  // velocity field sampling
  destructionBurst.samplingScale = burstBlk->getReal("samplingScale", destructionBurst.samplingScale);
  destructionBurst.maxSamplesPerAxis = burstBlk->getInt("maxSamplesPerAxis", destructionBurst.maxSamplesPerAxis);
  destructionBurst.minSampleSpread = burstBlk->getReal("minSampleSpread", destructionBurst.minSampleSpread);
  destructionBurst.linVelSampleWeightExp = burstBlk->getReal("linVelSampleWeightExp", destructionBurst.linVelSampleWeightExp);
  // noise & jitter
  destructionBurst.noiseAmplitude = burstBlk->getReal("noiseAmplitude", destructionBurst.noiseAmplitude);
  destructionBurst.noiseScale = burstBlk->getReal("noiseScale", destructionBurst.noiseScale);
  destructionBurst.noiseLateralFrac = burstBlk->getReal("noiseLateralFrac", destructionBurst.noiseLateralFrac);
  destructionBurst.smallPieceRandomOmega = burstBlk->getReal("smallPieceRandomOmega", destructionBurst.smallPieceRandomOmega);
  destructionBurst.smallOmegaSizeThreshold = burstBlk->getReal("smallOmegaSizeThreshold", destructionBurst.smallOmegaSizeThreshold);
  // limits
  destructionBurst.hardMaxSpeed = burstBlk->getReal("hardMaxSpeed", destructionBurst.hardMaxSpeed);
  destructionBurst.hardMaxOmega = burstBlk->getReal("hardMaxOmega", destructionBurst.hardMaxOmega);
  destructionBurst.maxTipSpeed = burstBlk->getReal("maxTipSpeed", destructionBurst.maxTipSpeed);

  keepAliveMaxHeightAboveGround = destr_blk->getReal("keepAliveMaxHeightAboveGround", keepAliveMaxHeightAboveGround);
  visualErrorSmoothTime = destr_blk->getReal("visualErrorSmoothTime", visualErrorSmoothTime);
  visualErrorAccumulateTime = destr_blk->getReal("visualErrorAccumulateTime", visualErrorAccumulateTime);
  warnOnBodiesOverflow = destr_blk->getBool("warnOnBodiesOverflow", warnOnBodiesOverflow);
#if DAGOR_DBGLEVEL > 0
  errorOnBodiesOverflow = destr_blk->getBool("errorOnBodiesOverflow", errorOnBodiesOverflow);
  minBodyCountForOverflowError = destr_blk->getInt("minBodyCountForOverflowError", minBodyCountForOverflowError);
#endif
}


void init(const DataBlock *blk)
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

void DestrObjProps::loadFromBlk(const DataBlock &blk)
{
  isPersistent = blk.getBool("isPersistent", isPersistent);
  collideWithDebris = blk.getBool("collideWithDebris", collideWithDebris);

  timeToLive = blk.getReal("timeToLive", timeToLive);
  inactiveTimeBeforeSink = blk.getReal("inactiveTimeBeforeSink", blk.getReal("timeForBodies", inactiveTimeBeforeSink));
  playerCollisionTime = blk.getReal("playerCollisionTime", blk.getReal("timeToKinematic", playerCollisionTime));
  timeToSinkUnderground = blk.getReal("timeToSinkUnderground", timeToSinkUnderground);

  timeToStartDisintegration = blk.getReal("timeToStartDisintegration", timeToStartDisintegration);
  disintegrationDuration = blk.getReal("disintegrationDuration", disintegrationDuration);
  disintegrationScale = blk.getReal("disintegrationScale", disintegrationScale);
}

void DestrAnimData::loadFromBlk(const DataBlock &blk)
{
  const char *resName = blk.getStr("animRes", animRes.c_str());
  if (animRes != resName)
  {
    animRes = resName;
    anim = (AnimV20::AnimData *)get_one_game_resource_ex(resName, Anim2DataGameResClassId);
    if (anim)
      release_game_resource_ex(anim, Anim2DataGameResClassId);
    else if (!animRes.empty())
      logerr("failed to load destr anim data <%s>", resName);
  }
}

void clear()
{
  G_ASSERT_RETURN(g_context, );
  for (const auto &rec : g_context->destructablesList)
    G_VERIFY(!rec->isOwned);
  clear_and_shrink(g_context->traceSpheres);
  clear_and_shrink(g_context->tracePieces);
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

void update(float dt, float cur_time, const Point3 &view_pos)
{
  G_ASSERT_RETURN(g_context, );
  TIME_PROFILE(destructables_update);
  Context &ctx = *g_context;
  const DestructablesConfig &config = ctx.config;
  ctx.viewPos = view_pos;
  ctx.curTime = cur_time;

  ctx.traceSpheres.clear();
  ctx.tracePieces.clear();

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
                                    if (!object->isOwned && deleteLimit-- > 0)
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
      if (!destr.isAlive() || destr.destrObjProps->isPersistent)
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
    for (int pass = 0; pass < 3; pass++)
    {
      const bool checkOutOfView = pass < 1;
      const bool checkPersistent = pass < 2;
      for (const auto &destrPtr : ctx.destructablesList)
      {
        if (ctx.numAliveBodies <= config.maxNumberOfDestructableBodies)
          break;
        DestructableObject &destr = *destrPtr;
        if (!destr.isAlive() || (destr.destrObjProps->isPersistent && checkPersistent))
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

static __forceinline bool trace_local_bbox(vec3f from, vec3f to, vec3f dir, const BBox3 &box, float &in_out_t, vec3f &out_norm)
{
  const bbox3f vbox = v_ldu_bbox3(box);
  if (v_bbox3_test_pt_inside(vbox, from))
  {
    in_out_t = 0.f;
    out_norm = v_neg(dir);
    return true;
  }
  float atMin = 0.f, atMaxUnused = 0.f;
  const int side = v_segment_box_intersection_side(from, to, vbox, atMin, atMaxUnused);
  if (side < 0)
    return false;
  vec4f_const BOX_SIDE_NORMALS[6] = {{-1.f, 0.f, 0.f, 0.f}, {0.f, -1.f, 0.f, 0.f}, {0.f, 0.f, -1.f, 0.f}, {1.f, 0.f, 0.f, 0.f},
    {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}};
  in_out_t *= atMin;
  out_norm = BOX_SIDE_NORMALS[side];
  return true;
}

void trace_ray(const Point3 &from, const Point3 &dir, float max_t, TraceHitList &out_hits)
{
  G_ASSERT_RETURN(g_context, );
  Context &ctx = *g_context;
  const vec4f vFrom = v_ldu_p3(&from.x), vDir = v_ldu_p3(&dir.x), vMaxT = v_splats(max_t);
  for (int i = 0; i < ctx.traceSpheres.size(); i++)
  {
    const vec4f sphere = ctx.traceSpheres[i];
    if (DAGOR_LIKELY(!v_test_ray_sphere_intersection(vFrom, vDir, vMaxT, sphere, v_splat_w(sphere))))
      continue;
    const PieceRef &ref = ctx.tracePieces[i];
    const DestructableObject::Piece &piece = static_cast<DestructableObject *>(ref.id)->pieces[ref.pieceIdx];
    if (!piece.isAlive())
      continue;
    const Point3 piecePos = Point3::xyz(piece.visualLoc.P);
    const quat4f rot = v_ldu(&piece.visualLoc.O.getQuat().x);
    const quat4f invRot = v_quat_conjugate(rot);
    const vec3f localFrom = v_quat_mul_vec3(invRot, v_sub(vFrom, v_ldu_p3(&piecePos.x)));
    const vec3f localDir = v_quat_mul_vec3(invRot, vDir);
    float hitT = max_t;
    vec3f localNorm;
    if (!trace_local_bbox(localFrom, v_madd(localDir, vMaxT, localFrom), localDir, piece.localPhysBBox, hitT, localNorm))
      continue;
    TraceHit &hit = out_hits.push_back();
    hit.ref = ref;
    hit.t = hitT;
    hit.pos = from + dir * hitT;
    v_stu_p3(&hit.normal.x, v_quat_mul_vec3(rot, localNorm));
    hit.physMatId = PhysMat::getMaterialIdByPhysBodyMaterial(piece.body->getMaterialId());
  }
  stlsort::sort_branchless(out_hits.begin(), out_hits.end(), [](const auto &a, const auto &b) { return a.t < b.t; });
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
