// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanPhys.h>

#include <gamePhys/props/atmosphere.h>
#include <gamePhys/props/softMaterialProps.h>
#include <gamePhys/props/physDestructibleProps.h>
#include <gamePhys/props/physContactProps.h>
#include <EASTL/sort.h>

#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/utils.h>
#include <gamePhys/phys/physDebugDraw.h>
#include <gamePhys/phys/walker/humanWeapPosInfo.h>
#include <gamePhys/phys/walker/humanPhysUtils.h>

#include <gameMath/traceUtils.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>

#include <math/dag_noise.h>
#include <math/dag_adjpow2.h>
#include <math/dag_geomTree.h>
#include <math/dag_capsule.h>
#include <math/dag_math3d.h>

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug3d.h>

#include <util/dag_convar.h>

#include <perfMon/dag_cpuFreq.h>

#include <gamePhys/common/fixed_dt.h>
#if !DISABLE_SYNC_DEBUG
#include <syncDebug/syncDebug.h>
#else
#define FM_SYNC_DEBUG(...)
#endif
#include <scene/dag_physMat.h>

#include <debug/dag_textMarks.h>

template class PhysicsBase<HumanPhysState, HumanControlState, CommonPhysPartialState>;

// This assertion ensures that this trait is not broken by accident (as it speed-up relocation/copying). Remove if necessary.
static_assert(eastl::is_trivially_destructible_v<HumanPhysState>);

static Point2 calc_ori_param(const HumanPhysState &state, const HumanControlState &ct)
{
  bool isStanding = state.moveState == EMS_STAND || state.moveState == EMS_ROTATE_LEFT || state.moveState == EMS_ROTATE_RIGHT;
  float moveMult = isStanding ? 0.f : 1.f;
  float walkYaw = dir_to_yaw(state.walkDir);
  Point3 gunDir = ct.getWishShootDir();
  float oriYaw = dir_to_yaw(gunDir);
  float deltaYaw = oriYaw - walkYaw;
  return yaw_to_2d_dir(deltaYaw) * moveMult;
}

static Quat calc_projected_orientation(const Point3 &fwd_dir, const Point3 &up)
{
  TMatrix resTm{};
  resTm.setcol(0, normalize(fwd_dir - fwd_dir * up * up));
  resTm.setcol(1, up);
  resTm.setcol(2, normalize(resTm.getcol(0) % up));
  return Quat(resTm);
}

static constexpr int CLIMB_CAST_FLAGS = dacoll::ETF_DEFAULT | dacoll::ETF_RI_PHYS;
static constexpr int WALK_TRACE_FLAGS = dacoll::ETF_DEFAULT | dacoll::ETF_RI_PHYS;

static constexpr float FALLBACK_TO_SPHERE_CAST_TICKS = 10.f;
static constexpr float FALLBACK_TO_SPHERE_TRACE_THRESHOLD = 0.2f;
static constexpr float SMALL_DIST = 0.01f;
static const float VERT_DIR_COS_EPS = cos(DegToRad(1.f));

// Values below these known to have side effects such as
// climbing misbehavior, these can be lowered only with climbing logic change.
static constexpr int CLIMB_RAYS_INITIAL = 3;
static constexpr int CLIMB_RAYS_FORWARD = 5;
static constexpr int CLIMB_RAYS_SIDEWAYS = 2;


HumanPhysState::HumanPhysState()
{
  overrideWallClimb = false;
  alive = true;
  isDowned = false;
  controllable = true;
  ignoresHumanCollision = false;
  canShoot = true;
  canAim = true;
  canZoom = true;
  disableCollision = false;
  forceWeaponDown = false;
  forceWeaponUp = false;
  isClimbing = false;
  isClimbingOverObstacle = false;
  isFastClimbing = false;
  isSwimming = false;
  isUnderwater = false;
  isHoldBreath = false;
  attachedToLadder = false;
  pulledToLadder = false;
  blockSprint = false;
  attachedToExternalGun = false;
  climbThrough = false;
  isAttached = false;
  cancelledClimb = false;
  climbGridCounter = 0;
  externalControlled = false;
  reduceToWalk = false;
  isLadderQuickMovingUp = false;
  isLadderQuickMovingDown = false;

  height = 1.f; // standing
  heightCurVel = 0.f;
  reset();
  gunDir.set(1.f, 0.f, 0.f);
  headDir.set(1.f, 0.f, 0.f);
  gunAngles.set(0.f, 0.f);
  targetGunSpd.set(0.f, 0.f);
  prevAngles.set(0.f, 0.f);
  breathOffset.zero();
  handsShakeOffset.zero();

  atTick = -1;
  canBeCheckedForSync = true;
  useSecondaryCcd = true;
  isVisible = true;
}

void HumanPhysState::resetStamina(float max_stamina) { stamina = max_stamina * maxStaminaMult * staminaBoostMult; }

void HumanPhysState::reset()
{
  lastAppliedControlsForTick = -1; // invalidate prev controls
  velocity.zero();
  standingVelocity.zero();
  gunSpd.set(0.f, 0.f);
  acceleration.zero();
  omega.zero();
  alpha.zero();
  setAimPos(0);
  breathShortness = 0.f;
  breathShakeMult = 1.f;
  handsShakeSpeedMult = 1.f;
  handsShakeMagnitude = 0.f;
  breathTimer = 0.f;
  knockBackTimer = 0.f;
  spdSummaryDiff = ZERO<Point3>();
  deltaVelIgnoreAmount = 0.f;

  posOffset.zero();

  kineticEnergy = 0.f;

  collisionLinksStateFrom = collisionLinksStateTo = getStandState();
  collisionLinkProgress = -1.f;


  weapEquipState.reset();

  walkDir.set(1.f, 0.f);
  bodyOrientDir.set(1.f, 0.f);

  resetStamina(100.f);
  stoppedSprint = false;
  isInAirHistory = 0;

  moveState = EMS_STAND;

  alive = true;

  controllable = true;
  canShoot = true;

  staminaDrainMult = 1.f;

  useSecondaryCcd = true;
  isVisible = true;
  externalControlled = false;
  reduceToWalk = false;
  isLadderQuickMovingUp = false;
  isLadderQuickMovingDown = false;
}

void HumanPhysState::applyPartialState(const CommonPhysPartialState &state)
{
  location = state.location;
  velocity = state.velocity;
}

void HumanPhysState::applyDesyncedState(const HumanPhysState & /*state*/) {}

void HumanPhysState::applyAlternativeHistoryState(const HumanPhysState &state)
{
  weapEquipState = state.weapEquipState;
  gunAimOffset = state.gunAimOffset;
  canZoom = state.canZoom;
  canAim = state.canAim;
  zoomPosition = state.zoomPosition;
}

void HumanPhysState::serialize(danet::BitStream &bs) const
{
  bs.Write(*static_cast<const HumanSerializableState *>(this));
  write_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
}

bool HumanPhysState::deserialize(const danet::BitStream &bs, IPhysBase &)
{
  if (!bs.Read(static_cast<HumanSerializableState &>(*this)))
    return false;
  return read_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
}

HumanPhys::HumanPhys(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step) :
  BaseType(physactor_offset, phys_vars, time_step), humanCollision(new HumanCollision)
{
  for (int i = 0; i < ESS_NUM; ++i)
  {
    walkSpeeds[i][EMS_STAND] = 0.f;
    walkSpeeds[i][EMS_WALK] = 1.f;
    walkSpeeds[i][EMS_RUN] = 3.5f;
    walkSpeeds[i][EMS_SPRINT] = 5.f;
    walkSpeeds[i][EMS_ROTATE_LEFT] = 0.f;
    walkSpeeds[i][EMS_ROTATE_RIGHT] = 0.f;
    rotateSpeeds[i] = TWOPI;
    alignSpeeds[i] = 0.f;
    rotateAngles[i].set(-HALFPI, HALFPI * 0.5f);

    wishVertJumpSpeed[i][EMS_STAND] = wishVertJumpSpeed[i][EMS_WALK] = wishVertJumpSpeed[i][EMS_RUN] =
      wishVertJumpSpeed[i][EMS_ROTATE_RIGHT] = wishVertJumpSpeed[i][EMS_ROTATE_LEFT] = 4.5f;
    wishVertJumpSpeed[i][EMS_SPRINT] = 3.f;
    mem_set_0(wishHorzJumpSpeed[i]);
  }
  v_bbox3_init_empty(traceCacheOOBB);
  mem_set_0(weaponParams);
  const float traceCacheExp = 1.f;
  traceCacheExpands = v_splats(traceCacheExp);
}

HumanPhys::~HumanPhys()
{
  if (torsoCollision)
    dacoll::destroy_dynamic_collision(torsoCollision);
  if (wallJumpCollision)
    dacoll::destroy_dynamic_collision(wallJumpCollision);
  if (climberCollision)
    dacoll::destroy_dynamic_collision(climberCollision);
}

void HumanPhys::loadFromBlk(const DataBlock *blk, const CollisionResource * /*collision*/, const char * /*unit_name*/,
  bool /*is_player*/)
{
  rayMatId = PhysMat::getMaterialId(blk->getStr("rayMat", "walkRay"));
  setCollisionMatId(PhysMat::getMaterialId(blk->getStr("collisionMat", "walker")));
  float invScale = safeinv(scale);
  collRad = blk->getReal("collisionRadius", 0.2f) * scale;
  walkRad = blk->getReal("walkSphereCastRadius", collRad * 0.8f * invScale) * scale;
  ccdRad = blk->getReal("ccdRadius", collRad * 0.8f * invScale) * scale;

  zoomSpeed = blk->getReal("zoomSpeed", zoomSpeed);

  hasGuns = blk->getBool("hasGuns", hasGuns);
  hasExternalHeight = blk->getBool("hasExternalHeight", hasExternalHeight);

  speedCollisionHardness = blk->getReal("speedCollisionHardness", speedCollisionHardness);

  maxObstacleHeight = blk->getReal("maxObstacleHeight", maxObstacleHeight) * scale;
  maxStepOverHeight = blk->getReal("maxStepOverHeight", maxStepOverHeight) * scale;
  maxCrawlObstacleHeight = blk->getReal("maxCrawlObstacleHeight", maxObstacleHeight * invScale) * scale;
  maxObstacleDownReach = blk->getReal("maxObstacleDownReach", maxObstacleDownReach);
  forceDownReachDists = blk->getPoint2("forceDownReachDists", forceDownReachDists);
  forceDownReachVel = blk->getReal("forceDownReachVel", forceDownReachVel);

  crawlLegsOffset = blk->getReal("crawlLegsOffset", crawlLegsOffset) * scale;
  crawlArmsOffset = blk->getReal("crawlArmsOffset", crawlArmsOffset) * scale;

  heightChangeDeltaClampMin = blk->getReal("heightChangeDeltaClampMin", heightChangeDeltaClampMin);
  heightChangeDeltaClampMax = blk->getReal("heightChangeDeltaClampMax", heightChangeDeltaClampMax);
  float tau = blk->getReal("heightChangeTau", heightChangeTauDef);
  float delta = blk->getReal("heightChangeSpringK", heightChangeDeltaDef);
  heightChangeSprK = calc_spring_k(tau, delta);
  heightChangeDamp = calc_damping(tau, delta);
  // debug("tau=%.4f delta=%.4f -> sprK=%.4f damp=%.4f", tau, delta, heightChangeSprK, heightChangeDamp);

  {
    DataBlock wallJumpProps;
    DataBlock *wallJumpBlk = wallJumpProps.addNewBlock("sphere");
    wallJumpBlk->addTm("tm", TMatrix::IDENT);
    wallJumpBlk->addReal("rad", blk->getReal("wallJumpRad", 0.5f));
    wallJumpCollision = dacoll::add_dynamic_collision(wallJumpProps, NULL, false, false);

    wallJumpOffset = blk->getPoint3("wallJumpOffset", wallJumpOffset);
    wallJumpSpd = blk->getReal("wallJumpSpeed", wallJumpSpd);
    canWallJump = blk->getBool("canWallJump", canWallJump);
  }

  if (const DataBlock *climberCollBlk = blk->getBlockByName("climberCollision"))
    climberCollision = dacoll::add_dynamic_collision(*climberCollBlk, NULL, false, false);

  {
    const DataBlock *weaponsBlk = blk->getBlockByNameEx("weapons");
    for (int i = 0; i < EWS_NUM; ++i)
    {
      const DataBlock *slotBlk = weaponsBlk->getBlockByNameEx(humanWeaponSlotNames[i], NULL);
      if (slotBlk)
        weaponParams[i].loadFromBlk(slotBlk);
    }
  }
  maxStamina = blk->getReal("stamina", maxStamina);
  sprintStaminaDrain = blk->getReal("sprintStaminaDrain", sprintStaminaDrain);
  restStaminaRestore = blk->getReal("restStaminaRestore", restStaminaRestore);
  jumpStaminaDrain = blk->getReal("jumpStaminaDrain", jumpStaminaDrain);
  climbStaminaDrain = blk->getReal("climbStaminaDrain", climbStaminaDrain);
  sprintStartStaminaLevel = blk->getReal("sprintStartStaminaLevel", sprintStartStaminaLevel);
  sprintEndStaminaLevel = blk->getReal("sprintEndStaminaLevel", sprintEndStaminaLevel);
  airStaminaRestoreMult = blk->getReal("airStaminaRestoreMult", airStaminaRestoreMult);

  climbOnPos = blk->getPoint3("climbOnPos", climbOnPos) * scale;
  climbOnRad = blk->getReal("climbOnRad", climbOnRad);
  climbLadderHeight = blk->getReal("climbLadderHeight", climbLadderHeight);
  climbPositions = blk->getInt("climbPositions", climbPositions);
  climbDetachProjLen = blk->getReal("climbDetachProjLen", climbDetachProjLen);

  additionalClimbRad = blk->getReal("additionalClimbRad", additionalClimbRad);
  swimOffsetForClimb = blk->getReal("swimOffsetForClimb", swimOffsetForClimb);
  additionalClimbPositions = blk->getInt("additionalClimbPositions", additionalClimbPositions);
  climbGridFrames = max(blk->getInt("climbGridFrames", climbGridFrames), 1);
  maxClimbPositionsPerFrame = (sqr(max(additionalClimbPositions, 1) * 2 - 1) + climbGridFrames - 1) / climbGridFrames;

  maxClimbSpeed = blk->getReal("maxClimbSpeed", maxClimbSpeed);
  climbVertAccel = blk->getReal("climbVertAccel", climbVertAccel);
  climbVertBrake = blk->getReal("climbVertBrake", climbVertBrake);
  climbVertBrakeMaxTime = blk->getReal("climbVertBrakeMaxTime", climbVertBrakeMaxTime);
  maxClimbHorzVel = blk->getReal("maxClimbHorzVel", maxClimbHorzVel);
  climbHorzAccel = blk->getReal("climbHorzAccel", climbHorzAccel);
  climbHorzBrake = blk->getReal("climbHorzBrake", climbHorzBrake);
  climbBrakeHeight = blk->getReal("climbBrakeHeight", climbBrakeHeight);
  climbTimeout = blk->getReal("climbTimeout", climbTimeout);
  climbAngleCos = cosf(DegToRad(blk->getReal("climbAngle", 45.f)));
  climbMinHorzSize = blk->getReal("climbMinHorzSize", climbMinHorzSize);
  climbOnMinVertSize = blk->getReal("climbOnMinVertSize", climbOnMinVertSize);
  climbThroughMinVertSize = blk->getReal("climbThroughMinVertSize", climbThroughMinVertSize);
  climbThroughForwardDist = blk->getReal("climbThroughForwardDist", climbThroughForwardDist);
  climbThroughPosOffset = blk->getPoint3("climbThroughPosOffset", climbThroughPosOffset);
  climbOverMaxHeight = blk->getReal("climbOverMaxHeight", climbOverMaxHeight);
  climbOverForwardOffset = blk->getReal("climbOverForwardOffset", climbOverForwardOffset);
  climbOverMinHeightBehindObstacle = blk->getReal("climbOverMinHeightBehindObstacle", climbOverMinHeightBehindObstacle);
  climbOverStaticVelocity = blk->getReal("climbOverStaticVelocity", climbOverStaticVelocity);
  climbMaxVelLength = blk->getReal("climbMaxVelLength", climbMaxVelLength);
  maxHeightForFastClimbing = blk->getReal("maxHeightForFastClimbing", maxHeightForFastClimbing);
  fastClimbingMult = blk->getReal("fastClimbingMult", fastClimbingMult);
  climbOverDistance = blk->getReal("climbOverDistance", climbOverDistance);

  turnSpeedStill = blk->getReal("turnSpeedStill", 2.f) * PI;
  turnSpeedWalk = blk->getReal("turnSpeedWalk", 1.5f) * PI;
  turnSpeedRun = blk->getReal("turnSpeedRun", 1.f) * PI;
  turnSpeedSprint = blk->getReal("turnSpeedSprint", 0.5f) * PI;

  defAimSpeed = aimSpeed = blk->getReal("aimSpeed", aimSpeed);

  isInertMovement = blk->getBool("isInertMovement", isInertMovement);
  climber = blk->getBool("climber", climber);
  canCrawl = blk->getBool("canCrawl", canCrawl);
  canCrouch = blk->getBool("canCrouch", canCrouch);
  canClimb = blk->getBool("canClimb", canClimb);
  canSprint = blk->getBool("canSprint", canSprint);
  canJump = blk->getBool("canJump", canJump);
  canSwim = blk->getBool("canSwim", canSwim);
  canStartSprintWithResistance = blk->getBool("canStartSprintWithResistance", canStartSprintWithResistance);
  additionalHeightCheck = blk->getBool("additionalHeightCheck", additionalHeightCheck);

  allowWeaponSwitchOnSprint = blk->getBool("allowWeaponSwitchOnSprint", allowWeaponSwitchOnSprint);

  leanDegrees = blk->getReal("leanDegrees", leanDegrees);

  maxSprintSin = sinf(DegToRad(blk->getReal("maxSprintAngle", 10.f)));
  maxRunSin = sinf(DegToRad(blk->getReal("maxRunAngle", 45.f)));


  stateChangeCollAnimSpeed = blk->getReal("stateChangeCollAnimSpeed", stateChangeCollAnimSpeed);

  moveStateThreshold = blk->getReal("moveStateThreshold", moveStateThreshold);

  acceleration = blk->getReal("acceleration", acceleration);
  frictionGroundCoeff = blk->getReal("frictionGroundCoeff", frictionGroundCoeff);
  frictionThresSpdMult = blk->getReal("frictionThresSpdMult", frictionThresSpdMult);

  for (int i = 0; i < EMS_NUM; ++i)
  {
    fricitonByState[i] = frictionGroundCoeff;
    accelerationByState[i] = acceleration;
  }

  externalFrictionPerSpeedMult = blk->getReal("externalFrictionPerSpeedMult", externalFrictionPerSpeedMult);
  fricitonByState[EMS_WALK] = blk->getReal("frictionGroundCoeffWalk", fricitonByState[EMS_WALK]);
  fricitonByState[EMS_RUN] = blk->getReal("frictionGroundCoeffRun", fricitonByState[EMS_RUN]);
  fricitonByState[EMS_SPRINT] = blk->getReal("frictionGroundCoeffSprint", fricitonByState[EMS_SPRINT]);

  accelerationByState[EMS_WALK] = blk->getReal("accelerationWalk", accelerationByState[EMS_WALK]);
  accelerationByState[EMS_RUN] = blk->getReal("accelerationRun", accelerationByState[EMS_RUN]);
  accelerationByState[EMS_SPRINT] = blk->getReal("accelerationSprint", accelerationByState[EMS_SPRINT]);

  airAccelerate = blk->getReal("airAccelerate", airAccelerate);

  minWalkControl = blk->getReal("minWalkControl", minWalkControl);
  stepAccel = blk->getReal("stepAccel", stepAccel);

  crawlStepSpeed = blk->getReal("crawlStepSpeed", crawlStepSpeed);

  static const char *essNames[] = {"standState", "crouchState", "crawlState", "downedState", "swimState", "swimUnderwaterState",
    "climbState", "climbThroughState", "climbOverObstacleState", "climbLadderState", "externallyControlledState"};
  G_STATIC_ASSERT(countof(essNames) == ESS_NUM);
  for (int i = 0; i < ESS_NUM; ++i)
  {
    const DataBlock *stateBlk = blk->getBlockByNameEx(essNames[i]);
    walkSpeeds[i][EMS_WALK] = stateBlk->getReal("walkSpeed", blk->getReal("walkSpeed", 1.f));
    walkSpeeds[i][EMS_RUN] = stateBlk->getReal("runSpeed", blk->getReal("runSpeed", 3.5f));
    walkSpeeds[i][EMS_SPRINT] = stateBlk->getReal("sprintSpeed", blk->getReal("sprintSpeed", 5.f));
    rotateSpeeds[i] = DegToRad(stateBlk->getReal("rotateSpeed", 360.f));
    alignSpeeds[i] = DegToRad(stateBlk->getReal("alignSpeed", 0.f));
    rotateAngles[i] = DEG_TO_RAD * stateBlk->getPoint2("rotateAngles", Point2(-90.f, 45.f));
    wishVertJumpSpeed[i][EMS_STAND] = wishVertJumpSpeed[i][EMS_WALK] = wishVertJumpSpeed[i][EMS_RUN] =
      wishVertJumpSpeed[i][EMS_ROTATE_RIGHT] = wishVertJumpSpeed[i][EMS_ROTATE_LEFT] =
        stateBlk->getReal("jumpVertSpeed", blk->getReal("jumpVertSpeed", 4.5f)) * scale;
    wishVertJumpSpeed[i][EMS_SPRINT] = stateBlk->getReal("sprintJumpVertSpeed", blk->getReal("sprintJumpVertSpeed", 3.f)) * scale;
    wishHorzJumpSpeed[i][EMS_SPRINT] = stateBlk->getReal("sprintJumpHorzSpeed", blk->getReal("sprintJumpHorzSpeed", 0.f));
    if (const DataBlock *collLinkBlk = stateBlk->getBlockByNameEx("collisionLinks", NULL))
      dacoll::load_collision_links(collisionLinks[i], *collLinkBlk, scale);

    collisionCenterPos[i] =
      stateBlk->getPoint3("collisionCenterPos", blk->getPoint3("collisionCenterPos", Point3(0.f, 0.f, 0.f))) * scale;
    ccdPos[i] = stateBlk->getPoint3("ccdPos", collisionCenterPos[i] + Point3(0.f, maxObstacleHeight + ccdRad, 0.f)) * scale;

    float jumpSpd = wishVertJumpSpeed[i][EMS_STAND];
    float jumpT = jumpSpd / gamephys::atmosphere::g();
    maxJumpHeight = max(maxJumpHeight, jumpSpd * jumpT - gamephys::atmosphere::g() * sqr(jumpT) * 0.5f);
  }

  {
    DataBlock torsoCollProps;
    DataBlock *torsoCylBlk = torsoCollProps.addNewBlock("capsule");
    torsoCylBlk->addTm("tm", TMatrix::IDENT);
    torsoCylBlk->addReal("rad", collRad);
    torsoCylBlk->addReal("height", blk->getReal("collisionHeight", 1.f));
    // torsoCylBlk->addPoint3("width", Point3(collRad, 1.f, collRad)); // identity height to scale it
    isTorsoInWorld = true;
    TMatrix cotm = getCollisionObjectsMatrix(); // After `collisionLinks` load
    torsoCollision =
      dacoll::add_dynamic_collision(torsoCollProps, &humanCollision->collObjUd, false, isTorsoInWorld, dacoll::EPL_ALL, &cotm);
  }

  if (!blk->getBlockByNameEx(essNames[ESS_CLIMB], nullptr))
    canClimb = false;
  if (!blk->getBlockByNameEx(essNames[ESS_CLIMB_THROUGH], nullptr))
    climbThroughMinVertSize = 0.0f;

  {
    enableSprintStopCollision = blk->getBool("enableSprintStopCollision", enableSprintStopCollision);
    sprintStopCollisionRad = blk->getReal("sprintStopCollisionRad", sprintStopCollisionRad);
    sprintStopCollisionStart =
      blk->getPoint3("sprintStopCollisionStart", collisionCenterPos[ESS_STAND] + Point3(0.f, maxObstacleHeight, 0.f));
    sprintStopCollisionEnd =
      blk->getPoint3("sprintStopCollisionEnd", collisionCenterPos[ESS_STAND] + Point3(collRad, maxObstacleHeight, 0.f));
  }

  minWalkSpeed = blk->getReal("minWalkSpeed", minWalkSpeed);
  aimingMoveSpeed = blk->getReal("aimingMoveSpeed", aimingMoveSpeed);
  waterFrictionResistance = blk->getPoint2("waterFriction", Point2(0.5f, 0.75f));
  waterJumpingResistance = blk->getPoint2("waterJumpingResistance", Point2(0.25f, 0.9f));

  standSlideAngle = cosf(DegToRad(blk->getReal("standSlideAngle", standSlideAngle)));
  crawlSlideAngle = cosf(DegToRad(blk->getReal("crawlSlideAngle", crawlSlideAngle)));
  climbSlideAngle = cosf(DegToRad(blk->getReal("climbSlideAngle", climbSlideAngle)));

  crawlWalkNormTau = blk->getReal("crawlWalkNormTau", crawlWalkNormTau);

  ladderClimbSpeed = blk->getReal("ladderClimbSpeed", ladderClimbSpeed);
  ladderQuickMoveUpSpeedMult = blk->getReal("ladderQuickMoveUpSpeedMult", ladderQuickMoveUpSpeedMult);
  ladderQuickMoveDownSpeedMult = blk->getReal("ladderQuickMoveDownSpeedMult", ladderQuickMoveDownSpeedMult);
  ladderCheckClimbHeight = blk->getReal("ladderCheckClimbHeight", ladderCheckClimbHeight);
  ladderZeroVelocityHeight = blk->getReal("ladderZeroVelocityHeight", ladderZeroVelocityHeight);
  ladderMaxOverHeight = blk->getReal("ladderMaxOverHeight", ladderMaxOverHeight);

  canFinishClimbing = blk->getBool("canFinishClimbing", canFinishClimbing);

  gunNodeName = blk->getStr("gunNode", "Bip01 Neck");
  gunLinkNameId = dacoll::get_link_name_id(gunNodeName);

  Point2 traceCacheSizeHorz = Point2(1.0f, 1.0f) * max(Point3::x0z(climbOnPos).length(), collRad) +
                              blk->getPoint2("traceCacheAdditionalSizeHorz", Point2(0.7f, 0.7f));
  Point2 traceCacheSizeVert = Point2(0.0, climbOnPos.y) + blk->getPoint2("traceCacheAdditionalSizeVert", Point2(1.0f, 0.2f));

  BBox3 traceCacheBox(Point3(-traceCacheSizeHorz.x, -traceCacheSizeVert.x, -traceCacheSizeHorz.y),
    Point3(traceCacheSizeHorz.x, traceCacheSizeVert.y, traceCacheSizeHorz.y));
  traceCacheOOBB = v_ldu_bbox3(traceCacheBox);

  Point3 traceCacheExp = blk->getPoint3("traceCacheExpands", Point3(1.0f, 1.0f, 1.0f));
  traceCacheExpands = v_ldu(&traceCacheExp.x);

  currentState.resetStamina(maxStamina * currentState.maxStaminaMult * currentState.staminaBoostMult);

  beforeJumpDelay = blk->getReal("beforeJumpDelay", beforeJumpDelay);
  beforeJumpCrouchHeight = blk->getReal("beforeJumpCrouchHeight", beforeJumpCrouchHeight);
  jumpInAirHistoryMask = blk->getInt("jumpInAirHistoryMask", jumpInAirHistoryMask);
}

