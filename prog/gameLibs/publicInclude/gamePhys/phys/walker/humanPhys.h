//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/commonPhysBase.h>
#include <EASTL/unique_ptr.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/contactData.h>
#include <generic/dag_carray.h>
#include <util/dag_simpleString.h>
#include <gamePhys/collision/collisionLinks.h>
#include <gamePhys/phys/walker/humanWeapPosInfo.h>
#include <math/dag_mathUtils.h>
#include <limits.h>
#include "humanWeapState.h"
#include "humanControlState.h"
#include <gameMath/quantization.h>
#include <gameMath/traceUtils.h>
#include <scene/dag_physMat.h>

// This causing controls overhead (and don't needed if you don't use 'change unit inplace' hack)
#ifndef HUMAN_USE_UNIT_VERSION
#define HUMAN_USE_UNIT_VERSION 0
#endif

namespace gamephys
{
struct CollisionContactData;
};

class GeomNodeTree;
struct PrecomputedWeaponPositions;
class ECSCustomPhysStateSyncer;

enum HUMoveState : uint8_t
{
  EMS_STAND,
  EMS_WALK,
  EMS_RUN,
  EMS_SPRINT,
  EMS_ROTATE_LEFT,
  EMS_ROTATE_RIGHT,
  EMS_NUM
};

enum HUStandState : uint8_t
{
  ESS_STAND,
  ESS_CROUCH,
  ESS_CRAWL,
  ESS_DOWNED,
  ESS_SWIM,
  ESS_SWIM_UNDERWATER,
  ESS_CLIMB,
  ESS_CLIMB_THROUGH,
  ESS_CLIMB_OVER,
  ESS_CLIMB_LADDER,
  ESS_EXTERNALLY_CONTROLLED,
  ESS_NUM,
};

struct HumanDmgState
{
  float legsHitTimer;

  HumanDmgState() : legsHitTimer(0.f) {}
};

struct HumanSerializableState //-V730
{
  int32_t atTick = -1;
  bool canBeCheckedForSync;

  gamephys::Loc location;
  Point3 velocity;

  Point3 standingVelocity;
  Point2 walkDir;
  Point2 bodyOrientDir;

  Point3 walkNormal = Point3(0.f, 1.f, 0.f);
  Point3 vertDirection = Point3(0.f, 1.f, 0.f);

  Point3 posOffset = Point3(0.f, 0.f, 0.f);

  Point3 gravDirection = Point3(0.f, -1.f, 0.f); // should be inverse of vert direction in most of the cases

  Point3 gunDir;
  Point2 breathOffset;
  Point2 handsShakeOffset;
  Point3 headDir;
  Point2 gunAngles;
  Point2 targetGunSpd;
  Point2 gunSpd;
  Point2 prevAngles;

  Point2 gunAimOffset = Point2(0.f, 0.f);
  float height = 1.f; // [-1..1]
  float heightCurVel;

  float gravMult = 1.f;

  float stamina;
  float aimPosition = 0.f; // [0..1]
  float leanPosition = 0.f;
  float extHeight = 1.f;
  float aimForZoomProgress = 0.f;

  float breathShortness;
  float breathShakeMult;
  float breathTimer;
  float breathTime = 0.f;
  float handsShakeTime = 0.f;
  float handsShakeMagnitude;
  float handsShakeSpeedMult;

  float zoomPosition = 0.f;

  float kineticEnergy;

  float knockBackTimer = 0.f; // timer when you'll be treated as is in air for controllability

  Point3 spdSummaryDiff = ZERO<Point3>();
  float deltaVelIgnoreAmount = 0.f;

  float distFromWaterSurface = 0.f; // used only when swimming to enable wave compensation

