//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_hierBitArray.h>
#include <util/dag_stdint.h>

namespace HumanInput
{
struct ButtonBits : public ConstSizeBitArray<9> // state bits for 512 buttons
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

// calibrated motion sensors of the latest device report; frame: X right, Y up, Z to the player
struct JoystickMotionState
{
  float angVelDegS[3] = {0, 0, 0}; // right-handed rotation rate around X (pitch), Y (yaw), Z (roll)
  float accelG[3] = {0, 0, 0};     // at rest the up axis reads +1
  uint32_t timestampUs = 0;        // device time since the pad connected, wraps
  uint32_t sampleCount = 0;        // reports received since connect
};

// points in pixels of a width x height pad, origin in the top-left corner, x to the right, y down;
// contactId changes for each new touch
struct JoystickTouchpadState
{
  struct Point
  {
    bool down = false;
    uint8_t contactId = 0;
    uint16_t x = 0, y = 0;
  };
  Point points[2];
  uint16_t width = 0, height = 0;
};

enum JoystickMicLed : int
{
  JOY_MIC_LED_OFF,
  JOY_MIC_LED_ON,
  JOY_MIC_LED_PULSE
};

} // namespace HumanInput
