// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <gamePhys/common/loc.h>
#include <generic/dag_carray.h>
#include <phys/dag_physDecl.h>


namespace danet
{
class BitStream;
}

namespace destructables
{

struct DestrNetPhys
{
  struct PhysBodyState
  {
    gamephys::Loc loc;
    Point3 vel, omega;

    static PhysBodyState add(const PhysBodyState &A, const PhysBodyState &B);
    static PhysBodyState subtract(const PhysBodyState &A, const PhysBodyState &B);
    static PhysBodyState interpolate(const PhysBodyState &A, const PhysBodyState &B, float t);

    void serialize(danet::BitStream &bs) const;
    bool deserialize(const danet::BitStream &bs);
  };

  // states are recorded at a fixed lifetime interval, so a slot index is the time
  static constexpr int HISTORY_STATES = 12;
  static constexpr float HISTORY_INTERVAL = 0.05f;

  struct PiecePhys
  {
    carray<PhysBodyState, HISTORY_STATES> history;
    int histCur = 0, histSize = 0;
    float lastAtTime = 0.f;

    // k = 0 is the newest recorded state
    PhysBodyState &back(int k) { return history[(histCur - k + HISTORY_STATES) % HISTORY_STATES]; }
    void record(const PhysBodyState &, float life_time);
  };
  dag::Vector<PiecePhys> pieces;
};

} // namespace destructables