  // these are very rarely changed, probably we could optimize them somehow in network (delta with prev?)
  float staminaDrainMult = 1.f;
  float moveSpeedMult = 1.f;
  float swimmingSpeedMult = 1.f;
  float jumpSpeedMult = 1.f;
  float climbingSpeedMult = 1.f;
  float staminaSprintDrainMult = 1.f;
  float staminaClimbDrainMult = 1.f;
  float accelerationMult = 1.f;
  float maxStaminaMult = 1.f;
  float restoreStaminaMult = 1.f;
  float staminaBoostMult = 1.f;
  float sprintSpeedMult = 1.f;
  float sprintLerpSpeedMult = 1.f; // 0.f means use run speed and disallow sprint, while 1.f means - do nothing.
  float breathAmplitudeMult = 1.f;
  float fasterChangeWeaponMult = 1.f;
  float crawlCrouchSpeedMult = 1.f;
  float fasterChangePoseMult = 1.f;
  float weaponTurningSpeedMult = 1.f;
  float aimingAfterFireMult = 1.f;
  float aimSpeedMult = 1.f;
  float speedInAimStateMult = 1.f;

  uint16_t overrideWallClimb : 1;
  uint16_t alive : 1;
  uint16_t isDowned : 1;
  uint16_t controllable : 1;
  uint16_t ignoresHumanCollision : 1;
  uint16_t canShoot : 1;
  uint16_t canAim : 1;
  uint16_t canZoom : 1;
  uint16_t disableCollision : 1;
  uint16_t forceWeaponDown : 1;
  uint16_t isClimbing : 1;
  uint16_t isSwimming : 1;
  uint16_t isUnderwater : 1;
  uint16_t isHoldBreath : 1;
  uint16_t attachedToLadder : 1;
  uint16_t pulledToLadder : 1;
  uint16_t blockSprint : 1;
  uint16_t climbThrough : 1;
  uint16_t isClimbingOverObstacle : 1;
  uint16_t isFastClimbing : 1;
  uint16_t isAttached : 1;
  uint16_t cancelledClimb : 1;
  uint16_t isMount : 1;
  uint16_t detachedFromLadder : 1;
  uint16_t isVisible : 1;
  uint16_t externalControlled : 1;
  uint16_t reduceToWalk : 1;
  uint16_t isLadderQuickMovingUp : 1;
  uint16_t isLadderQuickMovingDown : 1;

  TMatrix ladderTm = TMatrix::IDENT;
  Point3 climbFromPos = Point3(0.f, 0.f, 0.f);    // where we started our climbing
  Point3 climbContactPos = Point3(0.f, 0.f, 0.f); // top position if we have an offset in climbing
  Point3 climbToPos = Point3(0.f, 0.f, 0.f);      // this is our target pos
  Point3 climbToPosVel = Point3(0.f, 0.f, 0.f);   // this is velocity of our target pos. It might be a moving platform
  Point3 climbDir = Point3(1.f, 0.f, 0.f);        // this is our climb direction
  Point3 climbNorm = Point3(0.f, 1.f, 0.f);       // normal on which we'll stand after climb
  Point3 climbPosBehindObstacle = Point3(0.f, 0.f, 0.f);
  float climbDeltaHt = 0.f;
  float climbStartAtTime = 0.f;

  float gunBackoffAmount = 0.f;
  float gunTraceTimer = 0.f;

  float fallbackToSphereCastTimer = 0.f;

  uint8_t stoppedSprint : 1;
  uint8_t attachedToExternalGun : 1;
  uint8_t forceWeaponUp : 1;
  uint8_t useSecondaryCcd : 1;
  float lastDashTime = 0.f;
  HUMoveState moveState;
  uint8_t isInAirHistory;
  uint8_t climbGridCounter;

  float jumpStartTime = -1.f;
  float afterJumpDampingEndTime = -1.f;
  float afterJumpDampingHeight = 1.f;

  float frictionMult = 1.f;

  enum StateFlag : uint8_t
  {
    ST_JUMP = (1 << 0),
    ST_CROUCH = (1 << 1),
    ST_CRAWL = (1 << 2),
    ST_ON_GROUND = (1 << 3),
    ST_SPRINT = (1 << 4),
    ST_WALK = (1 << 5),
    ST_SWIM = (1 << 6),
    ST_DOWNED = (1 << 7)
  };
  uint8_t states = ST_ON_GROUND;

  HUStandState collisionLinksStateFrom;
  HUStandState collisionLinksStateTo;
  float collisionLinkProgress;

