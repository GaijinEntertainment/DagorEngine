// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_staticTab.h>
#include <gamePhys/phys/physActor.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <gamePhys/phys/commonPhysBase.h>
#include <daECS/core/entityId.h>
#include <daECS/core/componentType.h>
#include <EASTL/vector_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/deque.h>
#include <generic/dag_carray.h>
#include "net/netConsts.h"
#include "quantization.h"
#include "physConsts.h"

namespace net
{
class Object;
class IConnection;
class IMessage;
class MessageClass;
} // namespace net

class PhysVars;
namespace danet
{
class BitStream;
class IMessage;
} // namespace danet
class GeomNodeTree;
class CollisionResource;

namespace gamephys
{
struct CollisionContactData;
struct SeqImpulseInfo;
} // namespace gamephys
namespace daphys
{
struct SolverBodyInfo;
}

struct BasePhysSnapshot
{
  float atTime;
  int atTick;
  Point3 pos;
  Point3 vel; // used even in interpolation (for animation's speed & locomotion)
  Quat quat;
  bool operator<(const BasePhysSnapshot &rhs) const { return atTick < rhs.atTick; }
};

enum class PhysSnapSerializeType : uint8_t
{
  CLOSEST,
  NORMAL,
  FARAWAY,
  MIN,
  HIDDEN // Note: this value is not serialized to remote systems
};
constexpr unsigned PST_BITS = 2;

enum class PhysType : uint8_t
{
  HUMAN,
  VEHICLE,
  PHYSOBJ,
  PLANE,
  SHIP,
  DAS_PHYSOBJ,
  HELICOPTER,
  WALKER,

  CUSTOM0,
  CUSTOM1,
  CUSTOM2,
  CUSTOM3,
  CUSTOM4,
  CUSTOM5,
  CUSTOM6,
  CUSTOM7,

  NUM
};

template <typename Phys>
class ControlsSerializer;

class BasePhysActor;
struct FreePhysActorIndex
{
  float availableAtTime;
  unsigned freeIndex; // free index in physEidsList
  bool operator<(const FreePhysActorIndex &f) const { return availableAtTime < f.availableAtTime; }
};
typedef BasePhysActor *(*phys_actor_resolve_by_eid_t)(ecs::EntityId);
typedef eastl::vector<ecs::EntityId> phys_eids_list_t;
typedef eastl::vector<uint16_t> phys_id_gen_list_t;
typedef carray<eastl::deque<FreePhysActorIndex>, 2> free_actors_list_indexes_t; // Naturally sorted by time;
                                                                                // [0] are < 127; [1] are the rest
// This container represent PhysActorSyncState info packed per each player x phys actor (2 bits per actor, i.e. 4 in byte)
typedef eastl::vector<eastl::fixed_vector<uint8_t, 32>> phys_sync_states_packed_t;

struct NetPhysId
{
  int16_t index{};
  uint16_t generation{};
};

// Note: "net" part of name is implied (all physics are assumed to be network-ready)
class BasePhysActor : public IPhysActor
{
  friend class PhysActorIterator;

public:
  struct UpdateContext
  {
    TMatrix *transform; // not null
    bool physObjBroadcastAAS;
    bool humanAnimCharVisible;
  };

  static constexpr const int MAX_COLLISION_SUBOBJECTS = 32;

  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  const PhysType physType;
  PhysTickRateType tickrateType;
  int16_t netPhysId = -1;      // index in physEidsList
  bool clientSideOnly = false; // if true, this Phys Actor exists exclusively on the client

  static StaticTab<phys_eids_list_t *, (int)PhysType::NUM> allPhysActorsLists;
  static phys_sync_states_packed_t *allSyncStates[(int)PhysType::NUM];
  static inline dag::RelocatableFixedVector<ecs::EntityId, 2> locallyContolledClientEntities;

  static void resizeSyncStates(int i);

  int getId() const override final { return (int)(ecs::entity_id_t)eid; }
  ecs::EntityId getEid() const { return eid; }

  const char *getPhysTypeStr() const;

  virtual IPhysBase &getPhys() = 0;
  const IPhysBase &getPhys() const { return const_cast<BasePhysActor *>(this)->getPhys(); }

  static float minTimeStep, maxTimeStep;
  static float getDefTimeStepByTickrateType(PhysTickRateType tt)
  {
    return (tt == PhysTickRateType::Normal) ? BasePhysActor::minTimeStep : BasePhysActor::maxTimeStep;
  }