template <typename T>
inline T move_to_vec(const T &from, const T &to, float dt, float vel)
{
  float d = vel * dt;
  T dir = to - from;
  float dirLenSq = lengthSq(dir);
  if (dirLenSq <= sqr(d))
    return to;

  return from + dir * safediv(d, sqrtf(dirLenSq));
}

Point3 HumanPhys::calcGunPos(const TMatrix &tm, float gun_angles, float lean_pos, float height, PrecomputedPresetMode mode) const
{
  float bodyYaw = dir_to_yaw(currentState.bodyOrientDir);
  Point2 fwd = Point2::xz(currentState.location.O.getQuat().getForward());
  float persCourse = renorm_ang(dir_to_yaw(fwd), bodyYaw);
  float bodyDir = RadToDeg(bodyYaw - persCourse);
  return tm * (get_weapon_pos(precompWeaponPos, mode, height, RadToDeg(gun_angles), lean_pos, bodyDir) + gunOffset);
}

TMatrix HumanPhys::calcGunTm(const TMatrix &tm, float gun_angles, float lean_pos, float height, PrecomputedPresetMode mode) const
{
  Point3 pos = calcGunPos(tm, gun_angles, lean_pos, height, mode);
  TMatrix rotTm = makeTM(tm.getcol(2), gun_angles);
  TMatrix outTm = tm;
  outTm = rotTm % outTm;
  outTm.setcol(3, pos);
  return outTm;
}

void HumanPhys::applyStandingImpulse(const gamephys::CollisionContactData &contact, Point3 &pos_offs)
{
  const float slideThreshold = currentState.isCrawl() ? crawlSlideAngle : standSlideAngle;
  Point3 appliedNormal = contact.wnormB;
  Point3 gravDir = -currentState.gravDirection;
  float angleWithGravity = appliedNormal * gravDir;
  if (angleWithGravity > slideThreshold)
  {
    float vertOffset = max(contact.wnormB * -gravDir * contact.depth - pos_offs * gravDir, 0.f);
    pos_offs += gravDir * vertOffset;
    appliedNormal = gravDir;
  }
  else
    applyPush(contact, pos_offs);
  float spdDiff = min(currentState.velocity * appliedNormal, 0.f);
  currentState.spdSummaryDiff -= appliedNormal * spdDiff;
  currentState.velocity = currentState.velocity - spdDiff * appliedNormal;
}

void HumanPhys::applyPushingImpulse(const gamephys::CollisionContactData &contact, Point3 &pos_offs, float hardness,
  float speed_hardness)
{
  applyPush(contact, pos_offs, hardness);
  float spdDiff = min(currentState.velocity * contact.wnormB, 0.f) * speed_hardness;
  currentState.spdSummaryDiff -= contact.wnormB * spdDiff;
  currentState.velocity = currentState.velocity - spdDiff * contact.wnormB;
}

void HumanPhys::applyPush(const gamephys::CollisionContactData &contact, Point3 &pos_offs, float hardness)
{
  pos_offs += contact.wnormB * max(-contact.depth * hardness - pos_offs * contact.wnormB, 0.f);
}

static Point3 project_dir(const Point3 &dir, const Point3 &norm, float overbounce = 1.f)
{
  float proj = dir * norm;
  return dir - (proj > 0.f ? (proj / overbounce) : (proj * overbounce)) * norm;
}

static Point3 clip_dir(const Point3 &vel, const Point3 &normal, float power = 1.f)
{
  return vel - power * min(vel * normal, 0.f) * normal;
}

static Point3 climb_dir(const Point3 &vel, const Point3 &normal, const Point3 &up, float overbounce = 1.f)
{
  Point3 left = normalize(vel % up);
  Point3 correctedNormal = normalize(normal - (normal * left) * left);
  const float proj = vel * correctedNormal;
  const float overbounceProj = proj > 0.f ? (proj / overbounce) : (proj * overbounce);
  return vel - overbounceProj * correctedNormal;
}

bool HumanPhys::processCcdOffset(TMatrix &tm, const Point3 &to_pos, const Point3 &offset, float collision_margin, float speed_hardness,
  bool secondary_ccd, const Point3 &ccd_pos)
{
  constexpr real MIN_OFFSET_SQ = 1e-5f;  // NOTE: sqrt(0.00001f) ~ 0.0031622 ~ 3 millimeters
  if (lengthSq(offset) <= MIN_OFFSET_SQ) // don't check CCD for too small offsets to avoid math errors
  {
    if (isSimplifiedPhys)
      applyOffset(-offset); // for bots don't allow EVEN small movements without CCD
    return false;
  }

  Point3 fromPos = to_pos - offset;
  dacoll::ShapeQueryOutput shapeQuery;
  bool res = dacoll::sphere_cast(fromPos, to_pos, ccdRad, shapeQuery, rayMatId, getTraceHandle());
  if (res)
  {
    // Allow unrestricted offset, this is always safe, since we've sphere casted it.
    Point3 overMovement = offset * (1.f - shapeQuery.t);
    applyOffset(-overMovement);
    currentState.location.toTM(tm);

    const float smallOffset = 0.001f;
    if (DAGOR_LIKELY(collision_margin > smallOffset))
    {
      // We also want to apply appliedNormal * collision_margin, but this is not safe, since we didn't sphere casted that area.
      // It might seem to be "ok" because collision_margin is not "very big", but the thing is that you
      // can get sucked into other geometry with it during several frames, sphere cast above can push you further and further
      // if there's some other collision on the other side. In order to mitigate this we start another sphere cast.
      Point3 appliedNormal = -offset * safeinv(offset.length());
      // We'll use smallOffset so that ccd sphere starts NOT within / touching some geometry.
      float marginDist = collision_margin + smallOffset;
      Point3 marginOffset = appliedNormal * marginDist;
      Point3 tmpPos = tm * ccd_pos;
      dacoll::ShapeQueryOutput shapeQueryCm;
      bool resCm = dacoll::sphere_cast(tmpPos, tmpPos + marginOffset, ccdRad - smallOffset, shapeQueryCm, rayMatId, getTraceHandle());
      if (resCm)
        applyOffset(marginOffset * shapeQueryCm.t * 0.5f);
      else
        applyOffset(marginOffset);
      currentState.location.toTM(tm);
    }

    Point3 spdDiff = min(currentState.velocity * shapeQuery.norm, 0.0f) * speed_hardness * shapeQuery.norm;
    currentState.spdSummaryDiff -= spdDiff;
    currentState.velocity -= spdDiff;

    if (secondary_ccd)
    {
      // Try restricted part constrained by sphere_cast normal
      Point3 restrictedOffset = clip_dir(overMovement, shapeQuery.norm) + /*try finish the move*/
                                clamp(((shapeQuery.t * -offset) * shapeQuery.norm), 0.f, collision_margin) *
                                  shapeQuery.norm /*try to become parallel to normal*/;
      applyOffset(restrictedOffset);
      currentState.location.toTM(tm);
      processCcdOffset(tm, tm * ccd_pos, restrictedOffset, collision_margin, speed_hardness, false, ccd_pos);
      return true;
    }
  }
  FM_SYNC_DEBUG(WALKER_POS, P3D(currentState.location.P));
  return res;
}

static inline float get_water_resistance(const Point2 &modifiers, float water_level)
{
  float res = min(water_level * modifiers.x, 1.0f);
  return (1.0f - sqr(1.0f - res)) * modifiers.y;
}

void HumanPhys::updateStandStateProgress(float dt)
{
  HUStandState curState = currentState.getStandState();
  if (curState != currentState.collisionLinksStateTo)
  {
    currentState.collisionLinkProgress = 0.f;
    currentState.collisionLinksStateFrom = currentState.collisionLinksStateTo;
    currentState.collisionLinksStateTo = curState;
  }
  if (currentState.collisionLinkProgress >= 0.f)
    currentState.collisionLinkProgress += dt * stateChangeCollAnimSpeed;
  if (currentState.collisionLinkProgress > 1.f)
  {
    currentState.collisionLinksStateFrom = currentState.collisionLinksStateTo;
    currentState.collisionLinkProgress = -1.f;
  }
}

Point3 HumanPhys::calcCollCenter() const
{
  return currentState.collisionLinkProgress >= 0.f
           ? lerp(collisionCenterPos[currentState.collisionLinksStateFrom], collisionCenterPos[currentState.collisionLinksStateTo],
               currentState.collisionLinkProgress)
           : collisionCenterPos[currentState.collisionLinksStateTo];
}

Point3 HumanPhys::calcCcdPos() const
{
  return currentState.collisionLinkProgress >= 0.f ? lerp(ccdPos[currentState.collisionLinksStateFrom],
                                                       ccdPos[currentState.collisionLinksStateTo], currentState.collisionLinkProgress)
                                                   : ccdPos[currentState.collisionLinksStateTo];
}

void HumanPhys::updateWaterWavesCompensation(float water_height)
{
  // Speed is calculated from distance to surface.
  // If we're near the surface - try to follow the surface ideally
  // Then power decreases, so when we're really far away from the surface
  // we'll not be affected by waves that much (but still can be affected based
  // on the slope of the function)

  const float waveFollowingPosition = water_height - currentState.distFromWaterSurface;
  constexpr float distPowerMult = 2.f;
  constexpr float distEps = 0.1f;
  const float power = min(1.f, safeinv(max(currentState.distFromWaterSurface + distEps, 0.f)) * distPowerMult);
  const float deltaPos = waveFollowingPosition + currentState.posOffset.y - currentState.location.P.y;
  currentState.location.P.y += deltaPos * power;
}

float HumanPhys::updateSwimmingState()
{
  // Simplify swimming state and split into two cases:
  // * We're already swimming
  //   * Check that there's enough clearance for us to stand on (distance from top of the water to the top of the collision) -> switch
  //   to not swimming OR
  //   * Check that we're too high above water surface (different threshold from the previous case) -> switch to not swimming
  // * We're not swimming
  //   * Check that there's enough water for us to swim in at our current position (we're deep enough in the water) -> switch to
  //   swimming
  //
  // In both cases we must check for water height
  // When we swim we need to check for collision height
  // We also must use hysteresis for state switch, otherwise we'll see oscillation behaviour

  Point3 from = Point3::xyz(currentState.location.P);
  Point3 wtPos = from - currentState.posOffset;

  float waterHeight = 0.f;
  // If there's no water - reset swimming state and exit
  if (currentState.isDowned || currentState.attachedToLadder || currentState.pulledToLadder || currentState.isClimbing ||
      currentState.isAttached || !canSwim || !dacoll::traceht_water(wtPos, waterHeight))
  {
    currentState.isSwimming = false;
    currentState.isUnderwater = false;
    setStateFlag(HumanPhysState::ST_SWIM, false);
    return 0.f;
  }

  const float waterLevel = max(0.f, waterHeight - wtPos.y);

  if (currentState.isSwimming) // we were swimming in the previous tick
  {
    float waterClearance = waterLevel; // It's at least waterLevel

    // Check for collision height if we're swimming
    float t = waterSwimmingLevel;

    int matId = -1;
    Point3 norm = {};

    if (!currentState.isUnderwater &&
        dacoll::tracedown_normalized(from, t, &matId, &norm, CLIMB_CAST_FLAGS, nullptr, rayMatId, getTraceHandle()))
    {
      waterClearance = max(0.f, waterHeight - (from.y - t));
    }
    constexpr float clearanceBiasMult = 2.f;
    currentState.isSwimming = waterClearance > waterSwimmingLevel - swimmingLevelBias * clearanceBiasMult &&
                              waterLevel > waterSwimmingLevel - swimmingLevelBias;
  }
  else
    currentState.isSwimming = waterLevel > waterSwimmingLevel;

  currentState.isUnderwater = waterLevel > waterSwimmingLevel + swimmingLevelBias;

  setStateFlag(HumanPhysState::ST_SWIM, currentState.isSwimming);
  return waterLevel;
}

HumanPhys::WalkQueryResults HumanPhys::queryWalkPosition(const Point3 &pos, float from_ht, float down_ht, bool ignore_slide,
  bool is_crawl, float walk_rad, const Point3 &coll_norm) const
{
  WalkQueryResults res;
  const float slideThreshold = is_crawl ? crawlSlideAngle : standSlideAngle;
  const Point3 upDir = -currentState.gravDirection;
  Point3 curMaxPos = pos + upDir * from_ht;
  Point3 curMinPos = pos - upDir * down_ht;

  TraceMeshFaces *handle = getTraceHandle();
  int matId = PHYSMAT_DEFAULT;
  FM_SYNC_DEBUG(WALKER_MIN_MAX_POS, P3D(curMinPos), P3D(curMaxPos));

  bool isTraceHit = false;
  float traceHitY = 0.f;

  const bool useTrace = isSimplifiedPhys || (!is_crawl && isSimplifiedQueryWalkPosition);

  if (useTrace)
  {
    float t = length(curMinPos - curMaxPos);
    Point3 outNorm(0.f, 0.f, 0.f);
    if (get_normal_and_mat(curMaxPos, currentState.gravDirection, t, &matId, &outNorm, WALK_TRACE_FLAGS, nullptr, rayMatId, handle))
    {
      G_ASSERT(!check_nan(outNorm));
      Point3 resPoint = curMaxPos + currentState.gravDirection * t;
      Point3 norm = normalize(clip_dir(outNorm, coll_norm));
      if (dot(norm, upDir) < dot(outNorm, upDir))
        norm = outNorm; // prefer more 'walkable' normal

      isTraceHit = true;
      traceHitY = resPoint * upDir;

      if (ignore_slide ? upDir * norm >= slideThreshold : true)
      {
        res.haveResults = true;
        res.footstep = curMaxPos + upDir * (upDir * (resPoint - curMaxPos));
        res.walkNormal = norm;
        res.standingVel = Point3(0.f, 0.f, 0.f);
        res.isSliding = !isSimplifiedQueryWalkPosition && upDir * norm < slideThreshold;
        if (!res.isSliding)
          res.stepSize = (resPoint - pos) * upDir;
        res.contactStrength = cvt(resPoint * upDir, curMinPos * upDir, curMaxPos * upDir, 0.f, 2.f);

        const bool belowGround = dot(resPoint, upDir) >= dot(pos, upDir);
        const bool wasInAir = currentState.isInAirHistory & 2;
        res.isInAir = wasInAir && !belowGround;
        res.walkMatId = matId;
      }
    }
  }

  if (!useTrace || currentState.fallbackToSphereCastTimer > 0.f)
  {
    dacoll::ShapeQueryOutput shapeQuery;
    if (dacoll::sphere_cast_ex(curMaxPos, curMinPos, walk_rad, shapeQuery, rayMatId, make_span_const(&torsoCollision, 1), handle))
    {
      if (isSimplifiedQueryWalkPosition &&
          (!isTraceHit || rabs(shapeQuery.res * upDir - traceHitY) > FALLBACK_TO_SPHERE_TRACE_THRESHOLD))
        res.shouldFallbackToSphereCast = true;

      Point3 norm = normalize(clip_dir(shapeQuery.norm, coll_norm));
      if (dot(norm, upDir) < dot(shapeQuery.norm, upDir))
        norm = shapeQuery.norm; // prefer more 'walkable' normal
      if (ignore_slide ? upDir * norm >= slideThreshold : true)
      {
        res.haveResults = true;
        res.footstep = curMaxPos + upDir * (upDir * (shapeQuery.res - curMaxPos));
        res.walkNormal = norm;
        res.standingVel = shapeQuery.vel;
        float t = from_ht + down_ht;
        if (dacoll::traceray_normalized(curMaxPos, currentState.gravDirection, t, &matId, &norm, WALK_TRACE_FLAGS, nullptr, rayMatId,
              handle))
        {
          norm = normalize(clip_dir(norm, coll_norm));
          if (dot(upDir, norm) > dot(upDir, res.walkNormal))
            res.walkNormal = norm;
          if (res.walkMatId == PHYSMAT_DEFAULT) // if nothing were hit previously
            res.walkMatId = matId;
        }
        if (!is_crawl && !isSimplifiedPhys)
        {
          dacoll::ShapeQueryOutput walkShapeQuery;
          if (dacoll::sphere_cast_ex(curMaxPos, curMinPos, walk_rad * 2.f, walkShapeQuery, rayMatId,
                make_span_const(&torsoCollision, 1), handle))
          {
            if (dot(upDir, walkShapeQuery.norm) > dot(upDir, res.walkNormal))
              res.walkNormal = walkShapeQuery.norm;
          }
        }
        res.isSliding = !isSimplifiedQueryWalkPosition && upDir * res.walkNormal < slideThreshold;
        if (!res.isSliding)
          res.stepSize = (shapeQuery.res - pos) * upDir;
        res.contactStrength = cvt(shapeQuery.res * upDir, curMinPos * upDir, curMaxPos * upDir, 0.f, 2.f);

        const bool belowGround = dot(upDir, shapeQuery.res) >= dot(upDir, pos);
        const bool wasInAir = currentState.isInAirHistory & 2;
        res.isInAir = wasInAir && !belowGround;
      }
    }
  }

  FM_SYNC_DEBUG(WALKER_WALK_PARAMS, P3D(res.walkNormal), res.stepSize, res.contactStrength);
  return res;
}

