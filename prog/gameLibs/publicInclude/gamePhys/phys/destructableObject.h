//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_smallTab.h>
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

struct DestructableCreationParams
{
  const DynamicPhysObjectData *physObjData = nullptr;
  TMatrix tm = TMatrix::IDENT;
  // set out-of-bounds position, so if camera pos is not set,
  // it would count as being very far
  Point3 camPos = Point3(1e6f, 1e6f, 1e6f);

  int resIdx = -1;
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
    bool keepAlive = false;

    float lifeTime = 0.f;
    float inactiveTime = 0.f;
    float outOfViewTime = 0.f;
    float timeToLive = 0.f;
    float timeToFloat = -1.f;
    float timeToKinematic = 0.f;

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
    void makeKinematic();
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

  void addImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit = 7.f, float omegaLimit = 5.f);
  void setupInitialPhysState(const TMatrix &query_tm, const BBox3 &query_box, const Point3 &pos, const Point3 &impulse);

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

void init(const DataBlock *blk, int fgroup = dacoll::EPL_DEBRIS);
void close();

id_t addDestructable(gamephys::DestructableObject **out_destr, DestructableCreationParams &&params, PhysWorld *phys_world);
id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx = -1, uint32_t hash_val = 0, const DataBlock *blk = nullptr);
void clear();
void removeDestructableById(id_t id);

void update(float dt, const Point3 &view_pos);
void update_floatable(float dt, float cur_time);
dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects();
}; // namespace destructables