  HumanWeaponEquipState weapEquipState;
};

struct HumanPhysState : public HumanSerializableState
{
  int lastAppliedControlsForTick = -2; // < atTick

  Point3 omega;
  Point3 alpha;
  Point3 acceleration;

  Point3 meanCollisionNormal = Point3(0.f, 0.f, 0.f);

  int lastCcdCollisionTick = -1;

  float ladderAttachProgress = 0.f;
  uint16_t ladderNumSteps = 0;
  bool guidedByLadder = false;

  bool isControllable = true;
  bool isWishToMove = false; // I.e. if moving by controls

  Point3 torsoCollisionTmPos = Point3(0.f, 0.f, 0.f);

  int walkMatId = 0;
  int torsoContactMatId = PHYSMAT_INVALID;
  int16_t torsoContactRendinstPool = -1;
  // TODO: replace to StaticTab<> after fixing it's `is_trivially_destructible` trait
  uint16_t numTorsoContacts = 0;
  carray<gamephys::CollisionContactDataMin, 2> torsoContacts;

  HumanPhysState();

  void reset();
  void resetStamina(float max_stamina);

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs, IPhysBase &);

  void applyAlternativeHistoryState(const HumanPhysState &state);
  void applyPartialState(const CommonPhysPartialState &state);
  void applyDesyncedState(const HumanPhysState & /*state*/);

  bool isAttachedToLadder() const { return attachedToLadder; }
  bool isDetachedFromLadder() const { return detachedFromLadder; }
  bool isExternalControlled() const { return externalControlled; }

  bool operator==(const HumanPhysState &a) const
  {
    static constexpr const float posThresholdSq = 1e-8f;      // 0.1 mm (10% of 1 mm)
    static constexpr const float velDiffThresholdSq = 0.01f;  // 10cm (0.1m) per sec
    static constexpr float cosDirThreshold = 0.999998476913f; // cos of 0.1 degrees
    static constexpr const float orientThresholdDegrees = 0.1f;

    if (lengthSq(location.P - a.location.P) > posThresholdSq)
      return false;

    float velSq = velocity.lengthSq();
    float aVelSq = a.velocity.lengthSq();

    if (fabsf(velSq - aVelSq) > velDiffThresholdSq)
      return false;
    if (velSq > 1e-12f && (velocity / sqrtf(velSq)) * (a.velocity * safeinv(sqrtf(aVelSq))) < cosDirThreshold)
      return false;

    const Point3 &ypr = location.O.getYPR();
    const Point3 &aYpr = a.location.O.getYPR();

    return fabsf(ypr[0] - aYpr[0]) < orientThresholdDegrees && fabsf(ypr[1] - aYpr[1]) < orientThresholdDegrees &&
           fabsf(ypr[2] - aYpr[2]) < orientThresholdDegrees && (gunDir * a.gunDir) > cosDirThreshold &&
           (headDir * a.headDir) > cosDirThreshold && fabsf(height - a.height) < 1e-6f;
  }

  bool hasLargeDifferenceWith(const HumanPhysState &a) const
  {
    const Point3 &ypr = location.O.getYPR();
    const Point3 &aYpr = a.location.O.getYPR();

    const float ANGLE_THRESHOLD = 0.002f;

    bool isLargeDifference = lengthSq(location.P - a.location.P) > 0.01f || fabsf(ypr[0] - aYpr[0]) > ANGLE_THRESHOLD ||
                             fabsf(ypr[1] - aYpr[1]) > ANGLE_THRESHOLD || fabsf(ypr[2] - aYpr[2]) > ANGLE_THRESHOLD ||
                             lengthSq(velocity - a.velocity) > 0.01f || lengthSq(gunDir - a.gunDir) > 0.001f ||
                             lengthSq(headDir - a.headDir) > 0.001f || (heightCurVel - a.heightCurVel) > 0.01f ||
                             (height - a.height) > 0.01f;
    return isLargeDifference;
  }

  float getHeight() const { return height; }
  void setHeight(float ht) { height = clamp(ht, -1.f, 1.f); }
  bool isCrouch() const { return height > -0.5f && height < 0.5f; }
  bool isCrawl() const { return height <= -0.5f; }

  bool isAiming() const { return aimPosition > 0.5f; }
  bool isZooming() const { return zoomPosition > 0.5f; }
  float getAimPos() const { return aimPosition; }
  float getZoomPos() const { return zoomPosition; }
  void setAimPos(float ap) { aimPosition = clamp(ap, 0.f, 1.f); }

  HUStandState getStandState() const
  {
    if (externalControlled)
      return ESS_EXTERNALLY_CONTROLLED;
    if (isClimbing)
      return climbThrough ? ESS_CLIMB_THROUGH : isClimbingOverObstacle ? ESS_CLIMB_OVER : ESS_CLIMB;
    if (isSwimming)
      return isUnderwater ? ESS_SWIM_UNDERWATER : ESS_SWIM;
    if (isDowned)
      return ESS_DOWNED;
    if (isCrawl())
      return ESS_CRAWL;
    if (isCrouch())
      return ESS_CROUCH;
    if (attachedToLadder || pulledToLadder)
      return ESS_CLIMB_LADDER;
    return ESS_STAND;
  }
};
DAG_DECLARE_RELOCATABLE(HumanPhysState);

