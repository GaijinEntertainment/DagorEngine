// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riDestrInternal.h"
#include "netDestrPhys.h"

#include <daNet/bitStream.h>
#include <phys/dag_physics.h>
#include <gamePhys/phys/destructableObject.h>
#include <debug/dag_debug3d.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactData.h>
#include <gameMath/quantization.h>


namespace convar
{
static CONSOLE_BOOL_VAL("net_destr_phys", debug_draw, false);
static CONSOLE_FLOAT_VAL("net_destr_phys", hard_snap_dist, 0.1f);
static CONSOLE_FLOAT_VAL("net_destr_phys", hard_snap_vel_rel, 0.4f);
static CONSOLE_FLOAT_VAL("net_destr_phys", no_correction_vel_rel, 0.1f);
static CONSOLE_FLOAT_VAL("net_destr_phys", soft_correction_lerp, 0.75f);
static CONSOLE_FLOAT_VAL("net_destr_phys", omega_correction_max_vel, 1.f);
} // namespace convar


namespace destructables
{

DestrNetPhys::PhysBodyState DestrNetPhys::PhysBodyState::add(const PhysBodyState &A, const PhysBodyState &B)
{
  PhysBodyState s;
  s.loc.add(A.loc, B.loc);
  s.vel = A.vel + B.vel;
  s.omega = A.omega + B.omega;
  return s;
}

DestrNetPhys::PhysBodyState DestrNetPhys::PhysBodyState::subtract(const PhysBodyState &A, const PhysBodyState &B)
{
  PhysBodyState s;
  s.loc.substract(A.loc, B.loc);
  s.vel = A.vel - B.vel;
  s.omega = A.omega - B.omega;
  return s;
}

DestrNetPhys::PhysBodyState DestrNetPhys::PhysBodyState::interpolate(const PhysBodyState &A, const PhysBodyState &B, float t)
{
  PhysBodyState s;
  s.loc.interpolate(A.loc, B.loc, t);
  s.vel = lerp(A.vel, B.vel, t);
  s.omega = lerp(A.omega, B.omega, t);
  return s;
}

void DestrNetPhys::PhysBodyState::serialize(danet::BitStream &bs) const
{
  bs.Write(Point3::xyz(loc.P));
  gamemath::QuantizedQuat38 orient = loc.O.getQuat();
  const bool isAsleep = vel == Point3::ZERO && omega == Point3::ZERO;
  orient.qquat |= uint64_t(isAsleep) << 38;
  bs.WriteBits((const uint8_t *)&orient.qquat, 40);
  if (!isAsleep)
    for (int i = 0; i < 3; i++)
    {
      uint32_t p =
        gamemath::pack_scalar_signed<uint32_t>(vel[i], 12, 50.f) | (gamemath::pack_scalar_signed<uint32_t>(omega[i], 12, 32.f) << 12);
      bs.WriteBits((const uint8_t *)&p, 24);
    }
}

bool DestrNetPhys::PhysBodyState::deserialize(const danet::BitStream &bs)
{
  bool ok = true;
  Point3 pos = Point3::ZERO;
  ok &= bs.Read(pos);
  loc.P = pos;
  gamemath::QuantizedQuat38 o;
  ok &= bs.ReadBits((uint8_t *)&o.qquat, 40);
  const bool isAsleep = o.qquat & (uint64_t(1) << 38u);
  loc.O.setQuat(o.unpackQuat());
  if (!isAsleep)
    for (int i = 0; i < 3; i++)
    {
      uint32_t p = 0;
      ok &= bs.ReadBits((uint8_t *)&p, 24);
      vel[i] = gamemath::unpack_scalar_signed<uint32_t>(p & 0xFFFu, 12, 50.f);
      omega[i] = gamemath::unpack_scalar_signed<uint32_t>(p >> 12u, 12, 32.f);
    }
  else
    vel = omega = Point3::ZERO;
  return ok;
}

void DestrNetPhys::PiecePhys::record(const PhysBodyState &state, float life_time)
{
  if (histSize == 0 || life_time - lastAtTime >= HISTORY_INTERVAL)
  {
    lastAtTime = histSize > 0 ? max(lastAtTime + HISTORY_INTERVAL, life_time - HISTORY_INTERVAL) : life_time;
    histCur = (histCur + 1) % HISTORY_STATES;
    histSize = min(histSize + 1, HISTORY_STATES);
  }
  history[histCur] = state;
}

static void resync_piece(gamephys::DestructableObject::Piece &piece, DestrNetPhys::PiecePhys &net_piece,
  const DestrNetPhys::PhysBodyState &server)
{
  constexpr float MATCH_VEL_TIME = 0.2f;
  constexpr float PENETRATION_TOLERANCE = 0.03f;
  constexpr int DEBUG_DRAW_FRAMES = 1000000;

  using PhysBodyState = DestrNetPhys::PhysBodyState;

  TMatrix curTm;
  piece.body->getTm(curTm);
  PhysBodyState cur;
  cur.loc.fromTM(curTm);
  cur.vel = piece.body->getVelocity();
  cur.omega = piece.body->getAngularVelocity();
  net_piece.record(cur, piece.lifeTime);

  const auto drawDebug = [&](E3DCOLOR cur_color) {
    if (!convar::debug_draw)
      return;
    TMatrix serverTm;
    server.loc.toTM(serverTm);
    draw_debug_box_buffered(piece.localPhysBBox, serverTm, E3DCOLOR(255, 255, 255), DEBUG_DRAW_FRAMES);
    draw_debug_box_buffered(piece.localPhysBBox, curTm, cur_color, DEBUG_DRAW_FRAMES);
  };

  const auto distSq = [&](const PhysBodyState &a, const PhysBodyState &b) {
    const Point4 qa = a.loc.O.getQuatAsP4(), qb = b.loc.O.getQuatAsP4();
    const float orientDistSq = min((qa - qb).lengthSq(), (qa + qb).lengthSq());
    return float((a.loc.P - b.loc.P).lengthSq()) + orientDistSq * piece.boundingRadSq +
           (a.vel - b.vel).lengthSq() * sqr(MATCH_VEL_TIME);
  };

  int bestLag = 0;
  float bestDistSq = FLT_MAX;
  for (int k = 0; k < net_piece.histSize; k++)
    if (const float d = distSq(net_piece.back(k), server); d < bestDistSq)
      bestDistSq = d, bestLag = k;
  const PhysBodyState best = net_piece.back(bestLag);

  const float correctionThresholdVelTerm = max(server.vel.lengthSq() + server.omega.lengthSq() * piece.boundingRadSq,
    cur.vel.lengthSq() + cur.omega.lengthSq() * piece.boundingRadSq);
  const float hardSnapBaseSq = sqr(convar::hard_snap_dist.get());

  PhysBodyState target;
  TMatrix targetTm;

  if (bestLag < 2 && bestDistSq < hardSnapBaseSq + correctionThresholdVelTerm * sqr(convar::no_correction_vel_rel.get()))
  {
    // no correction
    drawDebug(E3DCOLOR(0, 255, 255));
    return;
  }
  else if (bestDistSq > hardSnapBaseSq + correctionThresholdVelTerm * sqr(convar::hard_snap_vel_rel.get()))
  {
    // hard snap
    target = server;
    target.loc.toTM(targetTm);
    net_piece.histSize = 0;
    net_piece.record(target, piece.lifeTime);
    drawDebug(E3DCOLOR(255, 0, 255));
  }
  else
  {
    // soft correction
    const float lerpK = convar::soft_correction_lerp.get();
    PhysBodyState corr;
    corr.loc.substract(server.loc, best.loc);
    corr.loc.P *= lerpK;
    corr.loc.O.setQuat(normalize(qinterp(Quat(0, 0, 0, 1), corr.loc.O.getQuat(), lerpK)));
    corr.vel = (server.vel - best.vel) * lerpK;
    const Point3 omegaDiff = server.omega - best.omega;
    const bool correctOmega = omegaDiff.lengthSq() * piece.boundingRadSq < sqr(convar::omega_correction_max_vel.get());
    corr.omega = correctOmega ? omegaDiff * lerpK : Point3::ZERO;
    corr.loc.P.y = 0;
    corr.vel.y = 0;

    target = PhysBodyState::add(cur, corr);
    target.loc.toTM(targetTm);
    piece.body->setTm(targetTm);

    {
      Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
      dacoll::test_collision_world(CollisionObject(piece.body), contacts, PHYSMAT_INVALID, dacoll::PhysLayer(dacoll::EPL_DEFAULT),
        dacoll::EPL_STATIC);
      for (const gamephys::CollisionContactData &contact : contacts)
        if (contact.depth < -PENETRATION_TOLERANCE)
        {
          piece.body->setTm(curTm);
          drawDebug(E3DCOLOR(0, 0, 255));
          return;
        }
    }

    for (int k = 0; k < net_piece.histSize; k++)
      net_piece.back(k) = PhysBodyState::add(net_piece.back(k), corr);

    const float lagT = float(bestLag) / DestrNetPhys::HISTORY_STATES;
    drawDebug(E3DCOLOR(int(255.f * lagT), int(255.f * (1.f - lagT)), 0));
  }

  gamephys::Loc addErr;
  addErr.substract(cur.loc, target.loc);
  addErr.O.setQuat(cur.loc.O.getQuat() * inverse(target.loc.O.getQuat()));
  const Quat prevErrO = piece.visualLocErr.O.getQuat();
  piece.visualLocErr.add(piece.visualLocErr, addErr);
  piece.posErrVel += cur.vel - target.vel;
  // rendered spin is err velocity + err-rotated body omega, keep it continuous across the omega change
  piece.rotErrVel += prevErrO * cur.omega - piece.visualLocErr.O.getQuat() * target.omega;
  piece.prevLoc = target.loc;

  piece.body->setTm(targetTm);
  piece.body->setVelocity(target.vel);
  piece.body->setAngularVelocity(target.omega);
  net_piece.record(target, piece.lifeTime);
}


void write_net_snapshot(uint32_t send_no, dag::ConstSpan<Point3> observers,
  dag::FunctionRef<void(int observer_idx, danet::BitStream &&bs)> send_cb)
{
  const int MAX_NET_LOD = 5;
  const float lod0DistSq = sqr(25.f);
  // TODO: implement proper island gather & sync
  danet::BitStream bs;
  for (int observerIdx = 0; observerIdx < observers.size(); observerIdx++)
  {
    const Point3 viewPos = observers[observerIdx];
    bs.Clear();
    uint16_t count = 0;
    uint32_t lodPhase = 0;
    const uint32_t countOfs = bs.GetWriteOffset();
    bs.Write(count);
    for (const gamephys::DestructableObject *destr : destructables::getDestructableObjects())
    {
      if (!destr || !destr->isAlive() || !destr->isNetSynced())
        continue;
      lodPhase++;
      bool anyPieceWritten = false;
      const uint32_t batchStartOfs = bs.GetWriteOffset();
      count++;
      bs.Write(uint8_t(0)); // batch mode - reserved
      bs.Write(destr->netSyncKey);
      uint8_t piecesCurRun = 0;
      uint32_t piecesCurRunOfs = bs.GetWriteOffset();
      bs.Write(uint8_t(0));
      for (uint32_t i = 0; i < destr->pieces.size(); i++)
      {
        auto &piece = destr->pieces[i];
        auto &netPiece = destr->netPhys->pieces[i];
        bool isWritten = piece.isInteractable() && netPiece.histSize > 0;
        int netLod = MAX_NET_LOD;
        if (isWritten)
        {
          // each next lod is twice as far and twice low frequency
          frexp((viewPos - Point3::xyz(piece.prevLoc.P)).lengthSq() / (lod0DistSq * 0.5f), &netLod);
          netLod = max(0, netLod) / 2;
          if (piece.inactiveTime > 3.f) // dont lower the LOD right after it settles
            netLod += 3;
          netLod = min(netLod, MAX_NET_LOD);
          isWritten = ((send_no + lodPhase) & ((1u << uint32_t(netLod)) - 1u)) == 0; // stagger
        }

        if ((isWritten ? 0u : 1u) != (piecesCurRun & 3u) || piecesCurRun >= 0xFCu)
        {
          bs.WriteAt(piecesCurRun, piecesCurRunOfs);
          if (piecesCurRun != 0)
          {
            piecesCurRunOfs = bs.GetWriteOffset();
            bs.Write(uint8_t(0));
          }
          piecesCurRun = uint8_t(!isWritten); // second bit reserved for more states
        }
        piecesCurRun += 4u;
        if (!isWritten)
          continue;

        netPiece.back(0).serialize(bs);
        anyPieceWritten = true;
      }
      if (anyPieceWritten)
      {
        bs.WriteAt(piecesCurRun, piecesCurRunOfs);
        bs.Write(uint8_t(0));
      }
      else
      {
        count--;
        bs.SetWriteOffset(batchStartOfs);
      }
    }
    bs.WriteAt(uint16_t(count), countOfs);
    if (count != 0)
      send_cb(observerIdx, eastl::move(bs));
  }
}


void read_net_snapshot(const danet::BitStream &bs)
{
  using namespace gamephys;

  if (convar::debug_draw)
    clear_buffered_debug_lines();
  uint16_t count = 0;
  bool ok = bs.Read(count);
  for (int i = 0; i < count && ok; i++)
  {
    uint8_t reserved;
    ok &= bs.Read(reserved);
    uint64_t netSyncKey = 0;
    ok &= bs.Read(netSyncKey);
    DestructableObject *destr = nullptr;
    if (ok)
      for (DestructableObject *d : getDestructableObjects())
        if (d->netSyncKey == netSyncKey && d->netPhys)
        {
          destr = d;
          break;
        }
    int pieceId = 0;
    while (true)
    {
      uint8_t pieceRun = 0;
      ok &= bs.Read(pieceRun);
      if (!ok || !pieceRun)
        break;
      if (pieceRun & 3u)
      {
        pieceId += pieceRun >> 2u;
        continue;
      }
      for (int j = 0; j < (pieceRun >> 2u); j++, pieceId++)
      {
        DestrNetPhys::PhysBodyState state;
        ok &= state.deserialize(bs);
        if (!ok || !destr || pieceId >= destr->pieces.size())
          continue;
        auto &piece = destr->pieces[pieceId];
        if (!piece.isInteractable())
          continue;
        resync_piece(piece, destr->netPhys->pieces[pieceId], state);
      }
    }
  }
}


} // namespace destructables