  virtual void setRoleAndTickrateType(IPhysActor::NetRole nr, PhysTickRateType trtype) = 0;

  int calcControlsTickDelta();

  virtual void postPhysUpdate(int32_t tick, float dt, bool is_for_real) override final;

  virtual void sendDesyncData(const danet::BitStream &sync_cur_data);

  BasePhysActor(ecs::EntityId eid_, PhysType phys_type_);

  virtual void enqueueCT(float at_time) = 0;

  virtual void savePhysSnapshot(BasePhysSnapshot &&snap, float additional_delay = 0.f) = 0;

  void updateTransform(const TMatrix &tm);
  virtual void teleportTo(const TMatrix &tm, bool hard_teleport) = 0;
  virtual void reset() = 0;

  virtual int getLastAASTick() const = 0; // return negative if AASes wasn't received yet
  virtual void resetAAS() = 0;

  virtual bool isHumanPlayer() const { return true; }
  virtual bool isControlledHero() const { return true; }

  virtual bool isAsleep() const = 0;

  void initNetPhysId(phys_eids_list_t &eidsList,
    phys_id_gen_list_t &physIdGenerations,
    free_actors_list_indexes_t &freeIndexes,
    phys_sync_states_packed_t &playersSyncStates,
    int *net__physId);

  virtual void onCollisionDamage(dag::Span<gamephys::SeqImpulseInfo> collisions,
    dag::Span<gamephys::CollisionContactData> contacts,
    int offender_id,
    double collision_impulse_damage_multiplier,
    float at_time) = 0;

  template <typename T>
  T *cast()
  {
    return T::staticPhysType == physType ? static_cast<T *>(this) : nullptr;
  }
  template <typename T>
  const T *cast() const
  {
    return T::staticPhysType == physType ? static_cast<const T *>(this) : nullptr;
  }

  static void registerPhys(PhysType s,
    const char *pname,
    const char *cname_str,
    ecs::component_t cname,
    ecs::component_type_t ctype,
    size_t type_sz,
    size_t snapshot_size,
    phys_id_gen_list_t *id_gen,
    free_actors_list_indexes_t *free_aidx,
    phys_sync_states_packed_t *ss_p,
    void (*snapshot_cache_reset_cb)(),
    const net::MessageClass *msg_cls,
    void (*msg_cb)(const net::IMessage *msg));
};

BasePhysActor *get_phys_actor(ecs::EntityId eid);

class PhysActorIterator
{
public:
  BasePhysActor *next()
  {
    if (j >= BasePhysActor::allPhysActorsLists.size())
      return nullptr;
    while (1)
    {
      if (!BasePhysActor::allPhysActorsLists[j] || ++i >= BasePhysActor::allPhysActorsLists[j]->size())
      {
        if (++j >= BasePhysActor::allPhysActorsLists.size())
          break;
        i = -1;
        continue;
      }
      if (BasePhysActor *ret = get_phys_actor((*BasePhysActor::allPhysActorsLists[j])[i]))
        return ret;
    }
    return nullptr;
  }

private:
  int j = 0, i = -1;
};

struct DummyCustomPhys
{
  DummyCustomPhys() = default;
  DummyCustomPhys(IPhysBase &) {}
  static inline void initOnce() {}
  static inline void onPreInit(ecs::EntityId /*eid*/, BasePhysActor &) {}
  static inline void onPostInit(ecs::EntityId /*eid*/, BasePhysActor &) {}
  static inline void onPreUpdate(float, const BasePhysActor::UpdateContext &) {}
  static inline void onPostUpdate(float, const BasePhysActor::UpdateContext &) {}
  static inline void onCollisionDamage(ecs::EntityId /*eid*/,
    BasePhysActor & /*act*/,
    dag::Span<gamephys::SeqImpulseInfo> /*collisions*/,
    dag::Span<gamephys::CollisionContactData> /*contacts*/,
    int /*offender_id*/,
    double /*collision_impulse_damage_multiplier*/,
    float /*at_time*/)
  {}
  dag::Span<NetWeapon *> getAllWeapons() const { return {}; }
  NetAutopilot *getAutopilot() const { return NULL; }
  static float aircraftCollisionImpulseSpeedMax;
};

template <typename Phys>
class SelectPhysSnapshotType
{
  template <typename U>
  static typename U::SnapshotType test(typename U::SnapshotType *);
  template <typename U>
  static BasePhysSnapshot test(...);

public:
  typedef decltype(test<Phys>(0)) type;
};