#if !_TARGET_PC && !defined(_MSC_VER)
G_STATIC_ASSERT(offsetof(HumanPhysState, lastAppliedControlsForTick) % alignof(HumanSerializableState) == 0);
// Add HumanPhysState::_no_tail_padding if this assertion fails
#endif

struct ClimbQueryResults
{
  bool canClimb = false;
  bool minPos = false;
  bool wallAhead = false; // Set only on failure, means that query failed due to the wall ahead of us.
  bool isClimbThrough = false;
  Point3 climbToPos = Point3(0.f, 0.f, 0.f);
  Point3 climbToPosVel = Point3(0.f, 0.f, 0.f);
  Point3 climbFromPos = Point3(0.f, 0.f, 0.f);
  Point3 climbContactPos = Point3(0.f, 0.f, 0.f);
  Point3 climbOverheadPos = Point3(0.f, 0.f, 0.f);
  Point3 climbNorm = Point3(0.f, 1.f, 0.f);
  Point3 climbDir = Point3(0.f, 1.f, 0.f);
};

class HumanPhys final : public PhysicsBase<HumanPhysState, HumanControlState, CommonPhysPartialState>
{
  using BaseType = PhysicsBase<HumanPhysState, HumanControlState, CommonPhysPartialState>;

  DPoint3 ccdMove = {0.0, 0.0, 0.0};
  carray<carray<float, EMS_NUM>, ESS_NUM> walkSpeeds;
  carray<float, ESS_NUM> rotateSpeeds;
  carray<float, ESS_NUM> alignSpeeds;
  carray<Point2, ESS_NUM> rotateAngles;
  carray<float, EMS_NUM> fricitonByState;
  carray<float, EMS_NUM> accelerationByState;
  float externalFrictionPerSpeedMult = 5.0;
  float minWalkSpeed = 0.01f;
  float aimingMoveSpeed = 1.f;
  float moveStateThreshold = 0.5f;

  float acceleration = 5.f;
  float frictionGroundCoeff = 4.f;
  float frictionThresSpdMult = 0.5f;

  int accelSubframes = 4; // have 4 subframes for acceleration/friction calculation

  float airAccelerate = 3.f;

  float minWalkControl = 0.3f;

  Point2 waterFrictionResistance = Point2(0.5f, 0.75f);
  Point2 waterJumpingResistance = Point2(0.25f, 0.9f);
  carray<carray<float, EMS_NUM>, ESS_NUM> wishVertJumpSpeed;
  carray<carray<float, EMS_NUM>, ESS_NUM> wishHorzJumpSpeed;
  float maxJumpHeight = 0.f;
  float beforeJumpDelay = 0.0f;
  float beforeJumpCrouchHeight = 0.5f;
  uint8_t jumpInAirHistoryMask = uint8_t(~1);
  bool canWallJump = false;
  bool climber = false;
  bool isInertMovement = false;
  bool canStartSprintWithResistance = false;
  bool additionalHeightCheck = false;

