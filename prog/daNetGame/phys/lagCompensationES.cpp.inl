// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lagCompensation.h"
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <animChar/dag_animCharacter2.h>
#include <ecs/core/entityManager.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_geomTree.h>
#include <gamePhys/phys/utils.h>
#include <util/dag_convar.h>
#include <math/dag_Quat.h>
#include "game/player.h"
#include "phys/netPhys.h"
#include "phys/physUtils.h"
#include "phys/gridCollision.h"
#include "net/net.h"

// To consider: replace this to convars
#define MAX_UNLAG_TIME_SEC \
  (PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC + BasePhysActor::maxTimeStep * PHYS_MAX_INTERP_DELAY_TICKS + 0.25f)

#if DAGOR_DBGLEVEL > 1
#define SLOW_ASSERT G_ASSERT
#else
#define SLOW_ASSERT
#endif

enum LCFlags
{
  LC_ALIVE = 1 << 0,
  LC_PHYS_ASLEEP = 1 << 1
};

enum LagRecordType
{
  LR_QUANTIZE,
  LR_TRANSFORM,
  LR_POS_QUAT
};

template <LagRecordType>
struct LagRecordLoc;
template <>
struct LagRecordLoc<LR_QUANTIZE>
{
  QuantizedWorldLocDefault quantLoc;

  void getPosQuat(Point3 &p, Quat &q) const { eastl::tie(p, q, eastl::ignore) = quantLoc.unpackTuple(); }
  void setPosQuat(const Point3 &p, const Quat &q) { quantLoc = QuantizedWorldLocDefault(p, q, false); }

  void setLocFrom(const BasePhysActor &actor)
  {
    quantLoc = QuantizedWorldLocDefault(actor.getPhys().getCurrentStateLoc(), /*in_motion*/ false);
  }
  TMatrix makeTM() const { return quantLoc.unpackTM(); }
};
template <>
struct LagRecordLoc<LR_TRANSFORM>
{
  TMatrix mTm;

  // use attribute instead of 'visualLocation' as former might be a little bit different due to various external subsytems
  // (visual_entity_offset, locomotion...)
  void setLocFrom(const BasePhysActor &actor) { mTm = g_entity_mgr->getOr(actor.getEid(), ECS_HASH("transform"), TMatrix::IDENT); }
  const TMatrix &makeTM() const { return mTm; }
};
template <>
struct LagRecordLoc<LR_POS_QUAT>
{
  Point3 pos;
  Quat ori;

  void getPosQuat(Point3 &p, Quat &q) const
  {
    p = pos;
    q = ori;
  }
  void setPosQuat(const Point3 &p, const Quat &q)
  {
    pos = p;
    ori = q;
  }

  void setLocFrom(const BasePhysActor &actor)
  {
    auto &phys = actor.getPhys();
    const gamephys::Loc &loc = phys.getCurrentStateLoc();
    pos = Point3::xyz(loc.P);
    ori = loc.O.getQuat();
  }
  TMatrix makeTM() const
  {
    TMatrix tm = ::makeTM(ori);
    tm.setcol(3, pos);
    return tm;
  }
};

struct LagRecordHdr
{
  float atTime = -1.f;
  int flags = 0;
};

// Represents position data of one entity for speicifc tick
template <LagRecordType lrtype>
struct LagRecord : public LagRecordHdr, public LagRecordLoc<lrtype>
{
  bool isValid() const { return atTime >= 0; }
  bool isAlive() const { return (flags & LC_ALIVE) != 0; }
  bool isPhysAsleep() const { return (flags & LC_PHYS_ASLEEP) != 0; }

  bool operator<(const LagRecord &rhs) const { return atTime < rhs.atTime; }

  static LagRecord make(const BasePhysActor &physActor, float at_time)
  {
    ecs::EntityId peid = physActor.getEid();

    LagRecord ret;
    ret.atTime = at_time;
    ret.flags = (ECS_GET_OR(peid, isAlive, false) ? LC_ALIVE : 0) | (physActor.isAsleep() ? LC_PHYS_ASLEEP : 0);
    ret.setLocFrom(physActor);

    G_ASSERT(ret.isValid());
    return ret;
  }

  void apply(BasePhysActor &physActor) const
  {
    G_ASSERT(isValid());
    G_ASSERTF(isAlive(), "%d<%s> isAlive mismatch %d != %d", (ecs::entity_id_t)physActor.eid,
      g_entity_mgr->getEntityTemplateName(physActor.eid), isAlive(), g_entity_mgr->getOr(physActor.eid, ECS_HASH("isAlive"), false));

    const TMatrix &tm = this->makeTM();

    physActor.updateTransform(tm);
  }

