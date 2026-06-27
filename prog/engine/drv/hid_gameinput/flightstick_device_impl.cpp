// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "flightstick_device.h"

#include <drv/hid/dag_hiXInputMappings.h>
#include <math/dag_mathBase.h>
#include <startup/dag_globalSettings.h>

#include <EASTL/algorithm.h>
#include <EASTL/span.h>
#include <limits>

using namespace HumanInput;

#undef min
#undef max

constexpr FlightStickDevice::AxisValue min_s_axis_value = eastl::numeric_limits<int16_t>::min();
constexpr FlightStickDevice::AxisValue max_s_axis_value = eastl::numeric_limits<int16_t>::max();
constexpr FlightStickDevice::AxisValue min_u_axis_value = eastl::numeric_limits<uint16_t>::min();
constexpr FlightStickDevice::AxisValue max_u_axis_value = eastl::numeric_limits<uint16_t>::max();
constexpr FlightStickDevice::AxisValue s_axis_value_range = max_s_axis_value - min_s_axis_value;
constexpr FlightStickDevice::AxisValue u_axis_value_range = max_u_axis_value - min_u_axis_value;

struct ButtonInfo
{
  const char *name = "";
  int bitIdx = 0;
};

static constexpr ButtonInfo ButtonsRemap[] = {
  {"A", 12},
  {"B", 13},
  {"X", 14},
  {"Y", 15},

  {"Previous", 10},
  {"Next", 11},
  {"View", 5},
  {"Menu", 4},

  {"FirePrimary", 17},
  {"FireSecondary", 16},

  {"HatUp", 0},
  {"HatRight", 3},
  {"HatDown", 1},
  {"HatLeft", 2},

  {"ExtraButton1", 28},
  {"ExtraButton2", 29},
  {"ExtraButton3", 30},
  {"ExtraButton4", 31},
  {"ExtraButton5", 32},

  {"SlideLeft", 8},
  {"SlideRight", 9},
};

struct AxisInfo
{
  const char *name = "";
  bool isSigned = false;
  float defVal;
};

static constexpr AxisInfo AxesInfo[] = {
  {"Throttle", true, 0.f},
  {"Yaw", false, 0.5f},
  {"Pitch", false, 0.5f},
  {"Roll", false, 0.5f},
  {"ExtraAxis1", true, 0.5f},
  {"ExtraAxis2", false, 0.0f},
  {"ExtraAxis3", false, 0.0f},
};

static constexpr int AxisRemap[] = {3, 2, 1, 0, 4, 5, 6};

static constexpr size_t supported_buttons_count = countof(ButtonsRemap);
static constexpr size_t supported_axis_count = countof(AxesInfo);
static constexpr int supported_button_bits = []() {
  int maxBit = 0;
  for (const ButtonInfo &info : ButtonsRemap)
    maxBit = eastl::max(maxBit, info.bitIdx);
  return maxBit + 1;
}();

struct AxisRange
{
  FlightStickDevice::AxisValue min;
  FlightStickDevice::AxisValue range;
};

static constexpr AxisRange axis_range(const AxisInfo &info)
{
  return info.isSigned ? AxisRange{min_s_axis_value, s_axis_value_range} : AxisRange{min_u_axis_value, u_axis_value_range};
}

struct VirtualButton
{
  size_t axisId = supported_buttons_count;
  size_t minButton = supported_buttons_count;
  size_t maxButton = supported_buttons_count;
};

static constexpr VirtualButton virtualButtons[] = {{0, JOY_XINPUT_REAL_BTN_R_THUMB_LEFT, JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT}, // Roll
  {1, JOY_XINPUT_REAL_BTN_R_THUMB_DOWN, JOY_XINPUT_REAL_BTN_R_THUMB_UP},                                                     // Pitch
  {4, JOY_XINPUT_REAL_BTN_L_SHOULDER, JOY_XINPUT_REAL_BTN_R_SHOULDER}};

FlightStickDevice::FlightStickDevice(FSDevicePtr flight_stick, int uid)
{
  device = flight_stick;
  name.sprintf("FlightStick %d", uid + 1);

  const GameInputDeviceInfo *deviceInfo = flight_stick->GetDeviceInfo();
  if (deviceInfo)
  {
    SNPRINTF(id, sizeof(id), "%04hx:%04hx", deviceInfo->vendorId, deviceInfo->productId);
  }
  state.axes.resize(supported_axis_count);
  axesLimits.assign(supported_axis_count, eastl::make_pair(min_raw_axis_value, max_raw_axis_value));
}

static bool is_default_readings(eastl::span<const float> axis, eastl::span<const bool> buttons)
{
  bool isAnyAxisNonZero = eastl::any_of(axis.begin(), axis.end(), [](float val) { return val > VERY_SMALL_NUMBER; });
  bool isAnyButtonPressed = eastl::any_of(buttons.begin(), buttons.end(), [](bool val) { return val; });
  return !(isAnyAxisNonZero || isAnyButtonPressed);
}

static bool can_trust_data(bool can_trust_prev, eastl::span<const float> axis, eastl::span<const bool> buttons)
{
  // When application is in background, we can't trust readings
  if (!::dgs_app_active)
    return false;
  // If previous readings were trusted, so there is no doubt that we can trust current readings
  if (can_trust_prev)
    return true;
  // If readings are not default, we can trust them
  return !is_default_readings(axis, buttons);
}