BBox3 HumanPhys::calcBBox(HUStandState cur_state) const
{
  dacoll::tmp_collisions_t objects;
  Point2 oriParam = calc_ori_param(currentState, appliedCT);
  dacoll::generate_collisions(TMatrix::IDENT, oriParam, collisionLinks[cur_state], objects);
  BBox3 res;
  for (auto &obj : objects)
    if (obj.haveCollision)
    {
      res += obj.tm * Point3(0.f, obj.scale.y * 0.5f, 0.f);
      res += obj.tm * Point3(0.f, -obj.scale.y * 0.5f, 0.f);
    }
  res += calcCollCenter();
  res.inflate(collRad);
  return res;
}

BBox3 HumanPhys::calcBBox() const
{
  HUStandState curState = currentState.getStandState();
  return calcBBox(curState);
}

void HumanPhys::updateHeadDir(float dt)
{
  currentState.headDir = normalize(approach(currentState.headDir, appliedCT.getWishLookDir(), dt, 0.05f));
}

void HumanPhys::updateAimPos(float dt, bool is_aiming)
{
  currentState.setAimPos(move_to(currentState.getAimPos(), is_aiming ? 1.f : 0.f, dt, aimSpeed * currentState.aimSpeedMult));
}


HumanPhys::CrawlQueryResults HumanPhys::queryCrawling(const TMatrix &tm, const Point3 &cur_coll_center, float waterLevel,
  const Point3 &walk_norm, const Point3 &coll_norm, bool was_crawling, bool is_in_air, bool is_sliding, float dt)
{
  CrawlQueryResults res;
  res.isInAir = is_in_air;
  res.isSliding = is_sliding;
  res.walkNormal = walk_norm;

  const Point3 upDir = -currentState.gravDirection;
  const Point3 forw = tm.getcol(0);
  const Point3 vert = upDir * (upDir * forw);
  const Point3 hzDir = normalize(forw - vert) + vert;

  const float handsOffset = crawlArmsOffset;
  const float feetOffset = crawlLegsOffset;

  const float obstacleHt = was_crawling ? maxCrawlObstacleHeight : maxObstacleHeight;

  const Point3 bellyPos = tm * cur_coll_center;
  const Point3 handsPos = bellyPos + handsOffset * hzDir;
  const Point3 feetPos = bellyPos + feetOffset * hzDir;

  WalkQueryResults crawlHandsQuery = queryWalkPosition(handsPos, obstacleHt, obstacleHt, true, true, walkRad, coll_norm);
  WalkQueryResults crawlFeetQuery = queryWalkPosition(feetPos, obstacleHt, obstacleHt, true, true, walkRad, coll_norm);

  res.walkMatId = !crawlHandsQuery.isInAir ? crawlHandsQuery.walkMatId : crawlFeetQuery.walkMatId;

  bool alreadyOnSurface = !res.isInAir;
  res.isInAir &= crawlFeetQuery.isInAir;

  const float slideThreshold = 0.85f;
  const float breakThreshold = 0.77f;
  const float slopeFactor = clamp(walk_norm * upDir, 0.0f, 1.0f);
  const float slopeHeight = (!isSimplifiedPhys ? 1.2f : 1.5f) - slopeFactor;

  bool slideDown = false;
  if (!crawlHandsQuery.haveResults && !crawlFeetQuery.haveResults)
  {
    slideDown |= (slopeFactor < slideThreshold);
    if (dot(handsPos, upDir) < dot(feetPos, upDir))
    {
      crawlHandsQuery.footstep = handsPos - upDir * slopeHeight;
      crawlFeetQuery.footstep = feetPos;
    }
    else
    {
      crawlHandsQuery.footstep = handsPos;
      crawlFeetQuery.footstep = feetPos - upDir * slopeHeight;
    }
  }
  else if (!crawlHandsQuery.haveResults)
  {
    slideDown |= (slopeFactor < slideThreshold);
    crawlHandsQuery.footstep = handsPos - upDir * slopeHeight;
  }
  else if (!crawlFeetQuery.haveResults)
  {
    slideDown |= (slopeFactor < slideThreshold);
    crawlFeetQuery.footstep = feetPos - upDir * slopeHeight;
  }

  // calculate new walknormal
  Point3 fwdPos = crawlHandsQuery.footstep;
  Point3 backPos = crawlFeetQuery.footstep;
  Point3 front = normalize(fwdPos - backPos);
  Point3 left = normalize(front % upDir);
  Point3 up = normalize(left % front);

  // choose standing vel closer to our current standing vel
  if (lengthSq(currentState.standingVelocity) < 1e-2f)
    res.standingVel = 0.5f * (crawlHandsQuery.standingVel + crawlFeetQuery.standingVel);
  else if (lengthSq(crawlHandsQuery.standingVel - currentState.standingVelocity) <
           lengthSq(crawlFeetQuery.standingVel - currentState.standingVelocity))
    res.standingVel = crawlHandsQuery.standingVel;
  else
    res.standingVel = crawlFeetQuery.standingVel;

  res.contactStrength = max(crawlFeetQuery.contactStrength, crawlHandsQuery.contactStrength);

  res.stepSize = (crawlFeetQuery.stepSize + crawlHandsQuery.stepSize) * 0.5f;
  if (alreadyOnSurface)
    res.stepSize = max(res.stepSize, 0.f);

  if ((!crawlHandsQuery.haveResults && !crawlFeetQuery.haveResults) || (crawlHandsQuery.isInAir && crawlFeetQuery.isInAir))
  {
    WalkQueryResults crawlBellyQuery = queryWalkPosition(bellyPos, obstacleHt, obstacleHt, true, true, walkRad, coll_norm);
    res.isSliding = slideDown && !crawlBellyQuery.isInAir;
    if (!crawlBellyQuery.isInAir)
    {
      res.walkMatId = crawlBellyQuery.walkMatId;
    }
    else if (crawlHandsQuery.haveResults || crawlFeetQuery.haveResults)
    {
      res.walkNormal = normalize(approach(res.walkNormal, up, dt, crawlWalkNormTau));
    }
    else if (slopeFactor < breakThreshold)
    {
      WalkQueryResults checkStandQuery = queryWalkPosition(bellyPos, 0.0f, standingHeight, true, false, walkRad, coll_norm);
      if (!checkStandQuery.haveResults || checkStandQuery.footstep * upDir <= bellyPos * upDir - crouchHeight)
      {
        res.walkNormal = normalize(res.walkNormal);
        res.result = false;
      }
    }
    else
    {
      const float alignToGroundTraceHt = 3.0f;
      crawlHandsQuery = queryWalkPosition(handsPos, 0.0f, alignToGroundTraceHt, true, true, walkRad, coll_norm);
      crawlFeetQuery = queryWalkPosition(feetPos, 0.0f, alignToGroundTraceHt, true, true, walkRad, coll_norm);

      if (!crawlHandsQuery.haveResults && !crawlFeetQuery.haveResults)
      {
        if (dot(handsPos, upDir) < dot(feetPos, upDir))
        {
          crawlHandsQuery.footstep = handsPos - upDir * slopeHeight;
          crawlFeetQuery.footstep = feetPos;
        }
        else
        {
          crawlHandsQuery.footstep = handsPos;
          crawlFeetQuery.footstep = feetPos - upDir * slopeHeight;
        }
      }
      else if (!crawlHandsQuery.haveResults)
      {
        crawlHandsQuery.footstep = handsPos - upDir * slopeHeight;
      }
      else if (!crawlFeetQuery.haveResults)
      {
        crawlFeetQuery.footstep = feetPos - upDir * slopeHeight;
      }

      Point3 fwdPos = crawlHandsQuery.footstep;
      Point3 backPos = crawlFeetQuery.footstep;
      Point3 front = normalize(fwdPos - backPos);
      Point3 left = normalize(front % upDir);
      Point3 up = normalize(left % front);

      if (crawlHandsQuery.haveResults || crawlFeetQuery.haveResults)
      {
        res.walkNormal = normalize(approach(res.walkNormal, up, dt, crawlWalkNormTau * 0.5f));
      }
      else
      {
        WalkQueryResults checkStandQuery = queryWalkPosition(bellyPos, 0.0f, standingHeight, true, false, walkRad, coll_norm);
        if (!checkStandQuery.haveResults || checkStandQuery.footstep * upDir <= bellyPos * upDir - crouchHeight)
        {
          res.walkNormal = normalize(res.walkNormal);
          res.result = false;
        }
      }
    }
  }
  else
  {
    res.isInAir = false;
    res.isSliding = slideDown;
    res.walkNormal = normalize(approach(res.walkNormal, up, dt, crawlWalkNormTau));
  }

  const float underWaterCrawlingHt = waterSwimmingLevel * 0.6f;
  if (waterLevel > underWaterCrawlingHt)
    res.result = false;

  return res;
}

CONSOLE_BOOL_VAL("walkerphys", draw_climb_hole_check, false);

ClimbQueryResults HumanPhys::climbQueryImpl(const TMatrix &tm, const Point3 &overhead_pos, const Point3 &cur_ccd_pos,
  const Point3 &climb_from_pos, const Point3 &vert_dir, bool check_norm, int ray_mat_id, TraceMeshFaces *handle) const
{
  ClimbQueryResults res;
  // check climb positions
  Point3 prevClimbPos = tm * overhead_pos; // above head pos
  Point3 horzDir = normalize(tm.getcol(0));
  Point3 worldClimbPos = tm * (Point3::xVz(climbOnPos, overhead_pos.y) + Point3::x0z(cur_ccd_pos));
  int numClimbPositions = currentState.isSwimming ? climbPositions + additionalClimbPositions : climbPositions;
  float additionalShift = currentState.attachedToLadder ? (collRad + length(currentState.ladderTm.getcol(0)) / 2.f) *
                                                            abs(normalize(currentState.ladderTm.getcol(0)) * horzDir)
                                                        : 0.0f;
  for (int i = 0; i < numClimbPositions; ++i)
  {
    Point3 climbPos = worldClimbPos + horzDir * (climbOnRad * i + additionalShift);
    dacoll::ShapeQueryOutput reachQuery;
    if (dacoll::trace_sphere_cast_ex(prevClimbPos, climbPos, climbOnRad, CLIMB_RAYS_FORWARD, reachQuery, ray_mat_id,
          getActor()->getId(), handle, CLIMB_CAST_FLAGS))
    {
      if (!res.canClimb)
        res.wallAhead = true;
      break;
    }
    Point3 minPos = basis_aware_xVz(climbPos, tm.getcol(3), vert_dir);
    dacoll::ShapeQueryOutput shapeQuery;
    Point3 fromPos = climbPos;
    Point3 toPos = minPos;
    if (dacoll::trace_sphere_cast_ex(fromPos, toPos, climbOnRad, CLIMB_RAYS_SIDEWAYS, shapeQuery, ray_mat_id, getActor()->getId(),
          handle, CLIMB_CAST_FLAGS))
    {
      Point3 dir = toPos - fromPos;
      float dist = length(dir);
      dir *= safeinv(dist);
      if (shapeQuery.t > 0.01f && (check_norm ? shapeQuery.norm * vert_dir >= climbSlideAngle : true) &&
          dot(shapeQuery.res, vert_dir) > dot(climb_from_pos, vert_dir)) // not stuck there, have some space down there
      {
        // Check horizontal hole size.
        Point3 horzDir = normalize((shapeQuery.res - climb_from_pos) % vert_dir);
        float ld = 0.0f, ud = 0.0f;
        if (!climbHoleCheck(fromPos, horzDir, climbMinHorzSize, ray_mat_id, handle, &ld, &ud))
        {
          // If hole is too small, don't climb at all.
          res.canClimb = false;
          break;
        }
        // Fixup climb pos so that human capsule would fit.
        if (ld < collRad)
          shapeQuery.res += horzDir * (collRad - ld);
        if (ud < collRad)
          shapeQuery.res -= horzDir * (collRad - ud);

        if (draw_climb_hole_check)
          draw_debug_line_buffered(fromPos - horzDir * ld, fromPos + horzDir * ud, E3DCOLOR_MAKE(255, 0, 255, 255), 600);

        // Check if we have enough vertical space here, cast up
        dacoll::ShapeQueryOutput vertQuery;
        fromPos = basis_aware_xVz(shapeQuery.res, fromPos, vert_dir); // start from cast down hit pos.
        dist = (fromPos - shapeQuery.res) * vert_dir;
        if ((dist < climbOnMinVertSize) &&
            dacoll::trace_sphere_cast_ex(fromPos, fromPos - dir * max(climbOnMinVertSize - dist - climbOnRad, SMALL_DIST), climbOnRad,
              CLIMB_RAYS_SIDEWAYS, vertQuery, ray_mat_id, getActor()->getId(), handle, CLIMB_CAST_FLAGS))
        {
          float vertSize = dist + (vertQuery.res - fromPos) * vert_dir;
          if ((climbThroughMinVertSize > 0.0f) && (vertSize > climbThroughMinVertSize))
          {
            // We don't have enough space to climb on, but we can try to climb through.
            // todo: make gravity-aware
            res = climbThroughQueryImpl(tm, overhead_pos, cur_ccd_pos, climb_from_pos, shapeQuery.res + climbThroughPosOffset,
              ray_mat_id, handle);
          }
          else
          {
            // If hole is too small, don't climb at all.
            res.canClimb = false;
          }
          if (draw_climb_hole_check)
            draw_debug_line_buffered(vertQuery.res, vertQuery.res - Point3(0.0f, vertSize, 0.0f), E3DCOLOR_MAKE(0, 255, 255, 255),
              600);
          break;
        }
        res.climbNorm = shapeQuery.norm;
        res.canClimb = true;
        res.climbToPos = shapeQuery.res;
        res.climbToPosVel = shapeQuery.vel;
        res.climbFromPos = climb_from_pos;
        res.climbOverheadPos = overhead_pos;
      }
    }
    if (currentState.isClimbing)
      break;
    prevClimbPos = climbPos;
  }
  return res;
}

bool HumanPhys::climbHoleCheck(const Point3 &worldPos, const Point3 &axis, float min_size, int ray_mat_id, TraceMeshFaces *handle,
  float *lower_dist, float *upper_dist) const
{
  dacoll::ShapeQueryOutput queryRes;
  if (lower_dist)
    *lower_dist = min_size;
  if (upper_dist)
    *upper_dist = min_size;
  float tmp = max(min_size - climbOnRad, SMALL_DIST);
  // Trace forward.
  if (!dacoll::trace_sphere_cast_ex(worldPos, worldPos + axis * tmp, climbOnRad, CLIMB_RAYS_SIDEWAYS, queryRes, ray_mat_id,
        getActor()->getId(), handle, CLIMB_CAST_FLAGS))
  {
    if (lower_dist)
    {
      // Hole is big enough already, but we need to return lower distance, so make another cast.
      if (dacoll::trace_sphere_cast_ex(worldPos, worldPos - axis * tmp, climbOnRad, CLIMB_RAYS_SIDEWAYS, queryRes, ray_mat_id,
            getActor()->getId(), handle, CLIMB_CAST_FLAGS))
        *lower_dist = queryRes.t * tmp;
    }
    return true;
  }
  float dist1 = queryRes.t * tmp;
  if (upper_dist)
    *upper_dist = dist1;
  if (dacoll::trace_sphere_cast_ex(worldPos, worldPos - axis * tmp, climbOnRad, CLIMB_RAYS_SIDEWAYS, queryRes, ray_mat_id,
        getActor()->getId(), handle, CLIMB_CAST_FLAGS))
  {
    tmp *= queryRes.t;
    if (lower_dist)
      *lower_dist = tmp;
    return dist1 + tmp >= min_size;
  }
  return true;
}

ClimbQueryResults HumanPhys::climbThroughQueryImpl(const TMatrix &tm, const Point3 &overhead_pos, const Point3 &cur_ccd_pos,
  const Point3 &climb_from_pos, const Point3 &orig_climb_to_pos, int ray_mat_id, TraceMeshFaces *handle) const
{
  Point3 startPosWorld = tm * (Point3::xVz(climbOnPos, overhead_pos.y) + Point3::x0z(cur_ccd_pos));
  Point3 dirWorld = tm % Point3(1.0f, 0.0f, 0.0f);

  ClimbQueryResults res;
  // At this point we've already issued climbQueryImpl, so we know there's a hole there, the question is
  // how big it is, let's check ahead.
  dacoll::ShapeQueryOutput queryRes;
  if (dacoll::trace_sphere_cast_ex(startPosWorld,
        startPosWorld + dirWorld * max(climbThroughForwardDist + collRad * 2.0f - climbOnRad, SMALL_DIST), climbOnRad,
        CLIMB_RAYS_SIDEWAYS, queryRes, ray_mat_id, getActor()->getId(), handle, CLIMB_CAST_FLAGS))
    return res;
  // Let's check that we have enough standing space at this point.
  if (!climbHoleCheck(startPosWorld + dirWorld * (climbThroughForwardDist + climbOnRad), Point3(0.0f, 1.0f, 0.0f), climbOnMinVertSize,
        ray_mat_id, handle))
    return res;
  // And another similar check further.
  if (!climbHoleCheck(startPosWorld + dirWorld * (climbThroughForwardDist + collRad * 2.0f - climbOnRad), Point3(0.0f, 1.0f, 0.0f),
        climbOnMinVertSize, ray_mat_id, handle))
    return res;

  res.climbNorm = Point3(0.0f, 1.0f, 0.0f);
  res.canClimb = true;
  res.isClimbThrough = true;
  // It's safe to climb here, climb to mid point.
  res.climbToPos = Point3::xVz(startPosWorld + dirWorld * (climbThroughForwardDist + collRad), orig_climb_to_pos.y);
  res.climbFromPos = climb_from_pos;
  res.climbOverheadPos = overhead_pos;

  return res;
}

bool HumanPhys::canClimbOverObstacle(const ClimbQueryResults &climb_res)
{
  bool isInAir = (currentState.isInAirHistory & 1) && !(currentState.states & HumanPhysState::ST_SWIM);
  if (isInAir || climb_res.isClimbThrough || climbOverMaxHeight < 0.f)
    return false;

  const Point3 maxPosForClimbingOver = Point3(currentState.location.P) + currentState.vertDirection * climbOverMaxHeight;
  const bool isHeightAllowed =
    dot(climb_res.climbToPos, currentState.vertDirection) <= dot(maxPosForClimbingOver, currentState.vertDirection);

  if (isHeightAllowed)
  {
    const float heightThreshold = 0.1f;

    Point3 climbToPosTracePos = climb_res.climbToPos + currentState.vertDirection * heightThreshold;
    Point3 climbOverForwardTracePos = climb_res.climbToPos + climb_res.climbDir * climbOverForwardOffset;
    climbOverForwardTracePos += currentState.vertDirection * heightThreshold;

    Point3 traceVec = climbOverForwardTracePos - climbToPosTracePos;
    float traceVecLenSq = lengthSq(traceVec);
    if (traceVecLenSq > sqr(1e-3f))
    {
      float t = sqrt(traceVecLenSq);
      if (dacoll::traceray_normalized(climbToPosTracePos, traceVec / t, t, nullptr, nullptr, dacoll::ETF_DEFAULT, nullptr, rayMatId,
            getTraceHandle()))
        return false;
    }

    real t = climbOverMinHeightBehindObstacle + heightThreshold;
    Point3 behindObstacleTracePos = climbOverForwardTracePos - currentState.vertDirection * t;
    dacoll::ShapeQueryOutput toClimbQuery;
    return !dacoll::trace_sphere_cast_ex(climbOverForwardTracePos, behindObstacleTracePos, climbOnRad, CLIMB_RAYS_FORWARD,
      toClimbQuery, rayMatId, getActor()->getId(), getTraceHandle(), CLIMB_CAST_FLAGS);
  }
  return false;
}