  static LagRecord<LR_POS_QUAT> interpolate(const LagRecord &a, const LagRecord &b, float at_time)
  {
    float alpha = (at_time - a.atTime) / float(b.atTime - a.atTime);
    G_ASSERTF(alpha > -1e-6f && alpha < (1.f + 1e-6f), "%f not in range [%f..%f]", at_time, a.atTime, b.atTime);

    Point3 pa, pb;
    Quat qa, qb;
    a.getPosQuat(pa, qa);
    b.getPosQuat(pb, qb);

    LagRecord<LR_POS_QUAT> ret;
    static_cast<LagRecordHdr &>(ret) = static_cast<const LagRecordHdr &>(alpha < 0.5f ? a : b);
    ret.setPosQuat(lerp(pa, pb, alpha), normalize(qinterp(qa, qb, alpha))); // TODO: use same exact code as in interpolation
                                                                            //       for visualization

    return ret;
  }
};

// This type respresents positional history of particular entity
// Note: can't use quantized data here if non quantized physics exist
typedef eastl::vector<LagRecord<LR_POS_QUAT>> EntityLagTrack;

bool is_need_to_lag_compensate() { return PHYS_ENABLE_INTERPOLATION && is_true_net_server(); }
float lag_compensation_time(ecs::EntityId avatar_eid,
  ecs::EntityId lc_eid,
  float at_time,
  int interp_delay_ticks_packed,
  float additional_interp_delay,
  BasePhysActor **out_lc_eid_phys_actor)
{
  if (!is_need_to_lag_compensate())
    return NO_LAG_COMPENSATION_TIME;
  const game::Player *plr = game::find_player_that_possess(avatar_eid);
  if (!plr || !plr->getConn())
    return NO_LAG_COMPENSATION_TIME;
  PhysUpdateCtx ctx;
  if (additional_interp_delay > 0.f) // this also handles NaN
    ctx.additionalInterpDelay = eastl::min(additional_interp_delay, PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC);
  ctx.unpack(interp_delay_ticks_packed);
  BasePhysActor *actor = get_phys_actor(lc_eid);
  if (out_lc_eid_phys_actor)
    *out_lc_eid_phys_actor = actor;
  // To consider: get timeStep from client or from LC history (as it might change)
  float ts = actor ? actor->getPhys().getTimeStep() : BasePhysActor::minTimeStep;
  PhysTickRateType tt = actor ? actor->tickrateType : PhysTickRateType::Normal;
  return at_time - ctx.getInterpDelay(tt, ts);
}

class CLagCompensationManager final : public ILagCompensationMgr
{
  eastl::vector_map<ecs::EntityId, EntityLagTrack> entityTrack; // entity handle -> entity's track
                                                                // (natually sorted from oldest to newest)
  eastl::vector_map<ecs::EntityId, EntityLagTrack> entityTrackTmp;
  eastl::vector<EntityLagTrack> freeTracks;
  eastl::vector_map<ecs::EntityId, LagRecord<LR_TRANSFORM>> restoreRecords;

public:
  void startLagCompensation(float to_time, ecs::EntityId except_eid) override
  {
    if (to_time == NO_LAG_COMPENSATION_TIME)
      return;
    for (auto it = entityTrack.cbegin(), ite = entityTrack.cend(); it != ite; ++it)
      if (it->first != except_eid)
        backtrackEntity(it, to_time);
  }

  void finishLagCompensation() override
  {
    for (auto &resPair : restoreRecords)
    {
      auto physActor = get_phys_actor(resPair.first);
      if (physActor)
        resPair.second.apply(*physActor);
    }
    restoreRecords.clear();
  }


  LCError backtrackEntity(ecs::EntityId eid, float to_time) override
  {
    auto pit = entityTrack.find(eid);
    return (pit == entityTrack.end()) ? LCError::UnknownEntity : backtrackEntity(pit, to_time);
  }

  void saveForRestore(ecs::EntityId eid, const BasePhysActor &actor)
  {
    auto restoreIt = restoreRecords.find(eid);
    if (restoreIt == restoreRecords.end())
      restoreRecords.emplace(eid, decltype(restoreRecords)::mapped_type::make(actor, 0.f));
  }

  LCError backtrackEntity(eastl::vector_map<ecs::EntityId, EntityLagTrack>::const_iterator pit, float at_time)
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(pit != entityTrack.cend());
#endif
    auto physActor = get_phys_actor(pit->first);
    if (!physActor)
      return LCError::NoPhysActor;
    ecs::EntityId eid = pit->first;
    if (!ECS_GET_OR(eid, isAlive, false))
      return LCError::EntityIsDead;
    const EntityLagTrack &track = pit->second;
    G_ASSERT(!track.empty()); // guaranteed by 'decayOldHistory'

    if (track.back().flags & LC_PHYS_ASLEEP)
    {
      saveForRestore(eid, *physActor);
      track.back().apply(*physActor);
      return LCError::Ok;
    }