  float ladderClimbSpeed = 1.f;
  float ladderQuickMoveUpSpeedMult = -1.0f;
  float ladderQuickMoveDownSpeedMult = -1.0f;
  float ladderCheckClimbHeight = 1.9f;
  float ladderZeroVelocityHeight = 1.7f;
  float ladderMaxOverHeight = 0.1f;

  float stepAccel = 8.f;

  carray<Point3, ESS_NUM> collisionCenterPos;
  carray<Point3, ESS_NUM> ccdPos;
  float maxObstacleHeight = 0.5f;
  float maxStepOverHeight = 0.0f;
  float maxCrawlObstacleHeight = 0.5f;
  float maxObstacleDownReach = 0.1f;

  Point2 forceDownReachDists = Point2(-1.0f, 0.0f);
  float forceDownReachVel = 0.f;

  float turnSpeedStill = TWOPI;
  float turnSpeedWalk = 1.5f * PI;
  float turnSpeedRun = PI;
  float turnSpeedSprint = HALFPI;

  float maxSprintSin = 0.17f;
  float maxRunSin = 0.7f;

  const float heightChangeTauDef = 0.2f;
  const float heightChangeDeltaDef = 0.3f;
  static float calc_spring_k(float tau, float delta) { return sqr(PI / 2 / tau) + sqr(-log(delta) / tau); }
  static float calc_damping(float tau, float delta) { return -2 * log(delta) / tau; }

  float heightChangeSprK = calc_spring_k(heightChangeTauDef, heightChangeDeltaDef);
  float heightChangeDamp = calc_damping(heightChangeTauDef, heightChangeDeltaDef);
  float heightChangeDeltaClampMin = -1.f;
  float heightChangeDeltaClampMax = 1.f;

  float sprintStaminaDrain = 20.f;
  float restStaminaRestore = 10.f;
  float jumpStaminaDrain = 20.f;
  float climbStaminaDrain = 0.f;
  float sprintEndStaminaLevel = 0;
  float sprintStartStaminaLevel = 30;
  float ladderStaminaDrain = 20.f;

  float standSlideAngle = 50.f;
  float crawlSlideAngle = 35.f;
  float climbSlideAngle = 30.f;

  float crawlWalkNormTau = 0.2f;

  float wallJumpSpd = 5.f;
  Point3 wallJumpOffset = Point3(0.f, 0.f, 0.f);
  CollisionObject wallJumpCollision;

  Point3 climbOnPos = Point3(0.4f, 2.f, 0.f);
  float climbOnRad = 0.2f;
  float climbLadderHeight = 0.6f;
  int climbPositions = 4;

  float climbDetachProjLen = -0.f;
  float swimOffsetForClimb = -0.1;
  float additionalClimbRad = 0.2f;
  int additionalClimbPositions = 4;
  int climbGridFrames = 5;
  int maxClimbPositionsPerFrame = 0;

  float maxClimbSpeed = 5.f;
  float climbVertAccel = 5.f;
  float climbVertBrake = 5.f;
  float climbVertBrakeMaxTime = 0.2f;
  float maxClimbHorzVel = 2.f;
  float climbHorzAccel = 10.f;
  float climbHorzBrake = 10.f;
  float climbBrakeHeight = 1.f;
  float climbTimeout = 2.f;
  float climbDistance = 1.f;
  float climbOverDistance = 1.7f;
  float climbAngleCos = 0.f;
  float climbMinHorzSize = 0.65f;
  float climbOnMinVertSize = 1.1f;
  float climbThroughMinVertSize = 0.65f;
  float climbThroughForwardDist = 0.3f;
  Point3 climbThroughPosOffset = Point3(0.f, 0.25f, 0.f);
  float climbMaxVelLength = 5.f;
  float maxHeightForFastClimbing = -1.f;
  float fastClimbingMult = 2.f;

  float climbOverMaxHeight = -1.f; // < 0 disabled
  float climbOverForwardOffset = 0.7f;
  float climbOverMinHeightBehindObstacle = 0.5f;
  float climbOverStaticVelocity = 2.f;