void HumanPhys::performClimb(const ClimbQueryResults &climb_res, float at_time)
{
  currentState.isClimbing = true;
  currentState.climbThrough = climb_res.isClimbThrough;
  currentState.climbToPos = climb_res.climbToPos;
  currentState.climbToPosVel = climb_res.climbToPosVel;
  currentState.climbStartAtTime = at_time;
  currentState.climbDeltaHt = (climb_res.climbToPos - climb_res.climbFromPos) * currentState.vertDirection;
  currentState.climbDir = climb_res.climbDir;
  currentState.climbFromPos = climb_res.climbFromPos;
  currentState.climbContactPos = climb_res.climbContactPos;
  currentState.climbNorm = climb_res.climbNorm;
  currentState.isClimbingOverObstacle = canClimbOverObstacle(climb_res);
  if (currentState.isClimbingOverObstacle)
  {
    Point3 targetPos = currentState.climbToPos + currentState.climbDir * climbOverForwardOffset;
    currentState.climbPosBehindObstacle = basis_aware_xVz(targetPos, Point3(currentState.location.P), currentState.vertDirection);
  }
  currentState.isFastClimbing =
    !currentState.isClimbingOverObstacle &&
    (dot(currentState.climbToPos, currentState.vertDirection) <=
      dot(Point3(currentState.location.P) + currentState.vertDirection * maxHeightForFastClimbing, currentState.vertDirection));
}

void HumanPhys::tryClimbing(bool was_jumping, bool is_in_air, const Point3 &gun_node_proj, float at_time)
{
  ClimbQueryResults bestRes;
  float bestScore = -FLT_MAX;
  bool haveMinPos = false;
  bool haveMaxPos = false;
  bool onlyOneClimbPos = currentState.attachedToLadder || isSimplifiedPhys;
  const int frontPositions = onlyOneClimbPos ? 1 : additionalClimbPositions;
  const int horzPositions = onlyOneClimbPos ? 1 : additionalClimbPositions;
  int climbIdx = 0;
  int climbIdxMin = 0;
  int climbIdxMax = INT_MAX;
  if (!onlyOneClimbPos && was_jumping)
  {
    // We only want to test the whole grid when we've just jumped or we have only
    // one climb position, otherwise we split the grid in time.
    climbIdxMin = ((int)currentState.climbGridCounter % climbGridFrames) * maxClimbPositionsPerFrame;
    climbIdxMax = climbIdxMin + maxClimbPositionsPerFrame - 1;
    ++currentState.climbGridCounter;
  }
  for (int i = -additionalClimbPositions + 1; i < frontPositions; ++i)
    for (int j = 0; j < horzPositions; ++j)
    {
      for (int sgn = -1; sgn <= 1 * sign(j); sgn += 2)
      {
        bool needMinPos = !haveMinPos && j == 0;
        if (!needMinPos && ((climbIdx < climbIdxMin) || (climbIdx > climbIdxMax)))
        {
          // Only skip if not min pos, that's to increase our chance of climbing
          // asap since min positions are not that expensive.
          ++climbIdx;
          continue;
        }
        ++climbIdx;
        testClimbingIteration(i, sgn * j, needMinPos, onlyOneClimbPos, is_in_air, gun_node_proj, haveMinPos, haveMaxPos, bestRes,
          bestScore);
      }
    }
  if (bestRes.canClimb)
    performClimb(bestRes, at_time);
}


bool HumanPhys::testClimbingIteration(int front_pos, int horz_pos, bool need_min_pos, bool only_one_climb_pos, bool is_in_air,
  const Point3 &gun_node_proj, bool &have_min_pos, bool &have_max_pos, ClimbQueryResults &best_res, float &best_score)
{
  const float heightOffset = currentState.isSwimming ? swimOffsetForClimb : 0.;
  Point3 offsetPos(additionalClimbRad * front_pos, heightOffset, additionalClimbRad * horz_pos);
  ClimbQueryResults climbRes = climbQuery(offsetPos, rayMatId, !currentState.isSwimming, need_min_pos, !have_max_pos, is_in_air,
    currentState.attachedToLadder, getTraceHandle() /*TODO: provide specific tracehandle to climb query for better behaviour*/);
  if (climbRes.canClimb && (need_min_pos || !climbRes.minPos || only_one_climb_pos))
  {
    float dotProduct = normalize(climbRes.climbToPos - gun_node_proj) * appliedCT.getWishLookDir();
    if (dotProduct < climbAngleCos && !isSimplifiedPhys && !currentState.attachedToLadder)
      return false;
    have_min_pos |= climbRes.minPos;
    have_max_pos |= !climbRes.minPos;
    const float vertFactor = 0.1f;
    float score = dotProduct + (climbRes.climbToPos - gun_node_proj) * currentState.vertDirection * vertFactor;

    if (score > best_score)
    {
      best_res = climbRes;
      best_score = score;
      return true;
    }
  }
  return false;
}


void HumanPhys::finishClimbing(const TMatrix &tm, const Point3 &cur_coll_center, const Point3 &vert_dir)
{
  Point3 curClimbPos = tm * cur_coll_center;
  bool overObstacle = dot(currentState.climbToPos, vert_dir) < dot(curClimbPos, vert_dir);
  if (!overObstacle && currentState.climbThrough) // we cannot do it yet
    return;
  if (!overObstacle)
  {
    // check if we have enough clearance here
    float t = standingHeight;
    constexpr float vertOffset = 0.1f;
    if (dacoll::traceray_normalized(currentState.climbToPos + vert_dir * vertOffset, vert_dir, t, nullptr, nullptr,
          dacoll::ETF_DEFAULT, nullptr, rayMatId, getTraceHandle()))
      return;
  }
  Point3 wishPos = basis_aware_xVz(currentState.climbContactPos, currentState.climbToPos, vert_dir);
  if (overObstacle)
    wishPos = currentState.climbToPos;
  Point3 wishDir = normalize(wishPos - tm.getcol(3));
  float curProjection = currentState.velocity * wishDir;
  float obstacleHt = max((currentState.climbToPos - tm.getcol(3)) * vert_dir, 0.f);
  float wishHtSpd = sqrtf(2.f * obstacleHt * currentState.gravMult * gamephys::atmosphere::g());
  float wishSpd = max(wishVertJumpSpeed[ESS_STAND][EMS_STAND] * currentState.jumpSpeedMult, wishHtSpd);
  currentState.velocity += max(0.f, wishSpd - curProjection) * wishDir;
  currentState.isClimbing = false;
  currentState.climbThrough = false;
  currentState.cancelledClimb = true;
}

ClimbQueryResults HumanPhys::climbQuery(const Point3 &offset, int ray_mat_id, bool check_norm, bool check_min, bool check_max,
  bool is_in_air, bool on_ladder, TraceMeshFaces *handle) const
{
  TIME_PROFILE(climbQuery);
  if (!check_min && !check_max)
    return ClimbQueryResults();
  TMatrix tm;
  currentState.location.toTM(tm);
  Point3 curCcdPos = ccdPos[currentState.collisionLinksStateTo];
  // is it possible to climb at all?
  dacoll::ShapeQueryOutput toClimbQuery;

  float jumpClimbHeight = climbOnPos.y;
  float halfHeight = jumpClimbHeight * 0.5f;
  float airClimbHeight = jumpClimbHeight - maxJumpHeight;
  float climbHeight =
    on_ladder ? climbLadderHeight + standingHeight : (!check_max ? halfHeight : (is_in_air ? airClimbHeight : jumpClimbHeight));
  Point3 overHeadPos = Point3::xVz(curCcdPos + offset, climbHeight);
  bool overheadRes = dacoll::trace_sphere_cast_ex(tm * curCcdPos, tm * overHeadPos, climbOnRad, CLIMB_RAYS_INITIAL, toClimbQuery,
    ray_mat_id, getActor()->getId(), handle, CLIMB_CAST_FLAGS);
  Point3 vertDir = on_ladder ? normalize(currentState.ladderTm.getcol(1)) : currentState.vertDirection;

  // if we haven't already minimized pos check it additionally
  bool shouldCheckMinimizedPos = !overheadRes && check_max && check_min && !on_ladder;
  bool minPos = !check_max;
  if (overheadRes && check_max && check_min)
  {
    overHeadPos.y *= 0.5f;
    overheadRes = dacoll::trace_sphere_cast_ex(tm * curCcdPos, tm * overHeadPos, climbOnRad, CLIMB_RAYS_INITIAL, toClimbQuery,
      ray_mat_id, getActor()->getId(), handle, CLIMB_CAST_FLAGS);
    minPos = true;
  }

  Point3 climbFromPos = tm * (curCcdPos + offset);
  ClimbQueryResults res;

  if (!overheadRes)
  {
    res = climbQueryImpl(tm, overHeadPos, curCcdPos + offset, climbFromPos, vertDir, check_norm, ray_mat_id, handle);
    if (!res.canClimb && shouldCheckMinimizedPos)
    {
      minPos = true;
      overHeadPos.y *= 0.5f;
      if (!dacoll::trace_sphere_cast_ex(tm * curCcdPos, tm * overHeadPos, climbOnRad, CLIMB_RAYS_INITIAL, toClimbQuery, ray_mat_id,
            getActor()->getId(), handle, CLIMB_CAST_FLAGS))
        res = climbQueryImpl(tm, overHeadPos, curCcdPos + offset, climbFromPos, vertDir, check_norm, ray_mat_id, handle);
      else
        res.wallAhead = true; // Act as wall is ahead of us.
    }
  }
  else
  {
    res.wallAhead = true; // Act as wall is ahead of us.
  }

  if (!res.canClimb && res.wallAhead && minPos)
  {
    // We can't climb because of a wall ahead at this point, try a bit higher.
    overHeadPos.y = climbHeight * 0.66f;
    if (!dacoll::trace_sphere_cast_ex(tm * curCcdPos, tm * overHeadPos, climbOnRad, CLIMB_RAYS_INITIAL, toClimbQuery, ray_mat_id,
          getActor()->getId(), handle, CLIMB_CAST_FLAGS))
      res = climbQueryImpl(tm, overHeadPos, curCcdPos + offset, climbFromPos, vertDir, check_norm, ray_mat_id, handle);
  }

  if (!res.canClimb && res.wallAhead && minPos)
  {
    // Try one more point.
    overHeadPos.y = climbHeight * 0.83f;
    if (!dacoll::trace_sphere_cast_ex(tm * curCcdPos, tm * overHeadPos, climbOnRad, CLIMB_RAYS_INITIAL, toClimbQuery, ray_mat_id,
          getActor()->getId(), handle, CLIMB_CAST_FLAGS))
      res = climbQueryImpl(tm, overHeadPos, curCcdPos + offset, climbFromPos, vertDir, check_norm, ray_mat_id, handle);
  }

  const Point3 &vdir = vertDir;
  res.minPos = (res.climbToPos - tm.getcol(3)) * vdir < halfHeight;
  res.climbFromPos = tm * curCcdPos; // fixup climb from pos as we only need it to be our initial position
  res.climbContactPos = lerp(res.climbFromPos, tm * res.climbOverheadPos,
    cvt(res.climbToPos * vdir, res.climbFromPos * vdir, (tm * res.climbOverheadPos) * vdir, 0.f, 1.f));
  res.climbDir = normalize(basis_aware_x0z(res.climbToPos - res.climbContactPos, vdir));
  return res;
}

template <typename MatProps>
const MatProps *get_mat_props(const gamephys::CollisionContactData &contact)
{
  return contact.matId < 0 ? nullptr : MatProps::get_props(contact.matId);
};


HumanPhys::TorsoCollisionResults HumanPhys::processTorsoCollision(TMatrix &tm, int num_iter, float speed_coll_hardness, float at_time)
{
  TorsoCollisionResults res;
  res.gunNodeTm = tm;
  res.meanCollisionPos = tm.getcol(3);
  const Point3 upDir = -currentState.gravDirection;

  const float slideThreshold = currentState.isCrawl() ? crawlSlideAngle : standSlideAngle;
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  for (int i = 0; i < num_iter + 1; ++i) // +1 because we have one 'pre-iteration' to determine if we should stand or sit
  {
    Point3 torsoPosOffs = ZERO<Point3>();
    dacoll::tmp_collisions_t objects;
    Point2 oriParam = calc_ori_param(currentState, appliedCT);
    dacoll::generate_collisions(tm, oriParam, collisionLinks[currentState.collisionLinksStateTo], objects);
    if (currentState.collisionLinkProgress >= 0.f)
    {
      dacoll::tmp_collisions_t prevObjects;
      dacoll::generate_collisions(tm, oriParam, collisionLinks[currentState.collisionLinksStateFrom], prevObjects);
      dacoll::lerp_collisions(objects, prevObjects, currentState.collisionLinkProgress);
    }
    for (int k = 0; k < objects.size(); ++k)
    {
      const dacoll::CollisionCapsuleProperties &obj = objects[k];
      if (obj.nameId == gunLinkNameId)
        res.gunNodeTm = obj.tm;
      if (!obj.haveCollision)
        continue;
      currentState.torsoCollisionTmPos = obj.tm.getcol(3);
      contacts.clear();
      dacoll::set_vert_capsule_shape_size(torsoCollision, collRad * obj.scale.x, obj.scale.y);
      dacoll::set_collision_object_tm(torsoCollision, obj.tm);
      dacoll::test_collision_frt(torsoCollision, contacts);
      dacoll::test_collision_lmesh(torsoCollision, obj.tm, 1.f, -1, contacts, getCollisionMatId());
      dacoll::test_collision_ri(torsoCollision, BBox3(Point3(0.f, 0.f, 0.f), 2.f * scale), contacts, true, at_time, getTraceHandle(),
        getCollisionMatId(), false);
      additionalCollisionChecks(torsoCollision, obj.tm, contacts);

      auto get_collForce = [&](const gamephys::CollisionContactData &contact) {
        const physmat::SoftMaterialProps *softProps = get_mat_props<physmat::SoftMaterialProps>(contact);
        return softProps ? softProps->collisionForce : 1.f;
      };
      // test_unit_collision_s(torsoCollision, obj.tm, 5.f, contacts, true, BBox3(Point3(0.f, 0.f, 0.f), 5.f), at_time);
      eastl::sort(contacts.begin(), contacts.end(),
        [&](const gamephys::CollisionContactData &lhs, const gamephys::CollisionContactData &rhs) {
          return get_collForce(rhs) > get_collForce(lhs);
        });
      Point3 meanCollisionPos = Point3(0.f, 0.f, 0.f);
      Point3 meanCollisionNormal = Point3(0.f, 0.f, 0.f);
      int collCount = 0;
      TMatrix ladderItm = inverse(currentState.ladderTm);
      Point3 ladderPos = currentState.ladderTm.getcol(3);
      Point3 ladderForward = currentState.ladderTm.getcol(0);
      for (int j = 0; j < contacts.size(); ++j)
      {
        gamephys::CollisionContactData &contact = contacts[j];
        if (currentState.attachedToLadder)
        {
          bool otherSide = ((tm.getcol(3) - ladderPos) * ladderForward) * ((contact.wposB - ladderPos) * ladderForward) < 0;
          bool isInLadder = BBox3(Point3(), 1) & (ladderItm * contact.wposB);
          if (otherSide || isInLadder)
          {
            remove_contact(contacts, j--);
            continue;
          }
        }

        if (contact.matId >= 0 && !PhysMat::isMaterialsCollide(getCollisionMatId(), contact.matId))
        {
          const physmat::PhysContactProps *contactProps = get_mat_props<physmat::PhysContactProps>(contact);
          if (contactProps && contactProps->removePhysContact)
          {
            remove_contact(contacts, j--);
            continue;
          }
        }
        const physmat::PhysDestructibleProps *destrProps = get_mat_props<physmat::PhysDestructibleProps>(contact);
        constexpr float contactDepthThreshold = 0.05f;
        constexpr float contactDotThreshold = -0.5;
        const float spd = length(currentState.velocity);
        const float velDot = currentState.velocity * contact.wnormB;

        if (contact.objectInfo && spd > VERY_SMALL_NUMBER)
        {
          Point3 velocityNormalized = currentState.velocity / spd;

          const IPhysActor *actor = getActor();
          const int32_t id = actor ? actor->getId() : -1;

          if (destrProps && contact.depth < contactDepthThreshold && velDot < contactDotThreshold * spd)
          {
            bool shouldDestroy = destrProps->climbingThrough && currentState.isClimbing;
            shouldDestroy |= destrProps->impulseSpeed > 0.f && sqr(destrProps->impulseSpeed) < sqr(velDot);
            if (shouldDestroy)
            {
              float destrImpulse = contact.objectInfo->getDestructionImpulse();
              contact.objectInfo->onImpulse(destrImpulse, velocityNormalized, contact.wpos, 0.f, contact.wnormB, /*flags*/ 0, id);
              remove_contact(contacts, j--);
              continue;
            }
          }

          contact.objectInfo->onImpulse(spd, velocityNormalized, contact.wpos, 0.f, contact.wnormB,
            gamephys::CollisionObjectInfo::CIF_NO_DAMAGE, id);
        }
      }

      for (int j = 0; j < contacts.size(); ++j)
      {
        gamephys::CollisionContactData &contact = contacts[j];
        FM_SYNC_DEBUG(WALKER_CONTACT_INFO, P3D(contact.wnormB), P3D(contact.wpos), contact.depth);
        float collisionForce = 1.f;
        const physmat::SoftMaterialProps *softProps = get_mat_props<physmat::SoftMaterialProps>(contact);
        if (currentState.ignoresHumanCollision && contact.matId == getCollisionMatId())
          continue;
        if (contact.matId != PHYSMAT_DEFAULT)
        {
          res.torsoContactMatId = contact.matId;
          bool hasMatId = false;
          for (auto &c : res.torsoContacts)
          {
            if (c.matId == contact.matId)
            {
              if (fabs(c.depth) < fabs(contact.depth))
                c = contact;
              hasMatId = true;
              break;
            }
          }
          if (!hasMatId && res.torsoContacts.size() < res.torsoContacts.capacity())
            res.torsoContacts.push_back(contact);
          if (contact.riPool >= 0)
            res.rendinstPool = contact.riPool;
        }
        if (softProps)
        {
          collisionForce = softProps->collisionForce;
          if (softProps->physViscosity != 1.f)
          {
            res.frictionViscosityMult = softProps->physViscosity;
            continue;
          }
        }
        meanCollisionPos += contact.wpos;
        meanCollisionNormal += contact.wnormB;
        collCount++;
        {
          float angleWithGravity = contact.wnormB * -currentState.gravDirection;
          if (angleWithGravity > slideThreshold)
          {
            res.isInAir = false;
            res.isSliding = false;

            if (!currentState.isCrawl())
              currentState.fallbackToSphereCastTimer = FALLBACK_TO_SPHERE_CAST_TICKS * timeStep;

            if (contact.wnormB.y > currentState.walkNormal.y && (!currentState.isCrawl() || currentState.isInAirHistory & 2))
              currentState.walkNormal = contact.wnormB;
          }
          float speedCollHardness = i == 0 ? collisionForce * speed_coll_hardness : 0.f;
          if (currentState.isClimbing)
          {
            // Almost Identical to the code in ::updateClimbing, but updateClimbing uses curClimbPos
            Point3 curClimbPos = tm * Point3(0.f, maxObstacleHeight, 0.f);
            bool overObstacle =
              dot(currentState.climbToPos, currentState.vertDirection) < dot(curClimbPos, currentState.vertDirection);
            if (!overObstacle)
              contact.wnormB = normalize(Point3::x0z(contact.wnormB)); // when climbing, reduce normal to horizontal offsets only
          }
          bool canStepOver =
            maxStepOverHeight > 0.f && dot(contact.wpos - tm.getcol(3), currentState.vertDirection) < maxStepOverHeight;
          if (canStepOver)
            speedCollHardness = 0.f;
          applyPushingImpulse(contact, torsoPosOffs, 1.f, speedCollHardness);
        }
      }
      if (collCount > 0)
      {
        res.haveContact = true;
        res.meanCollisionPos = meanCollisionPos * safeinv(float(collCount));
        res.meanCollisionNormal = normalize(meanCollisionNormal);
      }
    }
    // We need to crouch more
    if (i > 0)
    {
      if (torsoPosOffs.lengthSq() > 0.f && !currentState.disableCollision)
      {
        applyOffset(torsoPosOffs);
        currentState.location.toTM(tm);
      }
    }
    else if (torsoPosOffs * upDir < 0.f && currentState.height > 0.f)
    {
      float dh = standingHeight - crouchHeight;
      currentState.height = saturate(currentState.height + safeinv(dh) * (torsoPosOffs * upDir));
    }
    if (torsoPosOffs.lengthSq() == 0.f) // early exit if all resulted in no movement
      return res;
  }
  return res;
}

