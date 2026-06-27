// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>

#include <drv/hid/dag_hiJoystick.h>
#include <osApiWrappers/gdk/gameinput.h>

namespace HumanInput
{
using FlightStickAxisLimits = eastl::pair<float, float>;

constexpr double virtual_thumbkey_min_multiplier = 0.05;
constexpr double virtual_thumbkey_max_multiplier = 0.95;

using FSDevicePtr = IGameInputDevice *;

struct FlightStickDeviceState
{
  using AxisValue = int32_t;
  using ButtonBits = uint64_t;
  ButtonBits buttons = 0;
  eastl::fixed_vector<AxisValue, 4> axes;
};

template <typename T>
static inline void set_bit(T &value, size_t bit_shift, bool bit_value)
{
  if (bit_value)
    value |= static_cast<T>(1) << bit_shift;
  else
    value &= ~(static_cast<T>(1) << bit_shift);
}

bool operator==(const FlightStickDeviceState &lhs, const FlightStickDeviceState &rhs);
bool operator!=(const FlightStickDeviceState &lhs, const FlightStickDeviceState &rhs);

class FlightStickDevice : public IGenJoystick
{
public:
  using AxisValue = FlightStickDeviceState::AxisValue;
  using ButtonBits = FlightStickDeviceState::ButtonBits;
  static constexpr int min_raw_axis_value = -32000;
  static constexpr int max_raw_axis_value = 32000;
  static constexpr int min_max_raw_delta = max_raw_axis_value - min_raw_axis_value;

  FlightStickDevice(FSDevicePtr flight_stick, int uid);
  ~FlightStickDevice();

  bool updateState(int dt_msec);
  FSDevicePtr getDevice() const;

  const char *getName() const override;
  const char *getDeviceID() const override;
  bool getDeviceDesc(DataBlock &out_desc) const override;
  unsigned short getUserId() const override;
  const JoystickRawState &getRawState() const override;
  void setClient(IGenJoystickClient *other) override;
  IGenJoystickClient *getClient() const override;

  int getButtonCount() const override;
  const char *getButtonName(int idx) const override;

  int getAxisCount() const override;
  const char *getAxisName(int idx) const override;

  int getPovHatCount() const override;
  const char *getPovHatName(int idx) const override;

  int getButtonId(const char *name) const override;
  int getAxisId(const char *name) const override;
  int getPovHatId(const char *name) const override;
  void setAxisLimits(int axis_id, float min_val, float max_val) override;
  bool getButtonState(int btn_id) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;
  int getPovHatAngle(int axis_id) const override;

  bool isConnected() override;
  bool isXinputCompatible() const override { return true; }

protected:
  FSDevicePtr device = nullptr;
  IGenJoystickClient *client = nullptr;
  JoystickRawState rawState = {};
  FlightStickDeviceState state;

  char id[10] = "????:????";
  eastl::string name;

  eastl::fixed_vector<FlightStickAxisLimits, 4> axesLimits;

  bool canTrustData = false;

  float getAxisMultiplier(int axis_id) const;
};
} // namespace HumanInput