  CollisionObject torsoCollision;
  CollisionObject climberCollision;

  // Sprint stop collision
  Point3 sprintStopCollisionStart;
  Point3 sprintStopCollisionEnd;
  float sprintStopCollisionRad = 0.1f;
  bool enableSprintStopCollision = false;

  carray<dacoll::CollisionLinks, ESS_NUM> collisionLinks;
  bool isTorsoInWorld = true;

  float stateChangeCollAnimSpeed = 4.f;

  SimpleString gunNodeName;
  int gunLinkNameId = -1;

  bbox3f traceCacheOOBB;
  vec3f traceCacheExpands;

  // We try to stand on it
  void applyStandingImpulse(const gamephys::CollisionContactData &contact, Point3 &pos_offs);
  // We try to squiz through it
  void applyPushingImpulse(const gamephys::CollisionContactData &contact, Point3 &pos_offs, float hardness = 1.f,
    float speed_hardness = 1.f);
  void applyPush(const gamephys::CollisionContactData &contact, Point3 &pos_offs, float hardness = 1.f);

  void additionalCollisionChecks(const CollisionObject &co, const TMatrix &tm, Tab<gamephys::CollisionContactData> &out_contacts);

  bool get_normal_and_mat(const Point3 &curMaxPos, const Point3 &dir, float &t, int *matId, Point3 *outNorm, int flags,
    rendinst::RendInstDesc *out_desc, int rayMatId, const TraceMeshFaces *mesh) const;
  void updateStandStateProgress(float dt);
  float updateSwimmingState();

  struct CrawlQueryResults
  {
    Point3 walkNormal = Point3(0.f, 1.f, 0.f);
    Point3 standingVel = Point3(0.f, 0.f, 0.f);
    float stepSize = 0.f;
    float contactStrength = 1.f;
    int walkMatId = 0;
    bool isSliding = false;
    bool isInAir = false;
    bool result = true;
  };

  CrawlQueryResults queryCrawling(const TMatrix &tm, const Point3 &cur_coll_center, float waterLevel, const Point3 &walk_norm,
    const Point3 &coll_norm, bool was_crawling, bool is_in_air, bool is_sliding, float dt);

  void updateClimbing(float at_time, float dt, const TMatrix &tm, const Point3 &vert_dir, const Point3 &cur_coll_center,
    bool torso_collided, const Point3 &torso_coll_norm);

  bool checkWallClimbing(const TMatrix &tm);


  struct WalkQueryResults
  {
    Point3 footstep = Point3(0.f, 0.f, 0.f); // footstep in world positions
    Point3 walkNormal = Point3(0.f, 1.f, 0.f);
    Point3 standingVel = Point3(0.f, 0.f, 0.f);
    float stepSize = 0.f;
    float contactStrength = 1.f;
    float offsetSize = 0.f;
    int walkMatId = 0;
    bool isSliding = false;
    bool isInAir = true;
    bool shouldFallbackToSphereCast = false;
    bool haveResults = false;
  };

  WalkQueryResults queryWalkPosition(const Point3 &pos, float from_ht, float down_ht, bool ignore_slide, bool is_crawl, float walk_rad,
    const Point3 &coll_norm) const;

  struct TorsoCollisionResults
  {
    TMatrix gunNodeTm = TMatrix::IDENT;
    Point3 meanCollisionPos = Point3(0.f, 0.f, 0.f);
    Point3 meanCollisionNormal = Point3(0.f, 0.f, 0.f);
    float frictionViscosityMult = 1.f;
    bool isInAir = true;
    bool isSliding = true;
    bool haveContact = false;
    int torsoContactMatId = PHYSMAT_INVALID;
    StaticTab<gamephys::CollisionContactData, 2> torsoContacts;
    int rendinstPool = -1;
  };
  TorsoCollisionResults processTorsoCollision(TMatrix &tm, int num_iter, float speed_coll_hardness, float at_time);

  void updateHeight(const TMatrix &tm, float dt, float wish_height, bool &isCrawling, bool &isCrouching);