bool FlightStickDevice::updateState(int dt_msec)
{
  gdk::gameinput::Reading reading = gdk::gameinput::get_current_reading(GameInputKindAny, device);
  if (!reading)
    return false;

  float axis[supported_axis_count] = {};
  reading->GetControllerAxisState(supported_axis_count, axis);

  bool buttons[supported_buttons_count] = {};
  reading->GetControllerButtonState(supported_buttons_count, buttons);

  FlightStickDeviceState prevState = state;

  canTrustData = can_trust_data(canTrustData, axis, buttons);

  for (size_t i = 0; i < supported_axis_count; ++i)
  {
    const AxisInfo &info = AxesInfo[i];
    const AxisRange range = axis_range(info);
    float currentAxisValue = canTrustData ? axis[i] : info.defVal;
    state.axes[i] = FlightStickDevice::AxisValue(currentAxisValue * range.range) + range.min;
  }

  state.buttons = 0;

  for (const VirtualButton &virtualButton : virtualButtons)
  {
    size_t axisId = virtualButton.axisId;
    size_t minButton = virtualButton.minButton;
    size_t maxButton = virtualButton.maxButton;
    if (axisId >= getAxisCount())
      continue;

    axisId = AxisRemap[axisId];
    const AxisRange range = axis_range(AxesInfo[axisId]);
    AxisValue &axis = state.axes[axisId];
    AxisValue minThreshold = range.min + range.range * virtual_thumbkey_min_multiplier;
    AxisValue maxThreshold = range.min + range.range * virtual_thumbkey_max_multiplier;
    set_bit(state.buttons, minButton, axis <= minThreshold);
    set_bit(state.buttons, maxButton, axis >= maxThreshold);
  }

  for (size_t i = 0; i < supported_buttons_count; ++i)
    if (buttons[i])
      state.buttons |= 1ULL << ButtonsRemap[i].bitIdx;

  rawState.buttonsPrev = rawState.buttons;

  if (state != prevState)
  {
    rawState.x = getAxisPosRaw(0);  // Roll
    rawState.y = getAxisPosRaw(1);  // Pitch
    rawState.rx = getAxisPosRaw(2); // Yaw
    rawState.ry = getAxisPosRaw(3); // Throttle
    rawState.buttons.reset();
    rawState.buttons.orDWord0(state.buttons);
  }

  for (int i = 0; i < getButtonCount(); ++i)
    if (rawState.buttonsPrev.get(i))
      rawState.bDownTms[i] += dt_msec;
    else if (!rawState.buttons.get(i))
      rawState.bDownTms[i] = 0;

  return state != prevState;
}

void FlightStickDevice::setAxisLimits(int axis_id, float min_val, float max_val)
{
  if (axis_id < 0 || axis_id >= getAxisCount())
  {
    DEBUG_CTX("Invalid axis_id = %d", axis_id);
    return;
  }
  axesLimits[AxisRemap[axis_id]] = eastl::make_pair(min_val, max_val);
}

float FlightStickDevice::getAxisPos(int axis_id) const
{
  if (axis_id < 0 || axis_id >= getAxisCount())
  {
    DEBUG_CTX("Invalid axis_id = %d", axis_id);
    return 0.f;
  }

  axis_id = AxisRemap[axis_id];
  float multiplier = getAxisMultiplier(axis_id);
  const FlightStickAxisLimits &limits = axesLimits[axis_id];
  int minMaxDelta = limits.second - limits.first;
  return multiplier * minMaxDelta + limits.first;
}

int FlightStickDevice::getAxisPosRaw(int axis_id) const
{
  if (axis_id < 0 || axis_id >= getAxisCount())
  {
    DEBUG_CTX("Invalid axis_id = %d", axis_id);
    return 0;
  }

  axis_id = AxisRemap[axis_id];
  float multiplier = getAxisMultiplier(axis_id);
  return multiplier * min_max_raw_delta + min_raw_axis_value;
}

float FlightStickDevice::getAxisMultiplier(int axis_id) const
{
  if (axis_id >= getAxisCount())
  {
    DEBUG_CTX("Invalid axis_id = %d", axis_id);
    return 0.f;
  }
  const AxisRange range = axis_range(AxesInfo[axis_id]);
  return float(state.axes[axis_id] - range.min) / range.range;
}

const char *FlightStickDevice::getButtonName(int idx) const
{
  for (const ButtonInfo &info : ButtonsRemap)
    if (info.bitIdx == idx)
      return info.name;

  DEBUG_CTX("Invalid button idx: %d", idx);
  return nullptr;
}

const char *FlightStickDevice::getAxisName(int idx) const
{
  if (idx >= 0 && idx < (int)supported_axis_count)
    return AxesInfo[AxisRemap[idx]].name;

  DEBUG_CTX("Invalid axis idx: %d", idx);
  return nullptr;
}

int FlightStickDevice::getButtonCount() const { return supported_button_bits; }

int FlightStickDevice::getAxisCount() const { return supported_axis_count; }