CONSOLE_BOOL_VAL("walkerphys", disable_walk_query, false);
CONSOLE_BOOL_VAL("walkerphys", draw_climb_update, false);

void HumanPhys::updateClimbing(float at_time, float dt, const TMatrix &tm, const Point3 &vert_dir, const Point3 &cur_coll_center,
  bool torso_collided, const Point3 &torso_coll_norm)
{
  currentState.climbToPos += currentState.climbToPosVel * dt;
  currentState.climbFromPos += currentState.climbToPosVel * dt;
  currentState.climbContactPos += currentState.climbToPosVel * dt;

  if (draw_climb_update)
  {
    draw_debug_sphere_buffered(currentState.climbFromPos, 0.5f, E3DCOLOR_MAKE(0, 255, 0, 255));
    draw_debug_sphere_buffered(currentState.climbContactPos, 0.5f, E3DCOLOR_MAKE(0, 255, 0, 255));
    draw_debug_sphere_buffered(currentState.climbToPos, 0.5f, E3DCOLOR_MAKE(0, 0, 255, 255));
    draw_debug_sphere_buffered(currentState.climbPosBehindObstacle, 0.5f, E3DCOLOR_MAKE(0, 0, 255, 255));
  }

  Point3 curClimbPos = tm * cur_coll_center;
  // in case climbing over obstacle, we always want to push forward
  // therefore we always higher than the obstacle
  bool overObstacle = dot(currentState.climbToPos, vert_dir) < dot(curClimbPos, vert_dir);
  bool shouldPushForward = overObstacle;
  bool shouldRiseUp = !overObstacle;
  if (!shouldRiseUp && dot(tm.getcol(3), vert_dir) < dot(currentState.climbToPos, vert_dir))
  {
    Point3 dir = basis_aware_x0z(currentState.climbToPos - curClimbPos, vert_dir);
    float dist = length(dir);
    float predictDist = dist;
    dir *= safeinv(dist);
    predictDist += climbOnRad;
    dacoll::ShapeQueryOutput sphereQuery;
    if (dist != 0.f && dacoll::traceray_normalized(curClimbPos, dir, predictDist, nullptr, nullptr, dacoll::ETF_DEFAULT, nullptr,
                         rayMatId, getTraceHandle()))
      shouldRiseUp = true; // continue to rise
  }

  if (shouldPushForward)
  {
    if (currentState.isClimbingOverObstacle)
    {
      if (length(currentState.climbPosBehindObstacle - currentState.climbFromPos) <= length(curClimbPos - currentState.climbFromPos))
      {
        currentState.velocity = ZERO<Point3>();
        currentState.isClimbing = false;
      }
      else
        currentState.velocity = currentState.climbDir * climbOverStaticVelocity;
    }
    else
    {
      Quat gravOrient = calc_projected_orientation(appliedCT.getWishShootDir(), currentState.vertDirection);
      Point3 walkDir = normalize(gravOrient * Point3::x0y(appliedCT.getWalkDir() * appliedCT.getWalkSpeed()));
      float maxAccel = cvt(walkDir * currentState.climbDir, 0.f, 1.f, 1.f, 3.f);
      float maxClimbHorzMult = cvt(currentState.climbDeltaHt / maxObstacleHeight, 1.f, 4.f, maxAccel, 1.f);
      currentState.velocity += currentState.climbDir * dt * climbHorzAccel * sqr(maxClimbHorzMult);
      Point3 curHorzVel = basis_aware_x0z(currentState.velocity, vert_dir);
      float curHorzVelLen = curHorzVel.length();
      float wishMaxClimbHorzVel = maxClimbHorzMult * maxClimbHorzVel;
      if (curHorzVelLen > wishMaxClimbHorzVel)
      {
        float mult = wishMaxClimbHorzVel / curHorzVelLen;
        currentState.velocity -= curHorzVel * (1 - mult);
      }
      Point3 targetDir = basis_aware_x0z(currentState.climbToPos - tm.getcol(3), vert_dir);
      const Point3 leftClimbDir = Quat(vert_dir, PI * 0.5) * currentState.climbDir;
      float leftProj = leftClimbDir * targetDir;
      currentState.velocity += leftClimbDir * leftProj * dt * climbHorzAccel * sqr(maxClimbHorzMult);
      float dirProj = targetDir * currentState.climbDir;
      bool isTorsoBlocked = torso_collided && torso_coll_norm * currentState.climbDir < -0.5f && !shouldRiseUp;
      if (dirProj < climbDetachProjLen || (dirProj < 0.f && isTorsoBlocked))
      {
        currentState.isClimbing = false;
        currentState.climbThrough = false;
      }
    }
  }
  else
  {
    Point3 climbPos = currentState.climbContactPos;
    Point3 wishHorzVel = basis_aware_x0z(climbPos - curClimbPos, vert_dir);
    Point3 wishVel = basis_aware_xVz(wishHorzVel, currentState.velocity, vert_dir);
    if (torso_collided)
      wishVel = wishVel - wishVel * torso_coll_norm * torso_coll_norm;
    // do a 'snap' to wish position, instead of moving with acceleration
    currentState.location.P =
      move_to_vec(Point3(currentState.location.P), basis_aware_xVz(climbPos, currentState.location.P, vert_dir), dt, maxClimbHorzVel);
    float accel = (wishHorzVel * currentState.climbDir < 0.f) ? climbHorzBrake : climbHorzAccel;
    currentState.velocity = move_to_vec(currentState.velocity, wishVel, dt, accel);
  }

  const float climbingSpeedMult = currentState.climbingSpeedMult * (currentState.isFastClimbing ? fastClimbingMult : 1.f);
  const float velY = -(currentState.velocity * currentState.gravDirection);
  const float gravVertSpd = velY - currentState.gravMult * gamephys::atmosphere::g() * dt; // brake by gravity from
                                                                                           // your initial vert speed
  const float maxVertSpd = maxClimbSpeed * climbingSpeedMult;
  if (shouldRiseUp)
  {
    const float vertAccel = (currentState.velocity * vert_dir > 0.f ? climbVertAccel : climbVertBrake);
    const float vertSpd = currentState.velocity * vert_dir + dt * vertAccel * climbingSpeedMult;
    float clampedY = clamp(vertSpd, gravVertSpd, maxVertSpd);
    currentState.velocity = basis_aware_xVz(currentState.velocity, vert_dir * clampedY, vert_dir);
  }
  else
  {
    const float springyAfterClimbCoef = 1.35f; // must be > 1.3f for efficient climbing, < 1.4f to avoid fly-from-slope effect
                                               // (defaults)
    float clampedY = clamp(gravVertSpd, 0.f, maxVertSpd * springyAfterClimbCoef);
    currentState.velocity = basis_aware_xVz(currentState.velocity, vert_dir * clampedY, vert_dir);
  }

  float maxClimbDistance = currentState.isClimbingOverObstacle ? climbOverDistance : climbDistance;
  if (at_time > currentState.climbStartAtTime + climbTimeout ||
      basis_aware_x0z(currentState.climbContactPos - curClimbPos, vert_dir).lengthSq() > sqr(maxClimbDistance))
  {
    currentState.isClimbing = false;
    currentState.climbThrough = false;
  }
  currentState.isInAirHistory = -1;

  if (currentState.velocity.length() > climbMaxVelLength)
    currentState.velocity = normalize(currentState.velocity) * climbMaxVelLength;

  currentState.location.P += DPoint3(currentState.climbToPosVel * dt);
}

bool HumanPhys::checkWallClimbing(const TMatrix &tm)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  dacoll::set_collision_object_tm(climberCollision, tm);
  dacoll::test_collision_world(climberCollision, contacts, getCollisionMatId(), dacoll::EPL_DEFAULT,
    dacoll::EPL_ALL & ~(dacoll::EPL_CHARACTER | dacoll::EPL_DEBRIS));
  dacoll::test_collision_frt(climberCollision, contacts);
  dacoll::test_collision_lmesh(climberCollision, tm, 1.f, -1, contacts, getCollisionMatId());
  additionalCollisionChecks(climberCollision, tm, contacts);

  return !contacts.empty();
}

void HumanPhys::resetClimbing()
{
  currentState.isClimbing = false;
  currentState.isClimbingOverObstacle = false;
  currentState.isFastClimbing = false;
  currentState.climbThrough = 0;
  currentState.climbToPos.zero();
  currentState.climbToPosVel.zero();
  currentState.climbStartAtTime = -1.f;
  currentState.climbDeltaHt = 0.f;
  currentState.climbDir.zero();
  currentState.climbFromPos.zero();
  currentState.climbContactPos.zero();
  currentState.climbNorm = Point3(0, 1, 0);
}

void HumanPhys::updateHeight(const TMatrix &tm, float dt, float wishHeight, bool &isCrawling, bool &isCrouching)
{
  G_ASSERTF(wishHeight >= -1.f && wishHeight <= 1.f, "%f", wishHeight);
  if (currentState.height != wishHeight || fabsf(currentState.heightCurVel) > 1e-6)
  {
    bool forbidStateChange = false;
    if (wishHeight != currentState.height && !currentState.attachedToExternalGun && !currentState.attachedToLadder)
    {
      // do a standup collision check
      dacoll::tmp_collisions_t objects;
      HUStandState wishState = isCrawling ? ESS_CRAWL : isCrouching ? ESS_CROUCH : ESS_STAND;
      if (wishHeight > currentState.height)
      {
        dacoll::generate_collisions(tm, Point2(0.f, 0.f), collisionLinks[wishState], objects);
        Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
        for (int i = 0; i < objects.size() && !forbidStateChange; ++i)
        {
          const dacoll::CollisionCapsuleProperties &obj = objects[i];
          if (!obj.haveCollision)
            continue;
          contacts.clear();
          dacoll::set_vert_capsule_shape_size(torsoCollision, collRad * obj.scale.x, obj.scale.y);
          dacoll::set_collision_object_tm(torsoCollision, obj.tm);
          dacoll::test_collision_world(torsoCollision, contacts, getCollisionMatId(), dacoll::EPL_DEFAULT,
            dacoll::EPL_ALL & ~(dacoll::EPL_CHARACTER | dacoll::EPL_DEBRIS));
          dacoll::test_collision_frt(torsoCollision, contacts);
          dacoll::test_collision_lmesh(torsoCollision, obj.tm, 1.f, -1, contacts, getCollisionMatId());
          additionalCollisionChecks(torsoCollision, obj.tm, contacts);
          for (const gamephys::CollisionContactData &contact : contacts)
          {
            const physmat::SoftMaterialProps *softProps = get_mat_props<physmat::SoftMaterialProps>(contact);
            if (softProps && softProps->physViscosity > 0.f)
              continue;
            // if normal is not looking down, we don't forbit a state change. we'll solve state change in torso collision
            if (contact.wnormB.y > -0.5f)
              continue;
            forbidStateChange = true;
            break;
          }
        }
        if (!forbidStateChange && additionalHeightCheck)
        {
          float t = lerp(crouchHeight, standingHeight, saturate(currentState.height)) - collRad;
          Point3 norm;
          if (dacoll::traceray_normalized(tm.getcol(3), Point3(0, 1, 0), t, nullptr, &norm, dacoll::ETF_DEFAULT, nullptr, rayMatId,
                getTraceHandle()))
            forbidStateChange = norm.y < -0.5;
        }
      }
      if (forbidStateChange)
      {
        wishHeight = currentState.height, currentState.heightCurVel = 0;
        isCrawling = wishHeight < -0.5f;
        isCrouching = wishHeight >= -0.5f && wishHeight < 0.5f;
      }
    }

    static const float FIXED_DT = 1.0 / 1024.0;
    for (float fdt = dt; fdt > 0; fdt -= FIXED_DT) //-V1034
    {
      float changePosMult = currentState.fasterChangePoseMult;
      float sdt = fdt > FIXED_DT ? FIXED_DT : fdt;
      float deltaHeight = (wishHeight - currentState.height);
      float deltaHeightClamped = clamp(deltaHeight, heightChangeDeltaClampMin, heightChangeDeltaClampMax);
      float new_vel = currentState.heightCurVel * (1 - clamp(heightChangeDamp * changePosMult * sdt, 0.f, 1.f)) +
                      (deltaHeightClamped * heightChangeSprK * sqr(changePosMult)) * sdt;
      float dht = (new_vel + currentState.heightCurVel) * 0.5f * sdt;
      currentState.setHeight(currentState.height + dht);
      currentState.heightCurVel = new_vel;
      if (fabsf(currentState.heightCurVel) < 1e-4 && fabsf(wishHeight - currentState.height) < 1e-4)
      {
        currentState.setHeight(wishHeight);
        currentState.heightCurVel = 0;
        break;
      }
    }
  }
}

static float calc_state_speed(HUStandState stand_state, HUMoveState move_state,
  const carray<carray<float, EMS_NUM>, ESS_NUM> &walk_speeds, const HumanPhysState &cur_state, float min_spd)
{
  float baseSpeed = walk_speeds[stand_state][move_state];
  float mult = cur_state.moveSpeedMult * (move_state == EMS_SPRINT ? cur_state.sprintSpeedMult : 1.f);
  float resSpeed = baseSpeed * mult;
  if (move_state == EMS_SPRINT && cur_state.sprintLerpSpeedMult != 1.f)
  {
    float wishRunSpeed = max(min_spd, walk_speeds[stand_state][EMS_RUN]) * cur_state.moveSpeedMult;
    resSpeed = lerp(wishRunSpeed, resSpeed, cur_state.sprintLerpSpeedMult);
  }
  return resSpeed;
}

static Quat calc_human_wish_orient(const HumanPhysState &state, const TMatrix &tm, TMatrix &ladder_tm)
{
  if (state.isSwimming && state.isUnderwater)
    return dir_to_quat(state.gunDir);
  if (state.isAttached)
    return calc_projected_orientation(state.gunDir, state.walkNormal);
  if (state.isCrawl())
  {
    Point3 crawlDir = normalize(climb_dir(basis_aware_x0z(state.gunDir, state.vertDirection), state.walkNormal, state.vertDirection));
    return dir_and_up_to_quat(crawlDir, state.vertDirection);
  }
  Quat gunOrient;
  // check for gravitation dir deviation
  if (state.vertDirection.y < VERT_DIR_COS_EPS) // simplified dot(state.vertDirection, (0, 1, 0))
    gunOrient = calc_projected_orientation(state.gunDir, state.vertDirection);
  else
    euler_heading_to_quat(-dir_to_yaw(state.gunDir), gunOrient);
  if (state.attachedToLadder)
  {
    Point3 ladderDir = normalize(basis_aware_x0z(ladder_tm.getcol(3) - tm.getcol(3), ladder_tm.getcol(1)));
    if (ladderDir * ladder_tm.getcol(0) < 0.f)
    {
      ladder_tm.setcol(0, -ladder_tm.getcol(0));
      ladder_tm.setcol(2, -ladder_tm.getcol(2));
    }
    Quat ladderOrient = Quat(ladder_tm);
    return qinterp(gunOrient, ladderOrient, state.ladderAttachProgress);
  }
  return gunOrient;
}

