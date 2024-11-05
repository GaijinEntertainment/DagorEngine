//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_hierBitArray.h>
#include <util/dag_stdint.h>

namespace HumanInput
{
struct ButtonBits : public ConstSizeBitArray<8> // state bits for 256 buttons
{
  static constexpr int WORD_CNT = (SZ + 31) / 32;

  ButtonBits() { ctor(); }

  bool cmpEq(const ButtonBits &b) const { return memcmp(bits, b.bits, sizeof(bits)) == 0; }
  bool bitTest(const ButtonBits &op2) const
  {
    for (int i = 0; i < WORD_CNT; i++)
      if (bits[i] & op2.bits[i])
        return true;
    return false;
  }

  unsigned getWord0() const { return bits[0]; }
  uint64_t getDWord0() const { return uint64_t(bits[0]) | (uint64_t(bits[1]) << 32); }

  bool hasAny() const
  {
    for (int i = 0; i < WORD_CNT; i++)
      if (bits[i])
        return true;
    return false;
  }

  bool bitXor(ButtonBits &dest, const ButtonBits &op2) const
  {
    unsigned any = 0;
    for (int i = 0; i < WORD_CNT; i++)
    {
      unsigned w = (bits[i] ^ op2.bits[i]);
      dest.bits[i] = w;
      any |= w;
    }
    return any;
  }
  void bitXorAnd1(ButtonBits &dest, const ButtonBits &op2) const
  {
    for (int i = 0; i < WORD_CNT; i++)
      dest.bits[i] = (bits[i] ^ op2.bits[i]) & bits[i];
  }
  void bitXorAnd2(ButtonBits &dest, const ButtonBits &op2) const
  {
    for (int i = 0; i < WORD_CNT; i++)
      dest.bits[i] = (bits[i] ^ op2.bits[i]) & op2.bits[i];
  }
  void bitBlend(const ButtonBits &op2, const ButtonBits &mask)
  {
    for (int i = 0; i < WORD_CNT; i++)
      if (!mask.bits[i])
        bits[i] = op2.bits[i];
      else if (mask.bits[i] != 0xFFFFFFFF)
        bits[i] = (bits[i] & mask.bits[i]) | (op2.bits[i] & ~mask.bits[i]);
  }
  void bitOr(const ButtonBits &op2)
  {
    for (int i = 0; i < WORD_CNT; i++)
      bits[i] |= op2.bits[i];
  }
  void bitInvAnd(const ButtonBits &op2)
  {
    for (int i = 0; i < WORD_CNT; i++)
      bits[i] &= ~op2.bits[i];
  }

  void orWord0(unsigned mask) { bits[0] |= mask; }
  void orDWord0(uint64_t mask)
  {
    bits[0] |= (unsigned)(mask & uint64_t(0xFFFFFFFF));
    bits[1] |= (unsigned)(mask >> 32);
  }

  static const ButtonBits ZERO;
};

struct JoystickSettings
{
  bool present;
  bool enabled;
  int keyRepeatDelay;
  int keyFirstRepeatDelay;
  bool sortDevices;
  bool disableVirtualControls;
};

struct JoystickRawState
{
  static constexpr int MAX_BUTTONS = ButtonBits::FULL_SZ;
  static constexpr int MAX_POV_HATS = 4;

  int x, y, z;
  int rx, ry, rz;
  int slider[2];
  ButtonBits buttons;
  ButtonBits buttonsPrev;
  int povValues[MAX_POV_HATS];
  int povValuesPrev[MAX_POV_HATS];

  // acceleration sensors (X,Y,Z) and angular velocity sensor(G)
  float sensorX, sensorY, sensorZ, sensorG;
  short rawSensX, rawSensY, rawSensZ, rawSensG;

  // FIXME: anything below repeat is not considered for equality check by CompositeJoystickDevice
  bool repeat;
  unsigned short bDownTms[MAX_BUTTONS];

  const ButtonBits &getKeysPressed() const
  {
    buttons.bitXorAnd1(btmp[0], buttonsPrev);
    return btmp[0];
  }
  const ButtonBits &getKeysReleased() const
  {
    buttons.bitXorAnd2(btmp[1], buttonsPrev);
    return btmp[1];
  }

  const ButtonBits &getKeysDown() const { return buttons; }
  const ButtonBits &getKeysRepeated() { return repeat ? getKeysDown() : ButtonBits::ZERO; }

protected:
  static ButtonBits btmp[2];
};

} // namespace HumanInput