template <typename Phys>
class SelectPhysCachedQuantizedInfo
{
  template <typename U>
  static typename U::CachedQuantizedInfo test(typename U::CachedQuantizedInfo *);
  template <typename U>
  static CachedQuantizedInfoT<> test(...);

public:
  typedef decltype(test<Phys>(0)) type;
};

template <typename PhysImpl, typename CustomPhys, PhysType phys_type>
class PhysActor final : public BasePhysActor
{
public:
  typedef typename SelectPhysSnapshotType<CustomPhys>::type SnapshotType;
  typedef typename SelectPhysCachedQuantizedInfo<CustomPhys>::type CachedQuantizedInfo;
  static constexpr PhysType staticPhysType = phys_type;

  typedef CustomPhys CustomPhysType;

  struct SnapshotPair
  {
    SnapshotType prevSnap;
    SnapshotType lastSnap;
  };

  PhysImpl phys;
  CustomPhysType customPhys;
  using SnapshotsHistoryType = eastl::vector_set<SnapshotType,
    eastl::less<SnapshotType>,
    EASTLAllocatorType,
    eastl::fixed_vector<SnapshotType, PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY, /*overflow*/ true>>;
  union
  {
    CachedQuantizedInfo cachedQuantInfo;   // Server-only var
    SnapshotsHistoryType snapshotsHistory; // Client-only var
  };
  static phys_eids_list_t physEidsList;
  static phys_id_gen_list_t physIdGenerations;
  static free_actors_list_indexes_t freeActorIndexes;
  static phys_sync_states_packed_t syncStatesPacked;

  // daecs reg
  PhysActor(const ecs::EntityManager &mgr, ecs::EntityId eid);
  bool onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid);
  ~PhysActor();

  void update(float at_time, float remote_time, float dt, const BasePhysActor::UpdateContext &uctx);
  void updateNetPhys(IPhysActor::NetRole my_role, float at_time);
  virtual void initRole() const override final;
  bool beforeEnqueueCT(float /*at_time*/) { return true; }
  void afterEnqueueCT() {}
  void doEnqueueCT(float at_time);
  virtual void enqueueCT(float at_time) override final { doEnqueueCT(at_time); }

  void setRoleAndTickrateType(IPhysActor::NetRole nr, PhysTickRateType trtype) override;

  const SnapshotPair *findPhysSnapshotPair(float at_time) const;

  // Returns true if phys actor in motion
  bool serializePhysSnapshotBase(danet::BitStream &bs, PhysSnapSerializeType pst) const;
  void serializePhysSnapshotImpl(danet::BitStream &bs, PhysSnapSerializeType pst) const { serializePhysSnapshotBase(bs, pst); }
  static bool deserializeBasePhysSnapshot(const danet::BitStream &from,
    PhysSnapSerializeType pst,
    BasePhysSnapshot &snap,
    net::IConnection &conn,
    bool *in_motion = nullptr,
    bool *out_quat_ext2_bit = nullptr);
  static bool deserializePhysSnapshot(
    const danet::BitStream &from, PhysSnapSerializeType pst, BasePhysSnapshot &snap, net::IConnection &conn)
  {
    return deserializeBasePhysSnapshot(from, pst, snap, conn);
  }

  static void processSnapshotNoEntity(uint16_t netPhysId, BasePhysSnapshot &&basesnap);

  const SnapshotPair *updateRemoteShadowBase(float remote_time, float dt, const BasePhysActor::UpdateContext &uctx);
  void updateRemoteShadow(float remote_time, float dt, const BasePhysActor::UpdateContext &uctx)
  {
    updateRemoteShadowBase(remote_time, dt, uctx);
  }
  void savePhysSnapshot(BasePhysSnapshot &&snap, float additional_delay = 0.f) override;

  int getLastAASTick() const override
  {
    if (phys.lastProcessedAuthorityApprovedStateAtTick < 0)
      return -1;
    G_ASSERT(phys.authorityApprovedState);
    return eastl::max(phys.authorityApprovedState->atTick, phys.authorityApprovedPartialState.atTick);
  }

  void resetAAS() override;

  IPhysBase &getPhys() override { return phys; }
  const IPhysBase &getPhys() const { return phys; }

  void serializePhysSnapshot(danet::BitStream &to, PhysSnapSerializeType pst) const { serializePhysSnapshotImpl(to, pst); }

  void loadPhysFromBlk(const DataBlock *blk, const CollisionResource *coll_res, const char *blk_name)
  {
    phys.loadFromBlk(blk, coll_res, blk_name, true);
  }


  virtual bool isAsleep() const override { return phys.isAsleep(); }

  virtual void calcPosVelQuatAtTime(double /*at_time*/, DPoint3 & /*out_pos*/, Point3 & /*out_vel*/, Quat & /*out_quat*/) const;
  virtual DPoint3 calcPosAtTime(double /*at_time*/) const;
  virtual Quat calcQuatAtTime(double /*at_time*/) const;

  void teleportToDefaultImpl(const TMatrix &tm);
  virtual void teleportTo(const TMatrix &tm, bool) override { teleportToDefaultImpl(tm); }
  virtual void reset() override;

  virtual void onCollisionDamage(dag::Span<gamephys::SeqImpulseInfo> collisions,
    dag::Span<gamephys::CollisionContactData> contacts,
    int offender_id,
    double collision_impulse_damage_multiplier,
    float at_time) override;

  virtual dag::ConstSpan<NetWeapon *> getAllWeapons() const override;
  virtual NetAutopilot *getAutopilot() const override;
  static const net::MessageClass *getMsgCls();
  static void net_rcv_snapshots(const net::IMessage *msg);
};