void HumanPhys::updatePhys(float at_time, float dt, bool /*is_for_real*/)
{
  currentState.location.P -= ccdMove; // for initial tm, prevCcdPos, prevWorldCcdPos

  TMatrix tm = currentState.location.makeTM();
  const Point3 prevCcdPos = calcCcdPos();
  const Point3 prevWorldCcdPos = tm * prevCcdPos;

  currentState.location.P += ccdMove; // apply external ccdMove which should be verified
  ccdMove.zero();

  const float atmosphereGravity = currentState.gravMult * gamephys::atmosphere::g();

  // From here we change currentState and in the end validate it with processCcdOffset()
  //

  FM_SYNC_DEBUG(WALKER_HEADER, at_time);
  currentState.spdSummaryDiff = ZERO<Point3>();
  // Stage 1: find what controller wants
  const bool shouldTorsoBeInWorld = currentState.alive && currentState.isVisible;
  if (!shouldTorsoBeInWorld)
  {
    if (!currentState.alive)
      appliedCT.setNeutralCtrl();
    if (isTorsoInWorld)
      dacoll::remove_object_from_world(torsoCollision);
  }
  else if (!isTorsoInWorld)
    dacoll::add_object_to_world(torsoCollision);

  isTorsoInWorld = shouldTorsoBeInWorld;

  validateTraceCache();

  if (!currentState.controllable)
  {
    // save shoot history
    // TODO: remove
    bool shoot = appliedCT.isControlBitSet(HCT_SHOOT);
    appliedCT.setNeutralCtrl();
    if (currentState.canShoot)
      appliedCT.setControlBit(HCT_SHOOT, shoot);
  }

  if (!canMove)
    appliedCT.setWalkDir(Point2(0, 0));

  if (currentState.fallbackToSphereCastTimer > 0.f)
    currentState.fallbackToSphereCastTimer -= dt;
  else
    currentState.fallbackToSphereCastTimer = 0.f;

  int curTick = gamephys::nearestPhysicsTickNumber(at_time + dt, timeStep);
  const int ticksAfterCcd = 8;
  bool fallbackFromSimplePhys = curTick - currentState.lastCcdCollisionTick < ticksAfterCcd;
  if (fallbackFromSimplePhys)
    currentState.fallbackToSphereCastTimer = FALLBACK_TO_SPHERE_CAST_TICKS * timeStep;

  // Jumping
  bool isDowned = currentState.isDowned;
  bool isCrawling = appliedCT.isControlBitSet(HCT_CRAWL) && (!(currentState.isInAirHistory & 2) || currentState.height < 0.f) &&
                    canCrawl && !isDowned && !currentState.attachedToLadder;
  bool isCrouching = appliedCT.isControlBitSet(HCT_CROUCH) && !isCrawling && canCrouch && !isDowned && !currentState.attachedToLadder;
  bool isTimeForDelayedJump = currentState.jumpStartTime > 0.f && at_time > currentState.jumpStartTime + beforeJumpDelay;
  bool isJumping = !isDowned && !currentState.isAttached && !currentState.attachedToLadder &&
                   (appliedCT.isControlBitSet(HCT_JUMP) || isTimeForDelayedJump);
  bool shouldStopCrawling = isJumping && currentState.isCrawl();
  int wasJumpingState = -1; // not inited
  auto wasJumping = [&wasJumpingState, this]() {
    if (wasJumpingState < 0) // init on demand
    {
      const HumanControlState *prevctrl = NULL;
      for (int i = unapprovedCT.size() - 1; i >= 0; --i) // search for prev controls
        if (unapprovedCT[i].producedAtTick < appliedCT.producedAtTick)
        {
          prevctrl = &unapprovedCT[i];
          break;
        }
      wasJumpingState = prevctrl && prevctrl->isControlBitSet(HCT_JUMP);
    }
    return (bool)wasJumpingState;
  };

  if (shouldStopCrawling)
  {
    isJumping = false;
    isCrawling = false;
  }

  setStateFlag(HumanPhysState::ST_DOWNED, isDowned);
  setStateFlag(HumanPhysState::ST_CROUCH, isCrouching);
  setStateFlag(HumanPhysState::ST_CRAWL, isCrawling);

  bool isInAir = true;
  bool isSliding = false;
  float stepSize = 0.f;

  bool isAiming = appliedCT.isControlBitSet(HCT_AIM) && currentState.canAim;
  Point3 startVelocity = currentState.velocity;
  FM_SYNC_DEBUG(WALKER_VEL, P3D(currentState.velocity));
  FM_SYNC_DEBUG(WALKER_POS, P3D(currentState.location.P));

  {
    float wishHeight = isCrawling ? -1.f : isCrouching || isDowned ? 0.f : 1.f;
    if (currentState.isClimbing && currentState.climbThrough)
      wishHeight = 0.f;
    if (DAGOR_UNLIKELY(beforeJumpDelay > 0.f && currentState.jumpStartTime >= 0.f && at_time > currentState.jumpStartTime))
      wishHeight = at_time < currentState.jumpStartTime + beforeJumpDelay * .5f ? beforeJumpCrouchHeight : 1.f;
    if (DAGOR_UNLIKELY(at_time < currentState.afterJumpDampingEndTime))
      wishHeight = min(wishHeight, currentState.afterJumpDampingHeight);

    FM_SYNC_DEBUG(WALKER_WISH_HEIGHT, wishHeight);

    if (DAGOR_LIKELY(hasExternalHeight))
    {
      float currentWishHeight = wishHeight;
      wishHeight = currentState.extHeight;
      currentState.extHeight = currentWishHeight;
    }

    updateHeight(tm, dt, wishHeight, /*inout*/ isCrawling, /*inout*/ isCrouching);
    FM_SYNC_DEBUG(WALKER_WISH_HEIGHT, currentState.height);
  }
  bool startedCrawling = currentState.height < 0.f && isCrawling;

  float waterLevel = updateSwimmingState();
  float waterHeight = float(currentState.location.P.y - currentState.posOffset.y) + waterLevel;
  updateStandStateProgress(dt);

  float frictionViscosityMult = 1.f;
  float contactStrength = 0.f;
  TMatrix gunNodeTm = tm;
  if (hasGuns)
    gunNodeTm = calcGunTm(tm, currentState.gunAngles.y, currentState.leanPosition, currentState.height);
  Point3 curCollCenter = calcCollCenter();
  Point3 curCcdPos = calcCcdPos();
  currentState.meanCollisionNormal.zero();

  // Require percise hmap collision
  // It's required for holes in the ground
  // This might happened after terraforming
  const int prevStep = !currentState.isSwimming ? dacoll::set_hmap_step(1) : 0;
  FINALLY([prevStep]() {
    if (prevStep)
      dacoll::set_hmap_step(prevStep);
  });

  currentState.torsoContactMatId = PHYSMAT_INVALID;
  currentState.torsoContactRendinstPool = -1;

  Point3 gun_node_proj = tm.getcol(3);
  gun_node_proj.y = gunNodeTm.getcol(3).y;

  if (torsoCollision.body && !currentState.disableCollision)
  {
    // Feet walking collision
    const int numIter = isSimplifiedPhys ? 1 : 5;
    TorsoCollisionResults torsoRes = processTorsoCollision(tm, numIter, speedCollisionHardness, at_time);
    currentState.meanCollisionNormal = torsoRes.meanCollisionNormal;
    frictionViscosityMult = torsoRes.frictionViscosityMult;
    currentState.torsoContactMatId = torsoRes.torsoContactMatId;
    currentState.numTorsoContacts = 0;
    for (auto &c : torsoRes.torsoContacts)
      currentState.torsoContacts[currentState.numTorsoContacts++] = {c.wpos, c.matId};
    currentState.torsoContactRendinstPool = torsoRes.rendinstPool;
    isInAir = torsoRes.isInAir;
    isSliding = torsoRes.isSliding;
    if (!hasGuns)
      gunNodeTm = torsoRes.gunNodeTm;
    if (!disable_walk_query)
    {
      if (isCrawling)
      {
        CrawlQueryResults crawlQuery = queryCrawling(tm, curCollCenter, waterLevel, currentState.walkNormal,
          torsoRes.meanCollisionNormal, startedCrawling, torsoRes.isInAir, torsoRes.isSliding, dt);

        currentState.walkNormal = crawlQuery.walkNormal;
        isSliding = crawlQuery.isSliding;
        isInAir = crawlQuery.isInAir;
        contactStrength = crawlQuery.contactStrength;
        stepSize = crawlQuery.stepSize;
        currentState.walkMatId = crawlQuery.walkMatId;
        currentState.standingVelocity = crawlQuery.standingVel;

        if (!crawlQuery.result)
        {
          currentState.height = move_to(currentState.height, 0.f, 2.f * dt, 5.f); // standup
          isCrawling = false;
        }
        if (crawlQuery.result && !startedCrawling)
          updateHeight(tm, dt, -1.f, isCrawling, isCrouching);
      }

      if (!isCrawling)
      {
        Point3 walkQueryPos = tm * curCollCenter;
        const float obstacle_ht = maxObstacleHeight + collRad;
        WalkQueryResults walkQuery =
          queryWalkPosition(walkQueryPos, obstacle_ht, obstacle_ht, false, false, walkRad, torsoRes.meanCollisionNormal);
        if (walkQuery.haveResults)
        {
          // velocity projection on our walk normal, so we'll know what velocity is normal when moving on such a surface
          // we need to use previous walk normal so we'll know what surface we were previously to correctly account to it.
          const Point3 standingWalkNormal = currentState.walkNormal;
          Point3 localVel = currentState.velocity - currentState.standingVelocity;
          Point3 velocityProjection = localVel - localVel * standingWalkNormal * standingWalkNormal;

          currentState.walkNormal = walkQuery.walkNormal;
          isSliding = walkQuery.isSliding;

          // override isInAir flag with better detection of when we can step on something and when we can't
          isInAir = walkQuery.isInAir;

          const float relVertVel = localVel.y - velocityProjection.y;
          const float downReach = walkQuery.stepSize - relVertVel * dt;
          if (forceDownReachDists.x >= 0.f && forceDownReachVel > 0.f)
          {
            if (downReach < -forceDownReachDists.x && downReach >= -forceDownReachDists.y && relVertVel < 0.f &&
                relVertVel > -forceDownReachVel)
            {
              const bool unable = currentState.isDowned;
              const bool jumping = isJumping || wasJumping() || currentState.jumpStartTime >= 0.f;
              const bool acting = currentState.isSwimming || currentState.isClimbing || currentState.attachedToLadder;
              const bool isConsideredInAir = (currentState.isInAirHistory & 15) == 15 && !currentState.isSwimming;
              if (!unable && !jumping && !acting && (!isInAir || !isConsideredInAir))
                currentState.velocity -= standingWalkNormal * (forceDownReachVel + relVertVel);
            }
            else if (downReach < -maxObstacleDownReach)
              isInAir = true;
          }
          else if (downReach < -maxObstacleDownReach)
            isInAir = true;

          contactStrength = walkQuery.contactStrength;
          stepSize = walkQuery.stepSize;
          currentState.walkMatId = walkQuery.walkMatId;
          currentState.standingVelocity = walkQuery.standingVel;
        }
        else
        {
          if (currentState.isSwimming)
            currentState.standingVelocity.zero();
          currentState.walkNormal = torsoRes.meanCollisionNormal;
          contactStrength = 1.f;
          currentState.walkMatId = PHYSMAT_DEFAULT;
        }
        if (walkQuery.isSliding || walkQuery.shouldFallbackToSphereCast)
          currentState.fallbackToSphereCastTimer = FALLBACK_TO_SPHERE_CAST_TICKS * timeStep;
      }
    }
    else
      currentState.standingVelocity.zero();
    if (currentState.isClimbing)
    {
      updateClimbing(at_time, dt, tm, currentState.vertDirection, curCollCenter, torsoRes.haveContact, torsoRes.meanCollisionNormal);
      const bool exitedFromClimbing = !currentState.isClimbing; // was climbing previously
      if (exitedFromClimbing)
      {
        waterLevel = updateSwimmingState();
        waterHeight = float(currentState.location.P.y - currentState.posOffset.y) + waterLevel;
        currentState.distFromWaterSurface = waterLevel;
      }
    }
    bool shouldTryClimbing =
      isJumping && !currentState.cancelledClimb && currentState.stamina > climbStaminaDrain * currentState.staminaClimbDrainMult;
    bool climbAllowed = canClimb && -currentState.velocity.y < climbVertBrakeMaxTime * climbVertBrake && !isDowned;
    if (shouldTryClimbing && !currentState.isClimbing && climbAllowed)
    {
      tryClimbing(wasJumping(), isInAir, gun_node_proj, at_time);
      if (currentState.isClimbing)
        currentState.stamina -= climbStaminaDrain * currentState.staminaClimbDrainMult;
    }
    else if (isJumping && !currentState.cancelledClimb && !wasJumping() && currentState.isClimbing && canFinishClimbing)
      finishClimbing(tm, curCollCenter, currentState.vertDirection);
    if (!isJumping) // flip it off again if we're not pressing jump
      currentState.cancelledClimb = false;

    if ((isInAir || isSliding) && !currentState.isClimbing && !currentState.isSwimming && !currentState.attachedToLadder)
      currentState.velocity += currentState.gravDirection * atmosphereGravity * dt;
    FM_SYNC_DEBUG(WALKER_VEL, P3D(currentState.velocity));
  }

  if ((climberCollision.body && (climber || currentState.overrideWallClimb) && checkWallClimbing(tm)) ||
      currentState.attachedToLadder || currentState.isClimbing)
  {
    isInAir = false;
    isSliding = false;
  }

  currentState.ladderAttachProgress = move_to(currentState.ladderAttachProgress, currentState.attachedToLadder ? 1.f : 0.f, dt, 4.f);

  currentState.isInAirHistory = (currentState.isInAirHistory << 1) | (isInAir ? 1 : 0);
  bool isConsideredInAir = (currentState.isInAirHistory & 15) == 15 && !currentState.isSwimming;
  bool climbJumpNeeded = currentState.climbToPos.y - tm.getcol(3).y > max(climbOnPos.y - maxJumpHeight, 0.f);
  bool initialClimbJump = !isConsideredInAir && currentState.climbStartAtTime == at_time && climbJumpNeeded;
  if (isJumping && !wasJumping() && beforeJumpDelay > 0.f && currentState.jumpStartTime < 0.f)
    currentState.jumpStartTime = at_time;

  bool willJump = ((isJumping && !wasJumping()) || currentState.jumpStartTime >= 0.f) && (!isConsideredInAir || !isInAir) &&
                  currentState.stamina > jumpStaminaDrain && !isSliding && (!currentState.isClimbing || initialClimbJump) &&
                  !currentState.isDowned && !currentState.isSwimming && !currentState.attachedToLadder && canJump &&
                  currentState.alive;

  bool isPastJumpDelay = at_time > currentState.jumpStartTime + beforeJumpDelay;
  if ((currentState.jumpStartTime >= 0.f && !willJump) || isPastJumpDelay)
    currentState.jumpStartTime = -1.;
  willJump &= isPastJumpDelay;

  dacoll::ShapeQueryOutput sprintShapeQuery;
  Quat gunQuat;
  euler_heading_to_quat(-dir_to_yaw(appliedCT.getWishShootDir()), gunQuat);
  currentState.knockBackTimer = max(currentState.knockBackTimer - dt, 0.f);
  currentState.isControllable =
    !willJump && !isConsideredInAir && !isSliding && !currentState.isClimbing && currentState.knockBackTimer <= 0.f;

  if (currentState.isClimbing || isDowned)
    isAiming = false;

  isAiming &= currentState.weapEquipState.curState != EES_DOWN;
  currentState.isWishToMove = appliedCT.isMoving();

  bool isMoving = appliedCT.isMoving();
  bool wasSprinting = currentState.moveState == EMS_SPRINT;
  bool haveEnoughStamina =
    max(0.f, currentState.stamina) > (!wasSprinting ? sprintStartStaminaLevel * currentState.maxStaminaMult : sprintEndStaminaLevel);
  bool canSprintByLerp = currentState.sprintLerpSpeedMult > 0.f;

  bool isWeapAllowsSprint = currentState.weapEquipState.curState != EES_EQUIPING || allowWeaponSwitchOnSprint;
  bool isSprinting = isMoving && !isCrouching && !isCrawling && appliedCT.isControlBitSet(HCT_SPRINT) && haveEnoughStamina &&
                     !isDowned && !currentState.stoppedSprint && !currentState.blockSprint && appliedCT.getWalkDir().x > 0.1f &&
                     isWeapAllowsSprint && !isConsideredInAir && canSprintByLerp && canSprint && !isAiming;
  if (currentState.alive && !isSimplifiedPhys && isSprinting && enableSprintStopCollision)
  {
    const Point3 from = tm.getcol(3) + gunQuat * sprintStopCollisionStart;
    const Point3 to = tm.getcol(3) + gunQuat * sprintStopCollisionEnd;
    isSprinting &= !dacoll::sphere_cast_ex(from, to, sprintStopCollisionRad, sprintShapeQuery, rayMatId,
      make_span_const(&torsoCollision, 1), getTraceHandle());
  }
  // clamp sprint dir by 22.5 degrees as it was previously done by doing a sum
  const float sprintWalkYaw = dir_to_yaw(appliedCT.getWalkDir());
  const float sprintYawClamp = PI * (isSimplifiedQueryWalkPosition ? 0.25f : 0.125f);
  Point2 wishSprintDir = yaw_to_2d_dir(clamp(sprintWalkYaw, -sprintYawClamp, sprintYawClamp));
  Quat gravOrient = calc_projected_orientation(appliedCT.getWishShootDir(), currentState.vertDirection);
  Point2 wishWalkDir = normalize(currentState.alive ? isSprinting ? wishSprintDir : appliedCT.getWalkDir() : Point2(0.f, 0.f));
  Point3 wishDir3 = gravOrient * Point3::x0y(wishWalkDir);
  if (isSprinting && !wasSprinting && wishDir3 * currentState.velocity <= 0.f)
    isSprinting = false;
  setStateFlag(HumanPhysState::ST_SPRINT, isSprinting);

  bool isWalking = currentState.moveState == EMS_WALK && !isSprinting;
  setStateFlag(HumanPhysState::ST_WALK, isWalking);

  updateHeadDir(dt);

  float walkSpeed = appliedCT.getWalkSpeed();
  const bool backPedal = dot(appliedCT.getWishShootDir(), wishDir3) < 0.f; // less compensation if move backwards (climb down ladders)
  Point3 realWishDir = normalize(climb_dir(wishDir3, currentState.walkNormal, currentState.vertDirection, backPedal ? 1.1f : 1.001f));
  if (currentState.isSwimming && !currentState.isClimbing)
  {
    // try to approach to water surface if looking upward
    Point3 shootDir = appliedCT.getWishShootDir();
    Point3 leftDir = normalize(Point3(shootDir.z, 0.f, -shootDir.x));
    wishDir3 = normalize(wishWalkDir.x * shootDir - wishWalkDir.y * leftDir);
    // rotate by jump
    if (isJumping)
    {
      if (isMoving)
        wishDir3 = normalize(wishDir3 + Point3(0.f, 1.f, 0.f));
      else
        wishDir3.set(0.f, 1.f, 0.f);
      walkSpeed = 1.f;
      isMoving = true;
    }
    if (!currentState.isUnderwater && wishDir3.y > swimmingLookAngleSin && !currentState.isClimbing)
    {
      double height = currentState.location.P.y;
      currentState.location.P.y = approach(height, height + waterLevel - waterSwimmingLevel, dt, 0.2f);
    }
    // Vertical water movement compensation
    if (previousState.isSwimming)
      updateWaterWavesCompensation(waterHeight);
    realWishDir = wishDir3;
  }
  if (currentState.alive && isSprinting && wasSprinting)
  {
    // modify dir by movedir
    Point3 rootDir = get_some_normal(currentState.vertDirection);
    Point2 wishShootDir = basis_aware_dir_to_angles(appliedCT.getWishShootDir(), currentState.vertDirection, rootDir);
    Point2 wishSprintDir = basis_aware_dir_to_angles(wishDir3, currentState.vertDirection, rootDir);
    appliedCT.setWishShootDir(basis_aware_angles_to_dir(Point2(wishSprintDir.x, wishShootDir.y), currentState.vertDirection, rootDir));
  }

  if (isJumping && currentState.attachedToLadder)
  {
    currentState.attachedToLadder = false;
    currentState.detachedFromLadder = true;
  }

  float walkDotOrient = wishWalkDir.x;

  if (currentState.detachedFromLadder && float_nonzero(walkSpeed) && walkDotOrient > 0.f)
    currentState.detachedFromLadder = false; // restore our ability to be attached to ladders

  bool reduceToWalk = currentState.reduceToWalk || isAiming || float_nonzero(currentState.leanPosition);
  HUMoveState wishMoveState = EMS_STAND;
  if (isSprinting && !reduceToWalk)
    wishMoveState = EMS_SPRINT;
  else if (!isMoving)
  {
    wishMoveState =
      currentState.moveState == EMS_ROTATE_LEFT || currentState.moveState == EMS_ROTATE_RIGHT ? currentState.moveState : EMS_STAND;
    if (wishMoveState != EMS_STAND)
      currentState.moveState = wishMoveState;
  }
  else
    wishMoveState = EMS_RUN;

  if (wishMoveState == EMS_RUN && (walkDotOrient < 0.64f || reduceToWalk || isDowned))
    wishMoveState = EMS_WALK;

  HUStandState standState = currentState.getStandState();

  float minAllowedSpeed = minWalkSpeed;
  if (wishMoveState > EMS_WALK)
    minAllowedSpeed = max(minWalkSpeed, walkSpeeds[standState][wishMoveState - 1]);

  float deltaEnergy = 0.0f;
  if (!isConsideredInAir)
  {
    float heightGain = max(realWishDir * currentState.vertDirection, 0.f);
    float deltaPotentialWish = atmosphereGravity * heightGain * walkSpeeds[standState][currentState.moveState];
    float runEnergy = maxRunSin * walkSpeeds[standState][EMS_RUN];
    float sprintEnergy = maxSprintSin * walkSpeeds[standState][EMS_SPRINT];
    float energySource = atmosphereGravity * max(runEnergy, sprintEnergy);
    deltaEnergy = energySource - deltaPotentialWish;
  }
  const float maxEnergy = sqr(walkSpeeds[standState][EMS_SPRINT]) * 0.5f;
  currentState.kineticEnergy = clamp(currentState.kineticEnergy + deltaEnergy * dt, 0.f, maxEnergy);
  float maxAllowedSpeed = max(sqrtf(max(currentState.kineticEnergy, 0.f) * 2.f), minAllowedSpeed);
  if (isConsideredInAir && (wishMoveState == EMS_SPRINT || currentState.moveState == EMS_SPRINT))
  {
    wishMoveState = EMS_RUN;
    currentState.moveState = EMS_RUN;
  }


  if (appliedCT.isControlBitSet(HCT_SPRINT) && max(0.f, currentState.stamina) <= sprintEndStaminaLevel)
    currentState.stoppedSprint = true;
  else if (!appliedCT.isControlBitSet(HCT_SPRINT))
    currentState.stoppedSprint = false;

  Point3 velBeforeJump = currentState.velocity;
  bool justJumped = false;
  if (willJump)
  {
    G_ASSERT(currentState.alive);
    float jumpMult = currentState.jumpSpeedMult;
    if (initialClimbJump) // calculate proper amount of jump mult needed here, instead of just applying full-on jump
    {
      float obstacleHt = max(currentState.climbToPos.y - (tm.getcol(3).y + crouchHeight), 0.f);
      float wishVel = sqrtf(2.f * obstacleHt * atmosphereGravity);
      float wishMult = safediv(wishVel, wishVertJumpSpeed[standState][wishMoveState]);
      jumpMult = wishMult;
    }
    float wishVertSpeed = wishVertJumpSpeed[standState][wishMoveState] * jumpMult;
    float wishHorzSpeed = wishHorzJumpSpeed[standState][wishMoveState] * jumpMult;
    if (wishMoveState == EMS_SPRINT)
      wishHorzSpeed *= currentState.sprintSpeedMult;
    const Point3 upDir = -currentState.gravDirection;
    float deltaVertSpeed = max(wishVertSpeed - upDir * (currentState.velocity - currentState.standingVelocity), 0.f);
    deltaVertSpeed *= 1.0f - get_water_resistance(waterJumpingResistance, waterLevel);
    if (deltaVertSpeed)
    {
      currentState.velocity += upDir * deltaVertSpeed;
      currentState.velocity += wishHorzSpeed * wishDir3;
      float staminaDrain = safediv(jumpStaminaDrain * deltaVertSpeed, wishVertSpeed);
      currentState.stamina -= staminaDrain * currentState.staminaDrainMult;
      currentState.isInAirHistory = -1;
      setStateFlag(HumanPhysState::ST_JUMP, true);
      justJumped = true;
    }
  }
  else if (isJumping && !wasJumping() && isInAir && canWallJump && currentState.stamina > jumpStaminaDrain && !isSliding)
  {
    // setup test object
    TMatrix wallJumpTm = TMatrix::IDENT;
    wallJumpTm.setcol(3, tm * wallJumpOffset);
    dacoll::set_collision_object_tm(wallJumpCollision, wallJumpTm);

    // do a test
    Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
    dacoll::test_collision_world(wallJumpCollision, contacts, getCollisionMatId(), dacoll::EPL_DEFAULT,
      dacoll::EPL_ALL & ~(dacoll::EPL_CHARACTER | dacoll::EPL_DEBRIS));
    dacoll::test_collision_frt(wallJumpCollision, contacts);
    dacoll::test_collision_lmesh(wallJumpCollision, wallJumpTm, 1.f, -1, contacts, getCollisionMatId());
    float deepest = 0.f;
    Point3 deepestNorm(0.f, 1.f, 0.f);
    for (int j = 0; j < contacts.size(); ++j)
    {
      gamephys::CollisionContactData &contact = contacts[j];
      if (rabs(contact.wnormB.y) > 0.3f)
        continue;
      if (contact.depth < deepest)
      {
        deepest = contact.depth;
        deepestNorm = contact.wnormB;
      }
    }
    if (deepest < 0.f)
    {
      currentState.velocity += deepestNorm * wallJumpSpd;
      currentState.stamina -= jumpStaminaDrain * currentState.staminaDrainMult;
    }
  }

  TMatrix orthoLadderTm = currentState.ladderTm;
  if (currentState.attachedToLadder || currentState.guidedByLadder)
    orthoLadderTm.orthonormalize();

  Point3 wishVertDirection = currentState.guidedByLadder ? orthoLadderTm.getcol(1) : -currentState.gravDirection;


  if (currentState.vertDirection != wishVertDirection)
  {
    if (dot(currentState.vertDirection, wishVertDirection) < -0.8)
      wishVertDirection = currentState.location.O.getQuat().getLeft(); // Don't flip 180 degreees, flip gradually
    Point3 rotAxisVert = normalize(cross(currentState.vertDirection, wishVertDirection));
    if (rotAxisVert.lengthSq() > .1) // May not be true when vertDirection and wishVertDirection are very close
    {
      // gunAngles, bodyDir etc are all relative to rootDir
      // Although in standard gravity they coincide with their absolute counterparts (Becuase rootDir is 1, 0, 0)

      // Sometimes small changes in vertDir can cause big changes in rootDir
      // So recalc all relative things into their absolute versions, change vertDir and recalc relative things back
      Point3 vertBefore = currentState.vertDirection;
      Point3 rootBefore = get_some_normal(currentState.vertDirection);
      Point3 sideDirBefore = normalize(cross(rootBefore, currentState.vertDirection));
      Point2 gunAnglesBefore = currentState.gunAngles;
      Point3 gunDirBefore = basis_aware_angles_to_dir(gunAnglesBefore, vertBefore, rootBefore);
      Point3 body3DDirBefore = relative_2d_dir_to_absolute_3d_dir(currentState.bodyOrientDir, rootBefore, sideDirBefore);
      Point3 walk3DDirBefore = relative_2d_dir_to_absolute_3d_dir(currentState.walkDir, rootBefore, sideDirBefore);

      if (currentState.vertDirection * wishVertDirection > 0.999)
        currentState.vertDirection = wishVertDirection;
      else
        currentState.vertDirection = normalize(approach(dir_and_up_to_quat(currentState.vertDirection, rotAxisVert),
          dir_and_up_to_quat(wishVertDirection, rotAxisVert), dt, 0.2f)
                                                 .getForward());

      Quat changeInVert = quat_rotation_arc(vertBefore, currentState.vertDirection);
      Point3 newRoot = get_some_normal(currentState.vertDirection);
      Point3 newSide = normalize(cross(newRoot, currentState.vertDirection));

      currentState.gunDir = changeInVert * gunDirBefore;
      currentState.gunAngles = basis_aware_dir_to_angles(currentState.gunDir, currentState.vertDirection, newRoot);
      currentState.bodyOrientDir = absolute_3d_dir_to_relative_2d_dir(body3DDirBefore, newRoot, newSide);
      currentState.walkDir = absolute_3d_dir_to_relative_2d_dir(walk3DDirBefore, newRoot, newSide);

      // if (newRoot * rootBefore < 0.5)
      //   debug("Abrupt root change"); //This happens sometimes, it's ok, it's unavoidable
    }
  }

  bool isOnGround = !isConsideredInAir && !willJump;
  setStateFlag(HumanPhysState::ST_ON_GROUND, isOnGround);
  if (isOnGround)
    setStateFlag(HumanPhysState::ST_JUMP, false);

  float wishSpeed = clamp(walkSpeeds[standState][wishMoveState], minAllowedSpeed, maxAllowedSpeed);
  float frictionSpeedBase = walkSpeeds[standState][EMS_RUN];
  const float speedInAimState = aimingMoveSpeed * currentState.speedInAimStateMult;
  if (reduceToWalk)
  {
    wishSpeed = min(speedInAimState, wishSpeed);
    frictionSpeedBase = speedInAimState;
  }
  if (currentState.isClimbing)
    wishSpeed = 0.f;
  float speedMult = (isCrouching || isCrawling || isDowned) ? currentState.crawlCrouchSpeedMult : currentState.moveSpeedMult;
  wishSpeed *= speedMult;
  frictionSpeedBase *= speedMult;
  if (wishMoveState == EMS_SPRINT) //-V1051 false error. We don't need to check the value of the wishSpeed variable here
  {
    wishSpeed *= currentState.sprintSpeedMult;
    if (currentState.sprintLerpSpeedMult != 1.f)
    {
      float wishRunSpeed = clamp(walkSpeeds[standState][EMS_RUN], minAllowedSpeed, maxAllowedSpeed) * currentState.moveSpeedMult;
      wishSpeed = lerp(wishRunSpeed, wishSpeed, currentState.sprintLerpSpeedMult);
    }
  }
  if (currentState.isSwimming)
  {
    wishSpeed *= currentState.swimmingSpeedMult;
    frictionSpeedBase *= currentState.swimmingSpeedMult;
  }
  else
  {
    float waterResMult = 1.0f - get_water_resistance(waterFrictionResistance, waterLevel);
    wishSpeed *= waterResMult;
    frictionSpeedBase *= waterResMult;
  }
  if (float_nonzero(walkSpeed))
  {
    frictionSpeedBase *= walkSpeed;
    walkSpeed = max(walkSpeed, minWalkControl); // this one should be in that exact order
  }

  currentState.velocity -= currentState.standingVelocity;
  if ((currentState.isControllable || currentState.isSwimming) && !currentState.isAttached)
  {
    // we want to eventually walk on normal so add this to spd summary diff which is responsible for collision damage (fall damage as
    // well)
    currentState.spdSummaryDiff += -(currentState.velocity * currentState.walkNormal) * currentState.walkNormal;
    // Construct walk basis
    float prevVertSpd = currentState.velocity.y;
    float wishResSpd = walkSpeed * wishSpeed;
    Point3 wishVel = realWishDir * wishResSpd;
    if (!currentState.attachedToLadder && !currentState.pulledToLadder)
    {
      const float subDt = dt / accelSubframes;
      if (isInertMovement)
      {
        const Point3 wishDir = normalize(wishVel);
        const float currentSpeedSq = currentState.velocity.lengthSq();
        const float walkSpeed = calc_state_speed(standState, EMS_WALK, walkSpeeds, currentState, minAllowedSpeed);
        const float runSpeed = calc_state_speed(standState, EMS_RUN, walkSpeeds, currentState, minAllowedSpeed);
        const HUMoveState accState = isCrouching                       ? EMS_WALK
                                     : currentSpeedSq > sqr(runSpeed)  ? EMS_SPRINT
                                     : currentSpeedSq > sqr(walkSpeed) ? EMS_RUN
                                                                       : EMS_WALK;
        const float slidingFriction = min(1.f, frictionViscosityMult * currentState.frictionMult);
        const float externalDecceleration = max(0.f, frictionViscosityMult * currentState.frictionMult - 1.f);

        const float stateAcceleration = accelerationByState[accState] * currentState.accelerationMult;
        const float stateDecceleration = stateAcceleration * slidingFriction;

        for (int i = 0; i < accelSubframes; ++i)
        {
          const Point3 horzVel = project_dir(currentState.velocity, currentState.gravDirection);
          const float fricSpd = length(currentState.isSwimming ? currentState.velocity : horzVel);

          const float controlFriction = gamephys::calc_friction_mult(fricSpd, frictionThresSpdMult * frictionSpeedBase,
            fricitonByState[accState] * slidingFriction, subDt);

          const float curForwardSpeed = currentState.velocity * wishDir;
          const Point3 curSideVelocity = currentState.velocity - wishDir * curForwardSpeed;

          const float forwardSpeedAfterFriction = curForwardSpeed < 0.f ? (curForwardSpeed * controlFriction) : curForwardSpeed;
          const float forwardSpeedDiff = wishResSpd - forwardSpeedAfterFriction;
          const float acc = forwardSpeedDiff > 0 ? stateAcceleration : -stateDecceleration;
          const float forwardAcceleration = min(acc * subDt, abs(forwardSpeedDiff));
          const Point3 forwardVelocity = wishDir * (forwardSpeedAfterFriction + forwardAcceleration);
          const Point3 sideVelocity = curSideVelocity * controlFriction;
          currentState.velocity = forwardVelocity + sideVelocity;
          const float speed = length(currentState.velocity);
          const float frictionAcc = speed * externalFrictionPerSpeedMult * externalDecceleration;
          currentState.velocity -= (currentState.velocity * safeinv(speed)) * min(frictionAcc * subDt, speed);
        }
      }
      else
      {
        for (int i = 0; i < accelSubframes; ++i)
        {
          Point3 horzVel = project_dir(currentState.velocity, currentState.gravDirection);
          float fricSpd = length(currentState.isSwimming ? currentState.velocity : horzVel);
          currentState.velocity *= gamephys::calc_friction_mult(fricSpd, frictionThresSpdMult * frictionSpeedBase,
            frictionGroundCoeff * frictionViscosityMult * currentState.frictionMult, subDt);
          currentState.velocity =
            gamephys::accelerate_directional(currentState.velocity, normalize(wishVel), wishResSpd, acceleration * wishResSpd, subDt);
        }
      }
    }
    else if (currentState.attachedToLadder)
    {
      Point3 ladderUp = orthoLadderTm.getcol(1);
      prevVertSpd = ladderUp * currentState.velocity;
      TMatrix ladderItm = inverse(orthoLadderTm);
      Point3 localPos = ladderItm * tm.getcol(3);
      const float ladderOffset = collRad + length(currentState.ladderTm.getcol(0));
      const float ladderStrength = 10.f;
      Point3 localDir = Point3(sign(localPos.x) * ladderOffset, 0.f, 0.f) - localPos;
      Point3 dir = orthoLadderTm % (localDir * ladderStrength);
      float wishVertSpd = sign(walkDotOrient) * sign(walkSpeed) * ladderClimbSpeed;

      currentState.isLadderQuickMovingUp = false;
      currentState.isLadderQuickMovingDown = false;
      if (appliedCT.isControlBitSet(HCT_SPRINT) && !isDowned && isMoving && !isCrouching && !isCrawling && !isAiming)
      {
        // fast climb up stairs
        if (ladderQuickMoveUpSpeedMult >= 0.0f && wishVertSpd > 0.f && haveEnoughStamina)
        {
          wishVertSpd *= ladderQuickMoveUpSpeedMult;
          currentState.stamina -= ladderStaminaDrain * dt * currentState.staminaDrainMult;
          currentState.isLadderQuickMovingUp = true;
        }
        // fast climb down
        else if (ladderQuickMoveDownSpeedMult >= 0.0f && wishVertSpd < 0.f)
        {
          wishVertSpd *= ladderQuickMoveDownSpeedMult;
          currentState.isLadderQuickMovingDown = true;
        }
      }
      currentState.velocity = dir;
      // vertical part
      const float vertSpd = move_to(prevVertSpd, wishVertSpd, dt, stepAccel);
      currentState.velocity = currentState.velocity + (vertSpd - currentState.velocity * ladderUp) * ladderUp;
      const float ladderHalfHeight = length(currentState.ladderTm.getcol(1)) * 0.5f;
      if (localPos.y + ladderCheckClimbHeight >= ladderHalfHeight)
      {
        const bool climbDown = walkDotOrient < 0.f || prevVertSpd < 0.f || wishVertSpd < 0.f;
        if (canClimb && !currentState.cancelledClimb && !currentState.isClimbing && !isDowned && !climbDown && wishVertSpd > 0.f)
          tryClimbing(wasJumping(), false, gun_node_proj, at_time);
        if (!currentState.isClimbing)
        {
          const float overHeight = localPos.y + ladderZeroVelocityHeight - ladderHalfHeight;
          if (overHeight >= 0.f)
          {
            if (overHeight > ladderMaxOverHeight)
              currentState.velocity = currentState.velocity + (-ladderClimbSpeed - currentState.velocity * ladderUp) * ladderUp;
            else if (currentState.velocity * ladderUp > 0.f)
              currentState.velocity = currentState.velocity - currentState.velocity * ladderUp * ladderUp;
          }
        }
      }
    }
    if (!currentState.isSwimming && !currentState.attachedToLadder && !currentState.pulledToLadder)
    {
      Point3 upDir = -currentState.gravDirection;
      currentState.location.P += upDir * stepSize * stepAccel * dt;
      // project vertical velocity to walkplane with some accel
      float curProj = currentState.velocity * currentState.walkNormal;
      currentState.velocity -= upDir * curProj * stepAccel * dt;
    }
    if (currentState.isSwimming && !currentState.isUnderwater)
      currentState.velocity.y = min(currentState.velocity.y, 0.f); // clamp it by horizontal zero.

    Point2 horzVel = Point2::xz(currentState.velocity);
    wishWalkDir = horzVel;
  }
  else // just slide it on surface or ballistically fall (with minor control)
  {
    // allow air control
    const Point3 &upDir = currentState.gravDirection; // we can treat it inversely, that's fine as it's only applies in dot products
    float prevYSpd = currentState.velocity * upDir;
    Point3 horzVel = project_dir(currentState.velocity, currentState.gravDirection);
    currentState.velocity = gamephys::accelerate_directional(horzVel, wishDir3, wishSpeed, airAccelerate * wishSpeed, dt);
    currentState.velocity = currentState.velocity + (prevYSpd - currentState.velocity * upDir) * upDir; // reorienting it in that
                                                                                                        // direction
    if (isSliding)
    {
      currentState.spdSummaryDiff += -(currentState.velocity * currentState.walkNormal) * currentState.walkNormal;
      currentState.velocity =
        clip_dir(currentState.velocity, currentState.walkNormal, saturate(contactStrength)) + Point3(0.f, stepSize * 4.f, 0.f);
    }
  }

  if (wishMoveState != currentState.moveState && wishMoveState != EMS_ROTATE_RIGHT && wishMoveState != EMS_ROTATE_LEFT)
  {
    float horzVel = length(currentState.velocity - currentState.velocity * currentState.vertDirection * currentState.vertDirection);
    // TODO: rework rotate state (it's not real move state)
    HUMoveState curMove =
      currentState.moveState != EMS_ROTATE_LEFT && currentState.moveState != EMS_ROTATE_RIGHT ? currentState.moveState : EMS_STAND;
    if (wishMoveState > curMove)
    {
      float midSpeed = lerp(calc_state_speed(standState, curMove, walkSpeeds, currentState, minAllowedSpeed),
        calc_state_speed(standState, HUMoveState(curMove + 1), walkSpeeds, currentState, minAllowedSpeed), moveStateThreshold);
      if (horzVel >= midSpeed || curMove == EMS_STAND || (canStartSprintWithResistance && wishMoveState == EMS_SPRINT && isSprinting))
        currentState.moveState = HUMoveState(curMove + 1);
    }

    if (wishMoveState < curMove)
    {
      float midSpeed = lerp(calc_state_speed(standState, curMove, walkSpeeds, currentState, minAllowedSpeed),
        calc_state_speed(standState, HUMoveState(curMove - 1), walkSpeeds, currentState, minAllowedSpeed), moveStateThreshold);
      if (horzVel <= midSpeed)
        currentState.moveState = HUMoveState(curMove - 1);
    }
  }

  if (currentState.moveState == EMS_SPRINT && walkSpeed > 1e-5f)
    currentState.stamina -= sprintStaminaDrain * dt * walkSpeed * currentState.staminaDrainMult * currentState.staminaSprintDrainMult;
  else if (!currentState.isClimbing)
    restoreStamina(dt, isInAir && !currentState.isSwimming ? airStaminaRestoreMult : 1.f);
  currentState.stamina = max(0.f, currentState.stamina);

  if (reduceToWalk)
    wishMoveState = min(EMS_WALK, wishMoveState);

  currentState.aimForZoomProgress =
    appliedCT.isControlBitSet(HCT_AIM) ? max(currentState.aimPosition, currentState.aimForZoomProgress) : currentState.aimPosition;

  bool isZooming = appliedCT.isControlBitSet(HCT_ZOOM_VIEW) && currentState.canZoom && !isSprinting;
  currentState.zoomPosition = move_to(currentState.zoomPosition, (isZooming ? 1.f : 0.f) * currentState.aimForZoomProgress, dt,
    zoomSpeed * currentState.aimSpeedMult);

  updateAimPos(dt, isAiming);

  Quat wishOrient = calc_human_wish_orient(currentState, tm, orthoLadderTm);

  // Rotate walk dir
  if (currentState.alive && !currentState.isAttached)
  {
    Point3 horzProjection = project_onto_plane(currentState.velocity, currentState.vertDirection);
    Point3 fwd = wishOrient.getForward();
    Point3 rootDir = get_some_normal(currentState.vertDirection);
    Point3 sideDir = normalize(cross(rootDir, currentState.vertDirection));
    float persCourse = -basis_aware_dir_to_angles(fwd, currentState.vertDirection, rootDir).x;
    if (lengthSq(horzProjection) > 1e-4f || walkSpeed > 1e-4f)
    {
      if (walkSpeed == 0.f)
        wishDir3 = Point3::x0y(currentState.walkDir); // TODO: change to a 3d walkdir
      Point3 absoluteWalkDir = relative_2d_dir_to_absolute_3d_dir(currentState.walkDir, rootDir, sideDir);
      Point2 curWalkAngles = basis_aware_dir_to_angles(absoluteWalkDir, currentState.vertDirection, rootDir);
      float curWalkYaw = -curWalkAngles.x;
      float wishWalkYaw = -basis_aware_dir_to_angles(wishDir3, currentState.vertDirection, rootDir).x;
      wishWalkYaw = renorm_ang(wishWalkYaw, curWalkYaw);
      if (isSprinting && !wasSprinting)
        curWalkYaw = renorm_ang(persCourse, wishWalkYaw);
      if (fabsf(wishWalkYaw - curWalkYaw) > HALFPI)
        curWalkYaw = renorm_ang(curWalkYaw - PI, wishWalkYaw);

      curWalkYaw = approach(curWalkYaw, wishWalkYaw, dt, 0.1f);
      curWalkAngles.x = -curWalkYaw;
      absoluteWalkDir = basis_aware_angles_to_dir(curWalkAngles, currentState.vertDirection, rootDir);
      currentState.walkDir = absolute_3d_dir_to_relative_2d_dir(absoluteWalkDir, rootDir, sideDir);
      currentState.bodyOrientDir = absolute_3d_dir_to_relative_2d_dir(currentState.gunDir, rootDir, sideDir);

      if (isSprinting)
      {
        Point3 sprintWishOrientDir = basis_aware_angles_to_dir(Point2(-curWalkYaw, 0), currentState.vertDirection, rootDir);
        wishOrient = calc_projected_orientation(sprintWishOrientDir, currentState.vertDirection);
      }
    }
    else
    {
      float wishShootYaw = -basis_aware_dir_to_angles(appliedCT.getWishShootDir(), currentState.vertDirection, rootDir).x;
      Point3 absoluteBodyDir = relative_2d_dir_to_absolute_3d_dir(currentState.bodyOrientDir, rootDir, sideDir);
      Point2 curBodyAngles = basis_aware_dir_to_angles(absoluteBodyDir, currentState.vertDirection, rootDir);
      float walkYaw = renorm_ang(-curBodyAngles.x, wishShootYaw);
      float minDiff = rotateAngles[standState].x;
      float maxDiff = rotateAngles[standState].y;
      if (walkYaw < wishShootYaw + minDiff || walkYaw > wishShootYaw + maxDiff)
        currentState.moveState = walkYaw < wishShootYaw ? EMS_ROTATE_LEFT : EMS_ROTATE_RIGHT;
      bool rotating = currentState.moveState == EMS_ROTATE_LEFT || currentState.moveState == EMS_ROTATE_RIGHT;
      if (rotating || alignSpeeds[standState] > 0.f)
      {
        const float rotatingSpeed =
          (rotating ? rotateSpeeds[standState] : alignSpeeds[standState]) * currentState.weaponTurningSpeedMult;
        walkYaw = move_to(walkYaw, wishShootYaw, dt, rotatingSpeed);
        if (walkYaw == wishShootYaw && rotating)
          currentState.moveState = EMS_STAND;
        // recalc orient
        Point3 wishDir = fwd;
        if (currentState.isCrawl())
        {
          Point3 crawlDir = normalize(climb_dir(wishDir, currentState.walkNormal, currentState.vertDirection));
          wishOrient = dir_and_up_to_quat(crawlDir, currentState.vertDirection);
        }
        if (rotating)
        {
          currentState.gunAngles.x = -persCourse;
          currentState.gunSpd.zero();
          currentState.targetGunSpd.zero();
          currentState.gunDir = basis_aware_angles_to_dir(currentState.gunAngles, currentState.vertDirection, rootDir);
        }
      }
      walkYaw = clamp(walkYaw, renorm_ang(persCourse, walkYaw) + minDiff, renorm_ang(persCourse, walkYaw) + maxDiff);
      Point3 newBodyDir3d = basis_aware_angles_to_dir(Point2(-walkYaw, curBodyAngles.y), currentState.vertDirection, rootDir);
      currentState.bodyOrientDir = absolute_3d_dir_to_relative_2d_dir(newBodyDir3d, rootDir, sideDir);
      currentState.walkDir = currentState.bodyOrientDir;
    }
  }
  currentState.velocity += currentState.standingVelocity;

  Point3 prevOrientPos = currentState.location.O.getQuat() * curCollCenter;
  Point3 wishOrientPos = wishOrient * curCollCenter;
  if (isSimplifiedPhys && currentState.collisionLinkProgress >= 0.0f)
    currentState.location.O.setQuat(approach(currentState.location.O.getQuat(), wishOrient, dt, 0.2f));
  else
    currentState.location.O.setQuat(wishOrient);

  Point3 offset = prevOrientPos - wishOrientPos;
  Point3 prevOffset = currentState.posOffset;
  currentState.posOffset = approach(currentState.posOffset, currentState.isSwimming ? swimPosOffset : Point3(0.f, 0.f, 0.f), dt, 0.2f);
  currentState.location.P += offset + (currentState.posOffset - prevOffset);

  // sphere cast to figure out "kindof" CCD
  currentState.location.P += dpoint3(currentState.velocity * dt);
  currentState.location.toTM(tm);

  curCcdPos = calcCcdPos();
  if (!currentState.disableCollision)
  {
    if (!isSimplifiedPhys || fallbackFromSimplePhys || isConsideredInAir || isSliding)
    {
      processTorsoCollision(tm, 1, 0.f, at_time);
      if (justJumped && !isInAir)
      {
        currentState.spdSummaryDiff += -(velBeforeJump * currentState.walkNormal) * currentState.walkNormal;
      }
    }

    Point3 toCcdWorldPos = tm * curCcdPos;
    Point3 totalOffset = toCcdWorldPos - prevWorldCcdPos;
    bool hadCcdCollision = processCcdOffset(tm, toCcdWorldPos, totalOffset, collRad - ccdRad, speedCollisionHardness, true, curCcdPos);
    if (hadCcdCollision)
      currentState.lastCcdCollisionTick = curTick;
  }

  // TODO: maybe move it somehow inside another function to make this code cleaner?
  currentState.distFromWaterSurface = waterHeight - (currentState.location.P.y - currentState.posOffset.y);

  float spdDiffSq = lengthSq(currentState.spdSummaryDiff);
  if (float_nonzero(currentState.deltaVelIgnoreAmount) && float_nonzero(spdDiffSq))
  {
    float spdDiff = sqrtf(spdDiffSq);
    float resIgnoreAmount = min(currentState.deltaVelIgnoreAmount, spdDiff);
    currentState.spdSummaryDiff *= 1.f - safediv(resIgnoreAmount, spdDiff);
    currentState.deltaVelIgnoreAmount -= resIgnoreAmount;
  }
  currentState.acceleration = (currentState.velocity - startVelocity) * safeinv(dt);

  currentState.atTick = curTick;
  currentState.lastAppliedControlsForTick = appliedCT.producedAtTick;
}

