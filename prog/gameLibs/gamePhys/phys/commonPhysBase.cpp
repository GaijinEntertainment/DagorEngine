// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/commonPhysBase.h>
#include <daNet/bitStream.h>

#define DO_VEH_PARTIAL_MEMBERS \
  SR(location)                 \
  SR(velocity)                 \
  SR(atTick)                   \
  SR(unitVersion)

void write_controls_tick_delta(danet::BitStream &bs, int tick, int ctrlTick)
{
  int ctrlDelta = tick - ctrlTick - 1;
  if (ctrlDelta > 0 && ctrlDelta < (1 << 7))
  {
    bs.Write1();
    bs.WriteBits((const uint8_t *)&ctrlDelta, 7);
  }
  else
    bs.Write0();
}
bool read_controls_tick_delta(const danet::BitStream &bs, int tick, int &ctrlTick)
{
  ctrlTick = tick - 1;
  bool bCtrlDelta = false;
  if (!bs.Read(bCtrlDelta))
    return false;
  if (bCtrlDelta)
  {
    uint8_t ctrlDelta = 0;
    if (!bs.ReadBits(&ctrlDelta, 7))
      return false;
    ctrlTick = tick - ctrlDelta - 1;
  }
  return true;
}

void CommonPhysPartialState::serialize(danet::BitStream &bs) const
{
#define SR(x) bs.Write(x);
  DO_VEH_PARTIAL_MEMBERS
#undef SR
  write_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
}

bool CommonPhysPartialState::deserialize(const danet::BitStream &bs, IPhysBase &)
{
#define SR(x)      \
  if (!bs.Read(x)) \
    return false;
  DO_VEH_PARTIAL_MEMBERS
#undef SR
  return read_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
}