struct PhysUpdateCtx
{
  static PhysUpdateCtx ctx;
  carray<int, (int)PhysTickRateType::Last + 1> interpDelayTicks;
  float additionalInterpDelay = 0.f;
  float getInterpDelay(PhysTickRateType tt, float ts) const { return interpDelayTicks[(int)tt] * ts + additionalInterpDelay; }
  int getCurInterpDelayTicksPacked() const { return (interpDelayTicks[1] << 4) | interpDelayTicks[0]; }
  void unpack(int interp_delay_ticks_packed)
  {
    interpDelayTicks[0] = clamp(interp_delay_ticks_packed & 15, PHYS_MIN_INTERP_DELAY_TICKS, PHYS_MAX_INTERP_DELAY_TICKS);
    interpDelayTicks[1] = clamp(interp_delay_ticks_packed >> 4, PHYS_MIN_INTERP_DELAY_TICKS, PHYS_MAX_INTERP_DELAY_TICKS);
  }
  void update();
  void reset()
  {
    mem_set_0(interpDelayTicks);
    additionalInterpDelay = 0.f;
  }
};


void teleport_phys_actor(ecs::EntityId eid, const TMatrix &tm);
void reset_phys_actor(ecs::EntityId eid);
void phys_enqueue_controls(float at_time);
void net_send_phys_snapshots(float cur_time, float dt);
typedef bool (*deserialize_snapshot_cb_t)(const danet::BitStream &, PhysSnapSerializeType, BasePhysSnapshot &, net::IConnection &);
typedef void(process_snapshot_no_entity_cb_t)(uint16_t netPhysId, BasePhysSnapshot &&);
void net_rcv_phys_snapshots(const net::IMessage &msg,
  bool to_msg_sink,
  const danet::BitStream &bs,
  net::IConnection &conn,
  phys_eids_list_t &eids_list,
  phys_actor_resolve_by_eid_t resolve_eid_cb,
  deserialize_snapshot_cb_t deserialize_snap_cb,
  process_snapshot_no_entity_cb_t &snapshot_no_entity_cb);
void cleanup_phys();
void clear_all_phys_actors_lists();
ecs::EntityId find_closest_net_phys_actor(const Point3 &pos);

int calc_phys_update_to_tick(float at_time, float dt, int ctrl_tick, int ct_produced_at_tick, int last_aas_tick);

struct PairCollisionData
{
  bool havePairCollision;
  const ecs::string *pairCollisionTag;
  bool isHuman;
  bool isAirplane;
  bool isKinematicBody;
  bool hasCustomMoveLogic;
  bool humanAdditionalCollisionChecks;
  bool nphysPairCollision;
  float maxMassRatioForPushOnCollision;
  int collisionMaterialId;
  int ignoreMaterialId;
};
void query_pair_collision_data(ecs::EntityId eid, PairCollisionData &data);

typedef BasePhysActor *BasePhysActorPtr;
ECS_DECLARE_TYPE(BasePhysActorPtr);

#define ECS_DECLARE_PHYS_ACTOR_TYPE(T)  ECS_DECLARE_BOXED_TYPE(T)           //== for base_net_phys_ptr
#define ECS_REGISTER_PHYS_ACTOR_TYPE(T) ECS_REGISTER_BOXED_TYPE(T, nullptr) //== for base_net_phys_ptr