bool HumanPhys::isAiming() const
{
  return getActor()->getRole() & (IPhysActor::URF_LOCAL_CONTROL | IPhysActor::URF_AUTHORITY)
           ? currentState.getAimPos() > 0.f && appliedCT.isControlBitSet(HCT_AIM)
           : currentState.isAiming();
}

bool HumanPhys::isZooming() const
{
  return getActor()->getRole() & (IPhysActor::URF_LOCAL_CONTROL | IPhysActor::URF_AUTHORITY)
           ? currentState.getZoomPos() > 0.f && appliedCT.isControlBitSet(HCT_ZOOM_VIEW)
           : currentState.isZooming();
}

void HumanPhys::updatePhysInWorld(const TMatrix &tm)
{
  if (!torsoCollision)
    return;

  const bool shouldTorsoBeInWorld = currentState.alive && currentState.isVisible;
  if (!shouldTorsoBeInWorld)
  {
    if (isTorsoInWorld)
      dacoll::remove_object_from_world(torsoCollision);
  }
  else if (!isTorsoInWorld)
    dacoll::add_object_to_world(torsoCollision);

  isTorsoInWorld = shouldTorsoBeInWorld;
  dacoll::tmp_collisions_t objects;
  Point2 oriParam = calc_ori_param(currentState, appliedCT);
  dacoll::generate_collisions(tm, oriParam, collisionLinks[currentState.getStandState()], objects);
  for (const dacoll::CollisionCapsuleProperties &obj : objects)
  {
    if (!obj.haveCollision)
      continue;
    dacoll::set_vert_capsule_shape_size(torsoCollision, collRad * obj.scale.x, obj.scale.y);
    dacoll::set_collision_object_tm(torsoCollision, obj.tm);
  }
}

