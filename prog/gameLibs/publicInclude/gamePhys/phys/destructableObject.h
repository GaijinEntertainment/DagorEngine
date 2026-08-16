//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <util/dag_bitFlagsMask.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <gamePhys/collision/physLayers.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/phys/destructableRendObject.h>
#include <render/dynmodelRenderer/animcharDisintegration.h>


// Fwd declaration
template <typename T>
class DynamicPhysObjectClass;
class PhysWorld;
class DynamicPhysObjectData;
class DataBlock;

namespace frx
{
struct RenderMesh;
}

namespace gamephys
{

struct FracturePhysObject
{
  TMatrix tm, prevTm;
  eastl::unique_ptr<PhysBody> body;
  eastl::unique_ptr<frx::RenderMesh> mesh;

  FracturePhysObject() = default;
  FracturePhysObject(FracturePhysObject &&) = default;
  ~FracturePhysObject();
};

} // namespace gamephys

namespace destructables
{

typedef void *id_t;
static constexpr id_t INVALID_ID = nullptr;
static constexpr int DESTRUCTABLES_DELETE_MAX_PER_FRAME = 10;

enum class InteractionFlag : uint8_t
{
  None = 0,
  Static = 1 << 0,     // statics, terrain, rendinsts
  Self = 1 << 1,       // other destructable pieces
  Character = 1 << 2,  // characters, vehicles, ragdolls
  Projectile = 1 << 3, // projectile traces & interactions, TODO: split into 2 separate flags

  All = Static | Self | Character | Projectile
};
using InteractionFlags = BitFlagsMask<InteractionFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(InteractionFlag);

struct PieceRef
{
  id_t id = INVALID_ID;
  int gen = 0;
  int pieceIdx = -1;
};

struct TraceHit
{
  PieceRef ref;
  float t = 0.f;
  Point3 pos = Point3::ZERO;
  Point3 normal = Point3::ZERO;
  int physMatId = -1;
};

using TraceHitList = dag::RelocatableFixedVector<TraceHit, 2, true, framemem_allocator>;

struct DestructableCreationParams
{
  const DynamicPhysObjectData *physObjData = nullptr;
  TMatrix tm = TMatrix::IDENT;
  // set out-of-bounds position, so if camera pos is not set,
  // it would count as being very far
  Point3 camPos = Point3(1e6f, 1e6f, 1e6f);

  int resIdx = -1;
  int riPhysMatId = -1;
  uint32_t hashVal = 0;

  float timeToLive = -1.0f;
  float inactiveTimeBeforeSink = -1.0f;
  float timeToKinematic = -1.0f;
  float timeToSinkUnderground = -1.0f;
  float timeToStartDisintegration = -1.0f;
  float disintegrationDuration = -1.0f;
  float disintegrationScale = -1.0f;
  bool isDestroyedByExplosion = false;

  dag::Vector<gamephys::FracturePhysObject> fracturePhysObjects;
};
} // namespace destructables

template <>
struct BitFlagsTraits<destructables::InteractionFlag>
{
  static constexpr auto allFlags = destructables::InteractionFlag::All;
};

namespace gamephys
{

struct DestructableObjectAddImpulse;
class DestructableObject
{
public:
  int gen; // Note: intentionally not inited

private:
  friend struct gamephys::DestructableObjectAddImpulse;
  float inactiveTimeBeforeSink = 3.f;
  float timeToSinkUnderground = 3.f;
  float minInteractiveTime = 2.f;
  float scaleDt;
  bool floatEnabled = false;
  float disintegrationTime = 0.0f;
  animchar_disintegration::DisintegrationParameters disintegrationParameters;
  void doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit);

public:
  struct Piece
  {
    // not owned, when null, piece is considered dead
    PhysBody *body = nullptr;

    static constexpr int VISIBLE_FRAMES_THRESHOLD = 10;
    int visibleFrames = VISIBLE_FRAMES_THRESHOLD;
    bool isSentinelBody = false;
    bool isInteractiveDebris = false;
    destructables::InteractionFlags interactionFlags = destructables::InteractionFlag::Static;
    float elevationAboveGround = FLT_MAX;
    float boundingRadSq = 0.f;

    float lifeTime = 0.f;
    float inactiveTime = 0.f;
    float outOfViewTime = 0.f;
    float timeToLive = 0.f;
    float timeToFloat = -1.f;

    gamephys::Loc visualLoc;
    gamephys::Loc prevLoc;
    gamephys::Loc visualLocErr;
    Point3 posErrVel = Point3::ZERO;
    Point3 rotErrVel = Point3::ZERO;

    BBox3 localPhysBBox;

    bool isAlive() const { return body != nullptr; }
    bool isInteractable() const;
    void addImpulse(const Point3 &pos, const Point3 &impulseDir, float impulseLen, float speedLimit, float omegaLimit);
    void updateFloatable(float dt, float at_time);
    void update(DestructableObject &parent, float dt, float scaled_dt, bool force_inactive_timer);
    void setInteractionFlags(destructables::InteractionFlags flags);
    void changeInteractionFlags(destructables::InteractionFlags flags, bool enable);
    void destroy();
  };
  dag::Vector<Piece> pieces;
  int alivePieceCnt = 0; // < 0 - marked for delete