    EntityLagTrack::value_type const *prevRec = nullptr;
    for (auto rit = track.rbegin(), rend = track.rend(); rit != rend; ++rit) // search for lower bound (from newest to oldest)
    {
      if (!rit->isAlive())
        return LCError::EntityIsDead; // don't try to compensate death
      if (rit->atTime <= at_time)
      {
        prevRec = &*rit;
        break;
      }
    }
    EntityLagTrack::value_type const *rec = prevRec ? (prevRec + 1) : track.end();
    for (; rec != track.end(); ++rec)
      if (rec->atTime > at_time)
        break;
    if (rec == track.end())
    {
      logwarn("can't lag compensate entity %d to time %f, history [%f..%f]", (ecs::entity_id_t)eid, at_time, track.front().atTime,
        track.back().atTime);
      return LCError::TimeNotFound;
    }
    G_ASSERT(*prevRec < *rec);
    if ((rec->atTime - prevRec->atTime) > 3.f * physActor->getPhys().getTimeStep()) //-V1004 don't try interpolate from too old state
    {
      logwarn("can't lag compensate entity %d to time %f, time gap in history [%f..%f] is too wide", (ecs::entity_id_t)eid, at_time,
        prevRec->atTime, rec->atTime);
      return LCError::TimeGapIsTooBig;
    }

    saveForRestore(eid, *physActor);
    EntityLagTrack::value_type::interpolate(*prevRec, *rec, at_time).apply(*physActor);

    return LCError::Ok;
  }

  void captureEntities(float)
  {
    entityTrackTmp.clear();
    int nTracksMoved = 0;
    PhysActorIterator pit;
    while (BasePhysActor *physActor = pit.next())
    {
      auto it = entityTrack.find(physActor->eid);
      EntityLagTrack *track;
      if (it != entityTrack.end()) // already exist in history
      {
        track = &entityTrackTmp.insert(eastl::move(*it)).first->second;
        G_ASSERT(it->second.empty()); // ensure that it's moved
        nTracksMoved++;
      }
      else if (!freeTracks.empty()) // new entity & has free track for reuse
      {
        track = &entityTrackTmp.insert(eastl::make_pair(ecs::EntityId(physActor->eid), eastl::move(freeTracks.back()))).first->second;
        freeTracks.pop_back();
        G_ASSERT(track->capacity());
        track->clear();
      }
      else // new entity and no free tracks for reuse
        track = &entityTrackTmp.insert(physActor->eid).first->second;
      float actorTime = physActor->getPhys().getCurrentTick() * physActor->getPhys().getTimeStep();
      if (EntityLagTrack::value_type *prevRec = track->empty() ? nullptr : &track->back())
      {
        if (prevRec->isPhysAsleep() && physActor->isAsleep()) // Optimization: don't waste memory on static (asleep) phys actors
        {
          if (!prevRec->isAlive() && ECS_GET_OR(physActor->eid, isAlive, false))
            // ...but do update alive flag since technics might get repaired
            prevRec->flags |= LC_ALIVE;
          continue;
        }
        if ((actorTime - prevRec->atTime) < 1e-6f)
          continue;
      }
      track->emplace_back(EntityLagTrack::value_type::make(*physActor, actorTime));
      G_ASSERT(track->size() < 1024); // sanity check
    }
    entityTrack.swap(entityTrackTmp);
    if (int nTracksLeft = entityTrackTmp.size() - nTracksMoved) // there are some entities that wasn't moved (i.e. was destroyed)?
    {
      for (auto &trec : entityTrackTmp)
        if (!trec.second.empty())
        {
          freeTracks.push_back(eastl::move(trec.second));
          if (--nTracksLeft == 0)
            break;
        }
    }
  }

  void decayOldHistory(float at_time)
  {
    float minTime = at_time - MAX_UNLAG_TIME_SEC;
    for (auto it = entityTrack.begin(); it != entityTrack.end();)
    {
      auto &track = it->second;
      SLOW_ASSERT(eastl::is_sorted(track.begin(), track.end()));
      while (!track.empty() && track.front().atTime < minTime)
      {
        if (!track.front().isPhysAsleep() || track.size() > 1)
          track.erase(track.begin());
        else
          break;
      }
      if (track.empty())
        it = entityTrack.erase(it);
      else
        ++it;
    }
  }

  void update(float at_time)
  {
    if (!PHYS_ENABLE_INTERPOLATION)
      return;
    if (!game::get_num_connected_players())
      return;
    decayOldHistory(at_time);
    captureEntities(at_time);
  }
};
static CLagCompensationManager lag_compensation_mgr;
ILagCompensationMgr &get_lag_compensation() { return lag_compensation_mgr; }

ECS_TAG(net, server)
static void lag_compensation_update_es(const ecs::UpdateStageInfoAct &info) { lag_compensation_mgr.update(info.curTime); }