dag::ConstSpan<CollisionObject> HumanPhys::getCollisionObjects() const { return dag::ConstSpan<CollisionObject>(&torsoCollision, 1); }

TMatrix HumanPhys::getCollisionObjectsMatrix() const
{
  TMatrix curTm = currentState.location.makeTM();
  dacoll::tmp_collisions_t objects;
  Point2 oriParam = calc_ori_param(currentState, appliedCT);
  dacoll::generate_collisions(curTm, oriParam, collisionLinks[currentState.getStandState()], objects);
  for (const dacoll::CollisionCapsuleProperties &obj : objects)
  {
    if (!obj.haveCollision)
      continue;
    return obj.tm;
  }
  return curTm;
}

void HumanPhys::applyOffset(const Point3 &offset) { currentState.location.P += dpoint3(offset); }

void HumanPhys::reset()
{
  currentState.reset();
  currentState.height = 1.f;
  currentState.resetStamina(maxStamina * currentState.maxStaminaMult * currentState.staminaBoostMult);
  producedCT.setNeutralCtrl();
  producedCT.setChosenWeapon(EWS_PRIMARY);
}

bool HumanPhys::canStartSprint() const { return currentState.stamina > sprintStartStaminaLevel * currentState.maxStaminaMult; }

bool HumanPhys::isStaminaFull() const
{
  return currentState.stamina == maxStamina * currentState.maxStaminaMult * currentState.staminaBoostMult;
}

float HumanPhys::getJumpStaminaDrain() const { return jumpStaminaDrain; }
float HumanPhys::getClimbStaminaDrain() const { return climbStaminaDrain; }


CONSOLE_BOOL_VAL("walkerphys", draw_collision, false);
CONSOLE_BOOL_VAL("walkerphys", draw_gun_trace, false);
CONSOLE_BOOL_VAL("walkerphys", draw_gun_debug, false);
CONSOLE_BOOL_VAL("walkerphys", draw_proc_collision, false);
CONSOLE_BOOL_VAL("walkerphys", draw_sprint_collision, false);
CONSOLE_BOOL_VAL("walkerphys", draw_climber_collision, false);
CONSOLE_BOOL_VAL("walkerphys", draw_coll_center, false);
CONSOLE_BOOL_VAL("walkerphys", draw_ccd_collision, false);
CONSOLE_BOOL_VAL("walkerphys", draw_walk_query, false);
CONSOLE_BOOL_VAL("walkerphys", draw_walk_height, false);
CONSOLE_BOOL_VAL("walkerphys", draw_spd, false);
CONSOLE_BOOL_VAL("walkerphys", draw_standing_spd, false);
CONSOLE_BOOL_VAL("walkerphys", draw_body_yaw, false);
CONSOLE_BOOL_VAL("walkerphys", draw_gun_angles, false);
CONSOLE_BOOL_VAL("walkerphys", draw_walk_traces, false);
CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache, false);
CONSOLE_BOOL_VAL("walkerphys", draw_ladder_state, false);
CONSOLE_BOOL_VAL("walkerphys", draw_height_state, false);
CONSOLE_BOOL_VAL("walkerphys", draw_climb_state, false);
CONSOLE_BOOL_VAL("walkerphys", draw_climb_on_pos, false);
CONSOLE_BOOL_VAL("walkerphys", debug_climb_perf, false);

CONSOLE_BOOL_VAL("walkerphys", draw_walk_res, false);

CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache_stats, false);
CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache_invalidates, false);
CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache_casts, false);
CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache_misses, false);
CONSOLE_BOOL_VAL("walkerphys", draw_trace_cache_query_box, false);

CONSOLE_BOOL_VAL("walkerphys", draw_air_state, false);

void HumanPhys::drawDebug()
{
  TMatrix tm;
  visualLocation.toTM(tm);
  if (draw_collision)
  {
    TMatrix torsoCollisionTm = tm;
    float torsoHeight = lerp(crouchHeight, standingHeight, saturate(currentState.height)) - collRad;
    Point3 torsoPos = Point3(0.f, torsoHeight * 0.5f + collRad, 0.f);
    if (currentState.height >= 0.f)
      torsoCollisionTm.setcol(3, tm * torsoPos);
    else
      torsoCollisionTm.setcol(3, tm * lerp(torsoPos, Point3(0.5f, torsoHeight * 0.5f + 0.2f, 0.f), saturate(-currentState.height)));
    dacoll::fetch_sim_res(true);
    dacoll::set_vert_capsule_shape_size(torsoCollision, collRad, torsoHeight * 0.5f);
    dacoll::set_collision_object_tm(torsoCollision, torsoCollisionTm);
    dacoll::draw_collision_object(torsoCollision);
  }

  if (draw_gun_angles)
  {
    Point2 wishShootAngles = dir_to_angles(normalize(appliedCT.getWishShootDir()));
    String str(128, "gunAngles (%f, %f)", RadToDeg(currentState.gunAngles.x), RadToDeg(currentState.gunAngles.y));
    String strwish(128, "wishAngle (%f, %f)", RadToDeg(renorm_ang(wishShootAngles.x, currentState.gunAngles.x)),
      RadToDeg(wishShootAngles.y));
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), str.str());
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), strwish.str(), -1, 1);
  }

  dacoll::tmp_collisions_t objects;
  if (draw_proc_collision)
  {
    Point2 oriParam = calc_ori_param(currentState, appliedCT);
    generate_collisions(tm, oriParam, collisionLinks[currentState.collisionLinksStateTo], objects);
    if (currentState.collisionLinkProgress >= 0.f)
    {
      dacoll::tmp_collisions_t prevObjects;
      dacoll::generate_collisions(tm, oriParam, collisionLinks[currentState.collisionLinksStateFrom], prevObjects);
      dacoll::lerp_collisions(objects, prevObjects, currentState.collisionLinkProgress);
    }
  }

  if (draw_walk_height)
  {
    draw_debug_line_buffered(tm * Point3(1.f, maxObstacleHeight, 0.f), tm * Point3(-1.f, maxObstacleHeight, 0.f));
    draw_debug_line_buffered(tm * Point3(0.f, maxObstacleHeight, 1.f), tm * Point3(0.f, maxObstacleHeight, -1.f));
  }

  if (draw_gun_trace)
  {
    TMatrix gunTm = calcGunTm(tm, currentState.gunAngles.y, currentState.leanPosition, currentState.height);
    float gunLen = weaponParams[currentState.weapEquipState.curSlot].gunLen;
    Point3 gunAimPos = gunTm * weaponParams[currentState.weapEquipState.curSlot].offsAimNode;
    Point3 leftPos = gunTm * weaponParams[currentState.weapEquipState.curSlot].offsCheckLeftNode;
    Point3 rightPos = gunTm * weaponParams[currentState.weapEquipState.curSlot].offsCheckRightNode;
    Point3 gunDir = gunTm % Point3(gunLen, 0.f, 0.f);
    draw_debug_line_buffered(gunTm * Point3(0.f, 0.f, 0.f), gunTm * Point3(gunLen, 0.f, 0.f));
    draw_debug_line_buffered(gunAimPos, gunAimPos + gunDir);
    draw_debug_line_buffered(leftPos, leftPos + gunDir);
    draw_debug_line_buffered(rightPos, rightPos + gunDir);
    draw_debug_line_buffered(gunTm * Point3(gunLen, 0.f, 0.f), gunTm * Point3(gunLen * 4.f, 0.f, 0.f),
      E3DCOLOR_MAKE(0, 255, 255, 255));
  }

  if (draw_gun_debug)
  {
    String str(128, "curweap %d wishweap %d state %d, canswitch: %s", appliedCT.getChosenWeapon(), currentState.weapEquipState.curSlot,
      currentState.weapEquipState.curState, canSwitchWeapon ? "yes" : "no");
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), str.str());
  }

  if (draw_proc_collision)
  {
    dacoll::fetch_sim_res(true);
    Point3 fromPos = tm.getcol(3) + Point3(0.f, maxObstacleHeight + collRad, 0.f); // for traces
    for (int i = 0; i < objects.size(); ++i)
    {
      const dacoll::CollisionCapsuleProperties &obj = objects[i];
      const float rad = obj.haveCollision ? collRad : 0.001f;
      dacoll::set_vert_capsule_shape_size(torsoCollision, rad * obj.scale.x, obj.scale.y);
      dacoll::set_collision_object_tm(torsoCollision, obj.tm);
      dacoll::draw_collision_object(torsoCollision);

      Point3 toPos = obj.tm * Point3(0.f, obj.scale.y, 0.f);
      draw_debug_line_buffered(fromPos, toPos);
      fromPos = toPos;
    }
  }

  if (draw_sprint_collision)
  {
    Capsule cap;
    cap.set(sprintStopCollisionStart, sprintStopCollisionEnd, sprintStopCollisionRad);
    cap.transform(tm);
    draw_debug_capsule_w(cap, E3DCOLOR_MAKE(255, 255, 255, 255));
  }

  if (draw_climber_collision)
  {
    if (climberCollision.isValid())
    {
      dacoll::set_collision_object_tm(climberCollision, tm);
      dacoll::draw_collision_object(climberCollision);
    }
  }

  Point3 curCollCenter = calcCollCenter();

  if (draw_coll_center)
    draw_debug_sph(tm * curCollCenter, 0.1f, E3DCOLOR_MAKE(0, 255, 0, 255));

  if (draw_ccd_collision)
    draw_debug_sph(tm * calcCcdPos(), ccdRad);

  if (draw_body_yaw)
  {
    float bodyYaw = RadToDeg(dir_to_yaw(currentState.bodyOrientDir));
    float walkYaw = RadToDeg(dir_to_yaw(currentState.walkDir));
    String str(128, "walkYaw %f, bodyYaw %f", walkYaw, bodyYaw);
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), str.str());
  }
  if (draw_standing_spd)
  {
    String spdText(128, "spd = %f m/s", length(currentState.standingVelocity));
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), spdText.str(), -1, 1);
  }
  if (draw_spd)
  {
    String spdText(128, "spd = %f m/s with wish %f", length(currentState.velocity), appliedCT.getWalkSpeed());
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.f, 0.f), spdText.str(), -1, 1);
  }
  if (draw_ladder_state)
  {
    String ladderText(128, "ladder = %s", currentState.attachedToLadder ? "attached" : "nope");
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.5f, 0.f), ladderText.str(), -1, 1);
  }

  if (draw_height_state)
  {
    String heightText(128, "height = %f", currentState.height);
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 0.5f, 0.f), heightText.str(), -1, 1);
  }

  if (draw_walk_query)
  {
    TMatrix curTm;
    currentState.location.toTM(curTm);

    Point3 walkQueryPos = tm * curCollCenter;
    const float obstacle_ht = maxObstacleHeight + collRad;
    WalkQueryResults walkQuery =
      queryWalkPosition(walkQueryPos, obstacle_ht, obstacle_ht, false, false, walkRad, Point3(0.f, 0.f, 0.f));
    if (walkQuery.haveResults)
    {
      String str(0, "step %f, %f, isSliding %d, isInAir %d", walkQuery.stepSize, currentState.velocity.y, walkQuery.isSliding,
        walkQuery.isInAir);
      add_debug_text_mark(tm * Point3(0.f, 1.5f, 0.f), str.str());
    }
  }

  if (draw_walk_res)
    draw_debug_line_buffered(Point3::xyz(currentState.location.P), Point3::xyz(currentState.location.P) + currentState.walkNormal);

  if (draw_walk_traces)
  {
    Point3 curMaxPos = tm * Point3(0.f, maxObstacleHeight, 0.f);
    Point3 curMinPos = tm * Point3(0.f, -maxObstacleHeight, 0.f);
    {
      dacoll::ShapeQueryOutput shapeQuery;
      TraceMeshFaces *handle = getTraceHandle();
      if (dacoll::sphere_cast_ex(curMaxPos, curMinPos, walkRad, shapeQuery, rayMatId, make_span_const(&torsoCollision, 1), handle))
      {
        draw_debug_box_buffered(BBox3(shapeQuery.res, 0.1f));
        draw_debug_line_buffered(shapeQuery.res, shapeQuery.res + shapeQuery.norm);
        float t = maxObstacleHeight * 2.f;
        int matId = -1;
        Point3 norm;
        if (dacoll::tracedown_normalized(curMaxPos, t, &matId, &norm, dacoll::ETF_DEFAULT, nullptr, rayMatId, handle))
        {
          Point3 pos = curMaxPos + Point3(0.f, -t, 0.f);
          draw_debug_line_buffered(pos, pos + norm);
          draw_debug_box_buffered(BBox3(pos, 0.05f));
        }
      }
    }
    {
      dacoll::ShapeQueryOutput shapeQuery;
      if (dacoll::sphere_cast_ex(curMaxPos, curMinPos, walkRad, shapeQuery, rayMatId, make_span_const(&torsoCollision, 1), nullptr))
      {
        draw_debug_box_buffered(BBox3(shapeQuery.res, 0.1f));
        draw_debug_line_buffered(shapeQuery.res, shapeQuery.res + shapeQuery.norm, E3DCOLOR_MAKE(255, 255, 255, 255));
        float t = maxObstacleHeight * 2.f;
        int matId = -1;
        Point3 norm;
        if (dacoll::tracedown_normalized(curMaxPos, t, &matId, &norm, dacoll::ETF_DEFAULT, nullptr, rayMatId, nullptr))
        {
          Point3 pos = curMaxPos + Point3(0.f, -t, 0.f);
          draw_debug_line_buffered(pos, pos + norm, E3DCOLOR_MAKE(255, 255, 255, 255));
        }
      }
    }
  }

  TraceMeshFaces *handle = getTraceHandle();
  if (draw_trace_cache)
    gamephys::draw_trace_handle(handle);
  if (draw_trace_cache_stats)
    gamephys::draw_trace_handle_debug_stats(handle, tm.getcol(3));
  else
    TRACE_HANDLE_DEBUG_STATS_RESET(handle);
  handle->debugDrawInvalidates = draw_trace_cache_invalidates;
  handle->debugDrawCasts = draw_trace_cache_casts;
  handle->debugDrawMisses = draw_trace_cache_misses;
  if (draw_trace_cache_query_box)
  {
    BBox3 box;
    v_stu_bbox3(box, traceCacheOOBB);
    draw_debug_box_buffered(currentState.location.makeTM() * box);
  }

  if (draw_climb_state)
  {
    draw_debug_box_buffered(BBox3(currentState.climbToPos, 0.1f));
    draw_debug_box_buffered(BBox3(currentState.climbFromPos, 0.1f));
    draw_debug_box_buffered(BBox3(currentState.climbContactPos, 0.05f));
    draw_debug_line_buffered(currentState.climbToPos, currentState.climbContactPos);
    draw_debug_line_buffered(currentState.climbContactPos, currentState.climbFromPos);
    draw_debug_line_buffered(currentState.climbToPos, currentState.climbToPos + currentState.climbNorm);
    if (currentState.climbThrough)
      add_debug_text_mark(currentState.climbToPos, "climbing through", -1, 1);
    if (currentState.isFastClimbing)
      add_debug_text_mark(currentState.climbToPos + Point3(0.0, 0.1, 0.0), "fast climb", -1, 1);
    if (currentState.isClimbingOverObstacle)
    {
      draw_debug_box_buffered(BBox3(currentState.climbPosBehindObstacle, 0.1f));
      draw_debug_line_buffered(currentState.climbFromPos, currentState.climbPosBehindObstacle);
      add_debug_text_mark(currentState.climbToPos + Point3(0.0, 0.2, 0.0), "climbing over", -1, 1);
    }
  }
  if (draw_climb_on_pos)
  {
    draw_debug_sph(tm * climbOnPos, climbOnRad, E3DCOLOR_MAKE(0, 255, 0, 255));
  }
  if (draw_air_state)
  {
    int curPos = 0;
    for (int i = 0; i < 8; ++i)
      if (currentState.isInAirHistory & (1 << i))
      {
        String airStr(0, "Was in air %d frames ago", i);
        add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.6f, 0.f), airStr.str(), -1, curPos++);
      }
  }
  if (debug_climb_perf)
  {
    int64_t startTime = ref_time_ticks();
    for (int i = 0; i < 10; ++i)
      climbQuery(Point3(0.f, 0.f, 0.f), rayMatId, true, true, true, false, false, getTraceHandle());
    int resTime = get_time_usec(startTime);
    String timeText(128, "time = %d usec", resTime);
    add_debug_text_mark(tm.getcol(3) + Point3(0.f, 1.3f, 0.f), timeText.str(), -1, 1);
  }
}

void HumanPhys::setWeaponLen(float len, int slot_id)
{
  weaponParams[slot_id >= 0 ? slot_id : currentState.weapEquipState.curSlot].gunLen = len;
}

void HumanPhys::setStateFlag(HumanPhysState::StateFlag flag, bool enable)
{
  if (enable)
    currentState.states |= flag;
  else
    currentState.states &= ~flag;
}

void HumanPhys::setTmRough(TMatrix tm)
{
  G_ASSERT(tm.det() > 0);
  TMatrix prevTm = currentState.location.makeTM();
  BaseType::setTmRough(tm);

  TMatrix itm = inverse(prevTm);
  currentState.walkDir = Point2::xz(tm % (itm % Point3::x0y(currentState.walkDir)));
  currentState.bodyOrientDir = Point2::xz(tm % (itm % Point3::x0y(currentState.bodyOrientDir)));
  currentState.headDir = tm % (itm % currentState.headDir);
  currentState.gunDir = tm % (itm % currentState.gunDir);
  currentState.gunAngles = dir_to_angles(tm % (itm % angles_to_dir(currentState.gunAngles)));
  currentState.prevAngles = dir_to_angles(tm % (itm % angles_to_dir(currentState.prevAngles)));
}

void HumanPhys::restoreStamina(float dt, float mult)
{
  currentState.stamina = min(currentState.stamina + restStaminaRestore * dt * currentState.restoreStaminaMult * mult,
    maxStamina * currentState.maxStaminaMult * currentState.staminaBoostMult);
}