  eastl::unique_ptr<DynamicPhysObjectClass<PhysWorld>> physObj;
  eastl::unique_ptr<destructables::DestrRendData, destructables::DestrRendDataDeleter> rendData;
  dag::Vector<FracturePhysObject> fracturePhysObjects;
  int resIdx;
  Point4 intialTmAndHash[4]; // row0-2 - initialTm(43), row3 = hash

  DestructableObject(destructables::DestructableCreationParams &&params, PhysWorld *phys_world, float scale_dt);
  bool update(float dt, float cur_dt_scale, bool force_inactive_timer); // return false if it need to be destroyed
  Point4 getDisintegrationParams() const;
  [[nodiscard]] bool hasDisintegrationAnimation() const;

  destructables::id_t getId() const { return (destructables::id_t)this; }

  void setupInitialPhysState(const TMatrix &query_tm, const BBox3 &query_box, const Point3 &pos, const Point3 &impulse);
  void applyInitialImpulse(PhysWorld &pw, const TMatrix &query_tm, const BBox3 &query_box, const Point3 &pos, const Point3 &impulse);
  void addImpulseSimple(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speed_limit = 7.f, float omega_limit = 5.f);

  bool isFloatEnabled() const { return floatEnabled; }
  void setTimeToFloat(float time);
  bool isAlive() const { return alivePieceCnt > 0; }
  void markForDelete() { alivePieceCnt = -1; }

  int getBodyCount() const { return pieces.size(); }
  PhysBody *getBody(int i) const { return pieces[i].body; }
  DynamicPhysObjectClass<PhysWorld> *getPhysObj() { return physObj.get(); }
};
}; // namespace gamephys


namespace destructables
{
struct DestructablesConfig
{
  int maxNumberOfDestructableBodies = 100;
  int numOfDestrBodiesForScaleDt = 50; // negative - disabled
  float distForScaleDtSq = sqr(100.f); // <= 0 - disabled
  float maxScaleDt = 3.f;
  float minDestrRadiusSq = sqr(40.f);

  struct OutOfViewDisappear
  {
    IPoint2 softHardBodyLimit = IPoint2(50, 100);
    Point2 minSizeTime = Point2(5.f, 0.f);   // smallest pieces disappear after this long out of view
    Point2 maxSizeTime = Point2(30.f, 10.f); // pieces of maxRelSize disappear after this long
    Point2 maxRelSize = Point2(0.1f, 0.4f);
  } outOfViewDisappear;

  struct DestructionBurst
  {
    // baseline values and scaling
    float impactSpeed = 13.f;
    float radialSpeed = 3.5f;
    float directVelFalloff = 0.7f;
    float radialVelBaseRadius = 0.5f;
    float maxDirectVelBias = 2.f;
    float penetrationBase = 7.5f;
    float penetrationMax = 12.5f;
    float tumbleFactor = 1.f;

    // intensity scaling
    float refImpulse = 2000.f;
    float impulseIntensityGain = 0.5f;
    float impulseIntensityExponent = 0.5f;
    float maxIntensity = 2.f;

    // velocity field sampling
    float samplingScale = 1.f;
    int maxSamplesPerAxis = 8;
    float minSampleSpread = 0.5f;
    float linVelSampleWeightExp = 2.f;

    // noise & jitter
    float noiseAmplitude = 2.5f;
    float noiseScale = 2.f;
    float noiseLateralFrac = 0.25f;
    float smallPieceRandomOmega = 6.f;
    float smallOmegaSizeThreshold = 0.5f;

    // limits
    float hardMaxSpeed = 40.f;
    float hardMaxOmega = 10.f;
    float maxTipSpeed = 8.f;
  } destructionBurst;
  bool simpleInitialImpulse = true;

  InteractionFlags enabledInteractionFlags = InteractionFlag::Static | InteractionFlag::Character;
  float interactiveDebrisMinSize = VERY_BIG_NUMBER;
  float characterCollisionDist = 0.f;
  float characterCollisionDistHysteresis = 10.f;
  float minInteractiveHeight = 0.4f;
  float keepAliveMaxHeightAboveGround = 0.f;

  float visualErrorSmoothTime = 3.f;
  float visualErrorAccumulateTime = 1.f;

  int defaultFGroup = dacoll::EPL_DEFAULT;
  int defaultFMask = dacoll::EPL_ALL;

  bool warnOnBodiesOverflow = true;
  bool errorOnBodiesOverflow = false;
  unsigned minBodyCountForOverflowError = 10;

  void loadFromBlk(const DataBlock *destr_blk);
};


const DestructablesConfig &get_config();
DestructablesConfig &get_mutable_config();

void init(const DataBlock *blk);
void close();

id_t addDestructable(gamephys::DestructableObject **out_destr, DestructableCreationParams &&params, PhysWorld *phys_world);
id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx = -1, uint32_t hash_val = 0, const DataBlock *blk = nullptr);
void clear();
void removeDestructableById(id_t id);

void trace_ray(const Point3 &from, const Point3 &dir, float max_t, TraceHitList &out_hits);

void update(float dt, float cur_time, const Point3 &view_pos);
void update_floatable(float dt, float cur_time);
dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects();
}; // namespace destructables