  void updateWaterWavesCompensation(float water_height);

public:
  struct HumanCollision // heap allocated (i.e. not relocatable)
  {
    CollisionObjectUserData collObjUd;
    TraceMeshFaces cachedTraceMeshFaces;
  };

  float speedCollisionHardness = 1.f;
  float crouchHeight = 1.1f;
  float standingHeight = 1.8f;
  float walkRad = 0.2f;
  float collRad = 0.2f;
  float ccdRad = 0.2f;
  float maxStamina = 100.f;
  int rayMatId = -1;
  eastl::unique_ptr<HumanCollision> humanCollision; // not null
  float leanDegrees = 45.f;
  float scale = 1.f;

  float defAimSpeed = 5.f;
  float aimSpeed = 5.f;

  float zoomSpeed = 5.f;

  float crawlLegsOffset = -0.5f;
  float crawlArmsOffset = 0.8f;

  float crawlStepSpeed = 0.5f;
  float mass = 80.f;

  Point3 swimPosOffset = Point3(0.f, 1.2f, 0.f);

  float waterSwimmingLevel = 1.2f;
  float swimmingAcceleration = 10.f;
  float swimmingLevelBias = 0.1f;
  float swimmingLookAngleSin = -0.2f;

  // TODO: move all of this to separate systems/subphys
  bool canCrawl = true;
  bool canCrouch = true;
  bool canClimb = true;
  bool canSprint = true;
  bool canJump = true;
  bool canSwim = true;
  bool canSwitchWeapon = true;
  bool isSimplifiedPhys = false;
  bool isSimplifiedQueryWalkPosition = false;
  bool hasGuns = true; // TODO: as above, move to a separate thing when we'll separate it into ecsphys
  bool hasExternalHeight = true;
  bool canFinishClimbing = true;
  bool canMove = true; // It differs from controllable in that when set to false, it disables _only_ movement

  bool allowWeaponSwitchOnSprint = false;
  float airStaminaRestoreMult = 0.5f;

  Point3 gunOffset = Point3(0.f, 0.f, 0.f); // This offset is applied to precompWeaponPos

  HumanWeaponParamsVec weaponParams;
  const PrecomputedWeaponPositions *precompWeaponPos = nullptr; // if not null points to shared comp data

  HumanPhys(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step);
  ~HumanPhys();

  void loadFromBlk(const DataBlock *blk, const CollisionResource *collision, const char *unit_name, bool is_player);

  void validateTraceCache() override;
  virtual void updatePhys(float at_time, float dt, bool is_for_real) override final;
  void updatePhysInWorld(const TMatrix &tm) override;
  int getCollisionMatId() const { return humanCollision->collObjUd.matId; }
  void setCollisionMatId(int mat_id) { humanCollision->collObjUd.matId = mat_id; }
  HumanPhysState *getAuthorityApprovedState() const { return authorityApprovedState.get(); }
  void updateHeadDir(float dt);
  void updateAimPos(float dt, bool is_aiming);
  virtual dag::ConstSpan<CollisionObject> getCollisionObjects() const override;
  TMatrix getCollisionObjectsMatrix() const override;
  uint64_t getActiveCollisionObjectsBitMask() const override { return isTorsoInWorld ? ALL_COLLISION_OBJECTS : 0ull; }
  dacoll::CollisionLinks &getCollisionLinks(HUStandState state) { return collisionLinks[state]; };
  void applyPseudoVelOmegaDelta(const DPoint3 &add_pos, const DPoint3 &add_ori) override final
  {
    applyPseudoVelOmegaDeltaImpl(add_pos, add_ori, &ccdMove);
  }
  gamephys::Loc lerpVisualLoc(const PhysStateBase &prevState, const PhysStateBase &curState, float time_step,
    float at_time) override final
  {
    // Note: human's velocity is unreliable in case of collision
    return lerpVisualLocImpl(prevState, curState, time_step, at_time, /*lerp_vel*/ false);
  }

  bool processCcdOffset(TMatrix &tm, const Point3 &to_pos, const Point3 &offset, float collision_margin, float speed_hardness,
    bool secondary_ccd, const Point3 &ccd_pos);

