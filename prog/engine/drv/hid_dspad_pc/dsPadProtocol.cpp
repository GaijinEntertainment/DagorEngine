// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadProtocol.h"
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace HumanInput
{
namespace sonypad
{

const char *model_name(Model model)
{
  switch (model)
  {
    case Model::DUALSENSE: return "DualSense";
    case Model::DUALSENSE_EDGE: return "DualSense Edge";
    case Model::DUALSHOCK4: return "DualShock 4";
    default: return "Unknown Sony pad";
  }
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, int len)
{
  for (int i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (int b = 0; b < 8; b++)
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return crc;
}

uint32_t report_crc32(uint8_t seed, const uint8_t *data, int len)
{
  uint32_t crc = crc32_update(0xFFFFFFFFu, &seed, 1);
  return ~crc32_update(crc, data, len);
}

static inline uint16_t load_le16(const uint8_t *p) { return uint16_t(p[0] | (p[1] << 8)); }
static inline int16_t load_s16(const uint8_t *p) { return int16_t(load_le16(p)); }
static inline uint32_t load_le32(const uint8_t *p) { return uint32_t(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)); }
static inline void store_le32(uint8_t *p, uint32_t v)
{
  p[0] = uint8_t(v);
  p[1] = uint8_t(v >> 8);
  p[2] = uint8_t(v >> 16);
  p[3] = uint8_t(v >> 24);
}

bool check_report_crc32(uint8_t seed, const uint8_t *report, int len)
{
  return len >= 4 && report_crc32(seed, report, len - 4) == load_le32(report + len - 4);
}

// b points at the three button bytes: hat and face buttons, shoulders and sticks, system buttons
static uint32_t decode_buttons(const uint8_t *b, bool edge)
{
  uint32_t r = 0;
  r |= (b[0] & 0x10) ? BTN_SQUARE : 0;
  r |= (b[0] & 0x20) ? BTN_CROSS : 0;
  r |= (b[0] & 0x40) ? BTN_CIRCLE : 0;
  r |= (b[0] & 0x80) ? BTN_TRIANGLE : 0;
  r |= (b[1] & 0x01) ? BTN_L1 : 0;
  r |= (b[1] & 0x02) ? BTN_R1 : 0;
  r |= (b[1] & 0x04) ? BTN_L2 : 0;
  r |= (b[1] & 0x08) ? BTN_R2 : 0;
  r |= (b[1] & 0x10) ? BTN_CREATE : 0;
  r |= (b[1] & 0x20) ? BTN_OPTIONS : 0;
  r |= (b[1] & 0x40) ? BTN_L3 : 0;
  r |= (b[1] & 0x80) ? BTN_R3 : 0;
  r |= (b[2] & 0x01) ? BTN_PS : 0;
  r |= (b[2] & 0x02) ? BTN_TOUCHPAD : 0;
  r |= (b[2] & 0x04) ? BTN_MUTE : 0;
  if (edge)
  {
    r |= (b[2] & 0x10) ? BTN_FN_LEFT : 0;
    r |= (b[2] & 0x20) ? BTN_FN_RIGHT : 0;
    r |= (b[2] & 0x40) ? BTN_PADDLE_LEFT : 0;
    r |= (b[2] & 0x80) ? BTN_PADDLE_RIGHT : 0;
  }
  return r;
}

static void decode_touch(const uint8_t *p, JoystickTouchpadState::Point &t)
{
  t.down = (p[0] & 0x80) == 0;
  t.contactId = p[0] & 0x7F;
  t.x = uint16_t(p[1] | ((p[2] & 0x0F) << 8));
  t.y = uint16_t((p[2] >> 4) | (p[3] << 4));
}

static void decode_sensors(const uint8_t *gyro, const uint8_t *accel, InputSample &out)
{
  out.hasSensors = true;
  for (int i = 0; i < 3; i++)
  {
    out.gyroRaw[i] = load_s16(gyro + i * 2);
    out.accelRaw[i] = load_s16(accel + i * 2);
  }
}

// p points at the first payload byte after the transport header (x axis)
static void parse_dualsense_payload(const uint8_t *p, bool edge, InputSample &out)
{
  out.lx = p[0];
  out.ly = p[1];
  out.rx = p[2];
  out.ry = p[3];
  out.l2 = p[4];
  out.r2 = p[5];
  out.hat = p[7] & 0x0F;
  out.buttons = decode_buttons(p + 7, edge);
  decode_sensors(p + 15, p + 21, out);
  out.sensorTicks = load_le32(p + 27);
  decode_touch(p + 32, out.touch[0]);
  decode_touch(p + 36, out.touch[1]);
  out.hasTouch = true;
  out.hasStatus = true;
  out.batteryLevel = min(p[52] & 0x0F, 10);
  switch (p[52] >> 4)
  {
    case 0x0: out.charge = CHARGE_DISCHARGING; break;
    case 0x1: out.charge = CHARGE_CHARGING; break;
    case 0x2: out.charge = CHARGE_FULL; break;
    default: out.charge = CHARGE_UNKNOWN; break;
  }
  out.cablePowered = (p[53] & 0x08) != 0;
}

// the 10-byte report a Bluetooth pad sends until a feature report is read, in the DualShock 4 field order for
// both models: sticks, hat and buttons, triggers
static void parse_minimal_payload(const uint8_t *p, InputSample &out)
{
  out.lx = p[0];
  out.ly = p[1];
  out.rx = p[2];
  out.ry = p[3];
  out.hat = p[4] & 0x0F;
  uint8_t btn[3] = {p[4], p[5], uint8_t(p[6] & 0x03)};
  out.buttons = decode_buttons(btn, false);
  out.l2 = p[7];
  out.r2 = p[8];
}

// p: the first payload byte (left stick x); the count of touch packets is at p + 32, then one packet with its timestamp and
// two 4-byte points
static void parse_dualshock4_payload(const uint8_t *p, InputSample &out)
{
  parse_minimal_payload(p, out);
  decode_sensors(p + 12, p + 18, out);
  out.sensorTicks = load_le16(p + 9);
  out.sensorTicksMask = 0xFFFFu;
  out.sensorTickUs = 16.f / 3.f;
  if (p[32] > 0)
  {
    decode_touch(p + 34, out.touch[0]);
    decode_touch(p + 38, out.touch[1]);
    out.hasTouch = true;
  }
  out.hasStatus = true;
  uint8_t level = p[29] & 0x0F;
  out.cablePowered = (p[29] & 0x10) != 0;
  out.dongleEmpty = (p[30] & 0x04) != 0;
  if (out.cablePowered)
  {
    if (level <= 10)
    {
      out.batteryLevel = level;
      out.charge = CHARGE_CHARGING;
    }
    else if (level == 11)
    {
      out.batteryLevel = 10;
      out.charge = CHARGE_FULL;
    }
  }
  else
  {
    out.batteryLevel = level > 10 ? 10 : level;
    out.charge = CHARGE_DISCHARGING;
  }
}

bool parse_input_report(Model model, bool bluetooth, const uint8_t *report, int len, InputSample &out)
{
  out = InputSample();
  if (len < 1)
    return false;
  const uint8_t id = report[0];
  if (model == Model::DUALSENSE || model == Model::DUALSENSE_EDGE)
  {
    const bool edge = model == Model::DUALSENSE_EDGE;
    if (id == DS_INPUT_REPORT_USB && len >= DS_INPUT_REPORT_USB_SIZE && !bluetooth)
      parse_dualsense_payload(report + 1, edge, out);
    else if (id == DS_INPUT_REPORT_USB && len >= DS_INPUT_REPORT_BT_MINIMAL_SIZE)
      parse_minimal_payload(report + 1, out);
    else if (id == DS_INPUT_REPORT_BT && len >= DS_INPUT_REPORT_BT_SIZE)
    {
      if (!check_report_crc32(CRC32_SEED_INPUT, report, DS_INPUT_REPORT_BT_SIZE))
        return false;
      parse_dualsense_payload(report + 2, edge, out);
    }
    else
      return false;
    return true;
  }
  if (model == Model::DUALSHOCK4)
  {
    if (id == DS4_INPUT_REPORT_USB && len >= DS4_INPUT_REPORT_USB_SIZE && !bluetooth)
      parse_dualshock4_payload(report + 1, out);
    else if (id == DS4_INPUT_REPORT_USB && len >= DS4_INPUT_REPORT_BT_MINIMAL_SIZE)
      parse_minimal_payload(report + 1, out);
    else if (id == DS4_INPUT_REPORT_BT && len >= DS4_INPUT_REPORT_BT_SIZE)
    {
      if (!(report[1] & 0x80) || !check_report_crc32(CRC32_SEED_INPUT, report, DS4_INPUT_REPORT_BT_SIZE))
        return false;
      parse_dualshock4_payload(report + 3, out);
    }
    else
      return false;
    return true;
  }
  return false;
}

int battery_percent(const InputSample &s)
{
  if (!s.hasStatus || s.charge == CHARGE_UNKNOWN)
    return -1;
  if (s.charge == CHARGE_FULL)
    return 100;
  return min(int(s.batteryLevel) * 10 + 5, 100);
}

void SensorCalibration::setDefaults()
{
  for (int i = 0; i < 3; i++)
  {
    gyro[i].bias = 0;
    gyro[i].numer = 2048 * GYRO_RES_PER_DEG_S;
    gyro[i].denom = 32767;
    accel[i].bias = 0;
    accel[i].numer = 4 * ACCEL_RES_PER_G;
    accel[i].denom = 32767;
    gyroReportedBias[i] = 0;
  }
}

bool parse_calibration(Model model, const uint8_t *report, int len, SensorCalibration &out)
{
  out.setDefaults();
  if (len < 35)
    return false;
  const bool ds4Bt = model == Model::DUALSHOCK4 && report[0] == DS4_FEATURE_REPORT_CALIBRATION_BT;
  const int speed2x = load_s16(report + 19) + load_s16(report + 21);
  for (int i = 0; i < 3; i++)
  {
    const int bias = load_s16(report + 1 + i * 2);
    const int plus = load_s16(report + (ds4Bt ? 7 + i * 2 : 7 + i * 4));
    const int minus = load_s16(report + (ds4Bt ? 13 + i * 2 : 9 + i * 4));
    // the pad zeroes its own gyro, so hid-playstation keeps the reported bias for logging only
    out.gyroReportedBias[i] = bias;
    const int denom = abs(plus - bias) + abs(minus - bias);
    if (denom == 0 || speed2x == 0)
      continue;
    out.gyro[i].numer = speed2x * GYRO_RES_PER_DEG_S;
    out.gyro[i].denom = denom;
  }
  for (int i = 0; i < 3; i++)
  {
    const int plus = load_s16(report + 23 + i * 4), minus = load_s16(report + 25 + i * 4);
    const int range2g = plus - minus;
    if (range2g == 0)
      continue;
    out.accel[i].bias = plus - range2g / 2;
    out.accel[i].numer = 2 * ACCEL_RES_PER_G;
    out.accel[i].denom = range2g;
  }
  return true;
}

bool parse_firmware_info(Model model, const uint8_t *report, int len, FirmwareInfo &out)
{
  out = FirmwareInfo();
  if (model == Model::DUALSHOCK4)
  {
    if (len < 43 || report[0] != DS4_FEATURE_REPORT_FIRMWARE_INFO)
      return false;
    out.hwVersion = load_le16(report + 35);
    out.fwVersion = load_le16(report + 41);
    return true;
  }
  if (len < 46 || report[0] != DS_FEATURE_REPORT_FIRMWARE_INFO)
    return false;
  out.hwVersion = load_le32(report + 24);
  out.fwVersion = load_le32(report + 28);
  out.updateVersion = load_le16(report + 44);
  return true;
}

bool parse_mac_address(Model model, const uint8_t *report, int len, char out[18])
{
  const uint8_t id = model == Model::DUALSHOCK4 ? DS4_FEATURE_REPORT_PAIRING_INFO : DS_FEATURE_REPORT_PAIRING_INFO;
  if (len < 7 || report[0] != id)
    return false;
  snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", report[6], report[5], report[4], report[3], report[2], report[1]);
  return true;
}

static inline uint8_t clamp_u8(int v, int lo, int hi) { return uint8_t(v < lo ? lo : (v > hi ? hi : v)); }

static void encode_zones(uint8_t mode, uint16_t active, uint32_t values, uint8_t frequency, uint8_t out[11])
{
  out[0] = mode;
  out[1] = uint8_t(active);
  out[2] = uint8_t(active >> 8);
  store_le32(out + 3, values);
  out[9] = frequency;
}

// strength per position packs into 3 bits each; 0 keeps the position inactive
static bool pack_positions(const uint8_t *strength, uint16_t &active, uint32_t &values)
{
  active = 0;
  values = 0;
  for (int i = 0; i < TRIGGER_EFFECT_CONTROL_POINT_NUM; i++)
  {
    const int s = min(int(strength[i]), 8);
    if (s <= 0)
      continue;
    active |= uint16_t(1u << i);
    values |= uint32_t(s - 1) << (3 * i);
  }
  return active != 0;
}

static void encode_trigger_effect(const TriggerEffectCommand &cmd, uint8_t out[11])
{
  enum : uint8_t
  {
    MODE_OFF = 0x05,
    MODE_FEEDBACK = 0x21,
    MODE_WEAPON = 0x25,
    MODE_VIBRATION = 0x26,
  };
  uint16_t active;
  uint32_t values;
  uint8_t strength[TRIGGER_EFFECT_CONTROL_POINT_NUM] = {};
  switch (cmd.mode)
  {
    case TRIGGER_EFFECT_MODE_FEEDBACK:
    {
      const int pos = clamp_u8(cmd.feedback.position, 0, 9);
      for (int i = pos; i < TRIGGER_EFFECT_CONTROL_POINT_NUM; i++)
        strength[i] = cmd.feedback.strength;
      if (pack_positions(strength, active, values))
        return encode_zones(MODE_FEEDBACK, active, values, 0, out);
      break;
    }
    case TRIGGER_EFFECT_MODE_WEAPON:
    {
      const int start = clamp_u8(cmd.weapon.startPosition, 2, 7);
      const int end = clamp_u8(cmd.weapon.endPosition, start + 1, 8);
      const int s = min(int(cmd.weapon.strength), 8);
      if (s > 0)
      {
        // the pad reads the weapon strength from byte 3 of the effect block
        encode_zones(MODE_WEAPON, uint16_t((1u << start) | (1u << end)), uint32_t(s - 1), 0, out);
        return;
      }
      break;
    }
    case TRIGGER_EFFECT_MODE_VIBRATION:
    {
      const int pos = clamp_u8(cmd.vibration.position, 0, 9);
      for (int i = pos; i < TRIGGER_EFFECT_CONTROL_POINT_NUM; i++)
        strength[i] = cmd.vibration.amplitude;
      if (cmd.vibration.frequency > 0 && pack_positions(strength, active, values))
        return encode_zones(MODE_VIBRATION, active, values, cmd.vibration.frequency, out);
      break;
    }
    case TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK:
      if (pack_positions(cmd.multiplePositionFeedback.strength, active, values))
        return encode_zones(MODE_FEEDBACK, active, values, 0, out);
      break;
    case TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK:
    {
      const int start = clamp_u8(cmd.slopeFeedback.startPosition, 0, 8);
      const int end = clamp_u8(cmd.slopeFeedback.endPosition, start + 1, 9);
      const int s0 = clamp_u8(cmd.slopeFeedback.startStrength, 1, 8), s1 = clamp_u8(cmd.slopeFeedback.endStrength, 1, 8);
      const float slope = float(s1 - s0) / float(end - start);
      for (int i = start; i < TRIGGER_EFFECT_CONTROL_POINT_NUM; i++)
        strength[i] = i <= end ? uint8_t(roundf(s0 + slope * (i - start))) : uint8_t(s1);
      if (pack_positions(strength, active, values))
        return encode_zones(MODE_FEEDBACK, active, values, 0, out);
      break;
    }
    case TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION:
      if (cmd.multiplePositionVibration.frequency > 0 && pack_positions(cmd.multiplePositionVibration.amplitude, active, values))
        return encode_zones(MODE_VIBRATION, active, values, cmd.multiplePositionVibration.frequency, out);
      break;
    default: break;
  }
  encode_zones(MODE_OFF, 0, 0, 0, out);
}

// offsets inside the 47-byte DualSense output block shared by the USB and Bluetooth reports
enum
{
  DS_OUT_FLAG0 = 0,
  DS_OUT_FLAG1 = 1,
  DS_OUT_MOTOR_RIGHT = 2,
  DS_OUT_MOTOR_LEFT = 3,
  DS_OUT_MIC_LED = 8,
  DS_OUT_TRIGGER_R2 = 10,
  DS_OUT_TRIGGER_L2 = 21,
  DS_OUT_FLAG2 = 38,
  DS_OUT_LIGHTBAR_SETUP = 41,
  DS_OUT_PLAYER_LEDS = 43,
  DS_OUT_LIGHTBAR_RGB = 44,

  DS_OUTPUT_TAG = 0x10, // byte 2 of the Bluetooth header, after the sequence nibble
  DS_FLAG0_COMPATIBLE_VIBRATION = 0x01,
  DS_FLAG0_HAPTICS_SELECT = 0x02,
  DS_FLAG0_TRIGGER_R2 = 0x04,
  DS_FLAG0_TRIGGER_L2 = 0x08,
  DS_FLAG1_MIC_LED = 0x01,
  DS_FLAG1_LIGHTBAR = 0x04,
  DS_FLAG1_PLAYER_LEDS = 0x10,
  DS_FLAG2_LIGHTBAR_SETUP = 0x02,
  DS_FLAG2_COMPATIBLE_VIBRATION2 = 0x04,
  DS_LIGHTBAR_SETUP_LIGHT_OUT = 0x02,
};

static void fill_dualsense_common(const OutputState &st, bool rumble_v2, uint8_t *c)
{
  c[DS_OUT_FLAG0] |= DS_FLAG0_HAPTICS_SELECT;
  if (rumble_v2)
  {
    c[DS_OUT_FLAG2] |= DS_FLAG2_COMPATIBLE_VIBRATION2;
    c[DS_OUT_MOTOR_LEFT] = st.rumbleLeft;
    c[DS_OUT_MOTOR_RIGHT] = st.rumbleRight;
  }
  else
  {
    // the first firmware drives the haptics twice as hard in the compatible mode
    c[DS_OUT_FLAG0] |= DS_FLAG0_COMPATIBLE_VIBRATION;
    c[DS_OUT_MOTOR_LEFT] = st.rumbleLeft >> 1;
    c[DS_OUT_MOTOR_RIGHT] = st.rumbleRight >> 1;
  }
  if (st.lightbarSet)
  {
    c[DS_OUT_FLAG1] |= DS_FLAG1_LIGHTBAR;
    memcpy(c + DS_OUT_LIGHTBAR_RGB, st.lightbar, 3);
  }
  if (st.playerLedsSet)
  {
    c[DS_OUT_FLAG1] |= DS_FLAG1_PLAYER_LEDS;
    c[DS_OUT_PLAYER_LEDS] = st.playerLeds;
  }
  if (st.micLedSet)
  {
    c[DS_OUT_FLAG1] |= DS_FLAG1_MIC_LED;
    c[DS_OUT_MIC_LED] = st.micLed;
  }
  c[DS_OUT_FLAG0] |= DS_FLAG0_TRIGGER_R2 | DS_FLAG0_TRIGGER_L2;
  encode_trigger_effect(st.trigger[TRIGGER_EFFECT_INDEX_R2], c + DS_OUT_TRIGGER_R2);
  encode_trigger_effect(st.trigger[TRIGGER_EFFECT_INDEX_L2], c + DS_OUT_TRIGGER_L2);
  if (st.resetLightbar)
  {
    c[DS_OUT_FLAG2] |= DS_FLAG2_LIGHTBAR_SETUP;
    c[DS_OUT_LIGHTBAR_SETUP] = DS_LIGHTBAR_SETUP_LIGHT_OUT;
  }
}

enum
{
  DS4_FLAG0_MOTOR = 0x01,
  DS4_FLAG0_LIGHTBAR = 0x02,
  DS4_BT_HWCTL_HID = 0x80,
  DS4_BT_HWCTL_CRC32 = 0x40,
  DS4_BT_POLL_INTERVAL_MS = 4,
};

// common block: flag0, flag1, reserved, motor right, motor left, r, g, b, blink on, blink off
static void fill_dualshock4_common(const OutputState &st, uint8_t *c)
{
  c[0] = DS4_FLAG0_MOTOR | (st.lightbarSet ? DS4_FLAG0_LIGHTBAR : 0);
  c[3] = st.rumbleRight;
  c[4] = st.rumbleLeft;
  if (st.lightbarSet)
    memcpy(c + 5, st.lightbar, 3);
}

int build_output_report(Model model, bool bluetooth, bool rumble_v2, const OutputState &st, uint8_t &bt_seq, uint8_t *out, int cap)
{
  if (model == Model::UNKNOWN)
    return 0;
  const bool ds4 = model == Model::DUALSHOCK4;
  const int len = bluetooth ? (ds4 ? DS4_OUTPUT_REPORT_BT_SIZE : DS_OUTPUT_REPORT_BT_SIZE)
                            : (ds4 ? DS4_OUTPUT_REPORT_USB_SIZE : DS_OUTPUT_REPORT_USB_SIZE);
  if (cap < len)
    return 0;
  memset(out, 0, len);
  if (ds4)
  {
    if (bluetooth)
    {
      out[0] = DS4_OUTPUT_REPORT_BT;
      out[1] = DS4_BT_HWCTL_HID | DS4_BT_HWCTL_CRC32 | DS4_BT_POLL_INTERVAL_MS;
      fill_dualshock4_common(st, out + 3);
    }
    else
    {
      out[0] = DS4_OUTPUT_REPORT_USB;
      fill_dualshock4_common(st, out + 1);
    }
  }
  else if (bluetooth)
  {
    out[0] = DS_OUTPUT_REPORT_BT;
    out[1] = uint8_t(bt_seq << 4); // high nibble counts reports, low nibble is a zero tag
    out[2] = DS_OUTPUT_TAG;
    bt_seq = (bt_seq + 1) & 0x0F;
    fill_dualsense_common(st, rumble_v2, out + 3);
  }
  else
  {
    out[0] = DS_OUTPUT_REPORT_USB;
    fill_dualsense_common(st, rumble_v2, out + 1);
  }
  if (bluetooth)
    store_le32(out + len - 4, report_crc32(CRC32_SEED_OUTPUT, out, len - 4));
  return len;
}

} // namespace sonypad
} // namespace HumanInput