  Point3 calcGunPos(const TMatrix &tm, float gun_angles, float lean_pos, float height,
    PrecomputedPresetMode mode = PrecomputedPresetMode::TPV) const;
  TMatrix calcGunTm(const TMatrix &tm, float gun_angles, float lean_pos, float height,
    PrecomputedPresetMode mode = PrecomputedPresetMode::TPV) const;

  Point3 calcCollCenter() const;

  Point3 calcCcdPos() const;
  virtual void setTmRough(TMatrix tm) override final;

  virtual void applyOffset(const Point3 &offset) override;

  virtual void reset() override;

  bool isAiming() const;
  bool isZooming() const;

  virtual float getMass() const override { return mass; }
  virtual DPoint3 calcInertia() const override { return ZERO<DPoint3>(); }
  virtual Point3 getCenterOfMass() const override { return Point3(0.f, 0.5f, 0.f); }

  bool canStartSprint() const;
  bool isStaminaFull() const;
  float getJumpStaminaDrain() const;
  float getClimbStaminaDrain() const;

  void restoreStamina(float dt, float mult = 1.f);

  void tryClimbing(bool was_jumping, bool is_in_air, const Point3 &gun_node_proj, float at_time);
  bool testClimbingIteration(int front_pos, int horz_pos, bool need_min_pos, bool only_one_climb_pos, bool is_in_air,
    const Point3 &gun_node_proj, bool &have_min_pos, bool &have_max_pos, ClimbQueryResults &best_res, float &best_score);
  void finishClimbing(const TMatrix &tm, const Point3 &cur_coll_center, const Point3 &vert_dir);
  ClimbQueryResults climbQueryImpl(const TMatrix &tm, const Point3 &overhead_pos, const Point3 &cur_ccd_pos,
    const Point3 &climb_from_pos, const Point3 &vert_dir, bool check_norm, int ray_mat_id, TraceMeshFaces *handle) const;
  bool climbHoleCheck(const Point3 &worldPos, const Point3 &axis, float min_size, int ray_mat_id, TraceMeshFaces *handle,
    float *lower_dist = nullptr, float *upper_dist = nullptr) const;
  ClimbQueryResults climbThroughQueryImpl(const TMatrix &tm, const Point3 &overhead_pos, const Point3 &cur_ccd_pos,
    const Point3 &climb_from_pos, const Point3 &orig_climb_to_pos, int ray_mat_id, TraceMeshFaces *handle) const;
  ClimbQueryResults climbQuery(const Point3 &offset, int ray_mat_id = -1, bool check_norm = true, bool check_min = true,
    bool check_max = true, bool is_in_air = false, bool on_ladder = false, TraceMeshFaces *handle = nullptr) const;
  void resetClimbing();
  void performClimb(const ClimbQueryResults &climb_res, float at_time);
  bool canClimbOverObstacle(const ClimbQueryResults &climb_res);

  void drawDebug();

  void setWeaponLen(float len, int slot_id = -1);

  void setStateFlag(HumanPhysState::StateFlag flag, bool enable);
  BBox3 calcBBox() const;
  BBox3 calcBBox(HUStandState cur_state) const;

  TraceMeshFaces *getTraceHandle() const override { return &humanCollision->cachedTraceMeshFaces; }
  float getWalkSpeed(HUStandState ss, HUMoveState ms) const { return walkSpeeds[ss][ms]; }
  void setWalkSpeed(HUStandState ss, HUMoveState ms, float value) { walkSpeeds[ss][ms] = value; }
  float getWishVertJumpSpeed(HUStandState ss, HUMoveState ms) const { return wishVertJumpSpeed[ss][ms]; }

  inline float getMaxObstacleHeight() const { return maxObstacleHeight; }

  const CollisionObject &getTorsoCollision() const { return torsoCollision; }
  const CollisionObject &getClimberCollision() const { return climberCollision; }
  const CollisionObject &getWallJumpCollision() const { return wallJumpCollision; }

  float getLadderClimbSpeed() const { return ladderClimbSpeed; }
};

extern template class PhysicsBase<HumanPhysState, HumanControlState, CommonPhysPartialState>;
