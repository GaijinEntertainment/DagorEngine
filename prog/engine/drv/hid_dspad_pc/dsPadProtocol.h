// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/hid/dag_hiJoyData.h>
#include <drv/hid/dag_hiJoyTriggerEffect.h>
#include "../hid_commonCode/dsPadIds.h"

// Report layouts follow the Linux hid-playstation driver and SDL3 SDL_hidapi_ps5.c / SDL_hidapi_ps4.c.
namespace HumanInput
{
namespace sonypad
{

const char *model_name(Model model);

static constexpr int GYRO_RES_PER_DEG_S = 1024;
static constexpr int ACCEL_RES_PER_G = 8192;

enum
{
  MAX_REPORT_SIZE = 78,
  TOUCHPAD_WIDTH = 1920,
  DS_TOUCHPAD_HEIGHT = 1080,
  DS4_TOUCHPAD_HEIGHT = 942,

  DS_INPUT_REPORT_USB = 0x01,
  DS_INPUT_REPORT_USB_SIZE = 64,
  DS_INPUT_REPORT_BT = 0x31,
  DS_INPUT_REPORT_BT_SIZE = 78,
  DS_INPUT_REPORT_BT_MINIMAL_SIZE = 10,
  DS_OUTPUT_REPORT_USB = 0x02,
  DS_OUTPUT_REPORT_USB_SIZE = 48,
  DS_OUTPUT_REPORT_BT = 0x31,
  DS_OUTPUT_REPORT_BT_SIZE = 78,
  DS_FEATURE_REPORT_CALIBRATION = 0x05,
  DS_FEATURE_REPORT_PAIRING_INFO = 0x09,
  DS_FEATURE_REPORT_FIRMWARE_INFO = 0x20,

  DS4_INPUT_REPORT_USB = 0x01,
  DS4_INPUT_REPORT_USB_SIZE = 64,
  DS4_INPUT_REPORT_BT = 0x11,
  DS4_INPUT_REPORT_BT_SIZE = 78,
  DS4_INPUT_REPORT_BT_MINIMAL_SIZE = 10,
  DS4_OUTPUT_REPORT_USB = 0x05,
  DS4_OUTPUT_REPORT_USB_SIZE = 32,
  DS4_OUTPUT_REPORT_BT = 0x11,
  DS4_OUTPUT_REPORT_BT_SIZE = 78,
  DS4_FEATURE_REPORT_CALIBRATION_USB = 0x02,
  DS4_FEATURE_REPORT_CALIBRATION_BT = 0x05,
  DS4_FEATURE_REPORT_PAIRING_INFO = 0x12,
  DS4_FEATURE_REPORT_FIRMWARE_INFO = 0xA3,
};

enum : uint8_t
{
  CRC32_SEED_INPUT = 0xA1,
  CRC32_SEED_OUTPUT = 0xA2,
};

// Bluetooth reports carry a CRC32 over the HID transport byte (the seed) and the report body: report_crc32 covers len
// body bytes, check_report_crc32 takes len report bytes whose last four hold the stored CRC
uint32_t report_crc32(uint8_t seed, const uint8_t *data, int len);
bool check_report_crc32(uint8_t seed, const uint8_t *report, int len);

enum ButtonBit : uint32_t
{
  BTN_SQUARE = 1u << 0,
  BTN_CROSS = 1u << 1,
  BTN_CIRCLE = 1u << 2,
  BTN_TRIANGLE = 1u << 3,
  BTN_L1 = 1u << 4,
  BTN_R1 = 1u << 5,
  BTN_L2 = 1u << 6,
  BTN_R2 = 1u << 7,
  BTN_CREATE = 1u << 8,
  BTN_OPTIONS = 1u << 9,
  BTN_L3 = 1u << 10,
  BTN_R3 = 1u << 11,
  BTN_PS = 1u << 12,
  BTN_TOUCHPAD = 1u << 13,
  BTN_MUTE = 1u << 14,
  BTN_FN_LEFT = 1u << 15,
  BTN_FN_RIGHT = 1u << 16,
  BTN_PADDLE_LEFT = 1u << 17,
  BTN_PADDLE_RIGHT = 1u << 18,
};

enum ChargeState : uint8_t
{
  CHARGE_DISCHARGING,
  CHARGE_CHARGING,
  CHARGE_FULL,
  CHARGE_UNKNOWN
};

struct InputSample
{
  uint32_t buttons = 0;
  uint32_t sensorTicks = 0; // the time between two samples is ((ticks - previous) & sensorTicksMask) * sensorTickUs
  uint32_t sensorTicksMask = 0xFFFFFFFFu;
  float sensorTickUs = 1.f / 3.f;
  int16_t gyroRaw[3] = {0, 0, 0};  // pitch (X), yaw (Y), roll (Z)
  int16_t accelRaw[3] = {0, 0, 0}; // X, Y, Z
  JoystickTouchpadState::Point touch[2];

  uint8_t lx = 0x80, ly = 0x80, rx = 0x80, ry = 0x80;
  uint8_t l2 = 0, r2 = 0;
  uint8_t hat = 8; // 0 up, clockwise to 7 up-left, 8 centered
  bool hasSensors = false;
  bool hasTouch = false;
  bool hasStatus = false;
  uint8_t batteryLevel = 0; // 0..10 in 10% steps
  ChargeState charge = CHARGE_UNKNOWN;
  bool cablePowered = false;
  bool dongleEmpty = false; // DualShock 4 USB adapter with no pad paired
};

// false when the bytes are not an input report of this model and transport, or the CRC fails; out is reset either way.
// A 10-byte report 0x01 is served on both transports: sticks, hat, buttons and triggers, the has* flags stay false
bool parse_input_report(Model model, bool bluetooth, const uint8_t *report, int len, InputSample &out);
int battery_percent(const InputSample &s); // 0..100; -1 without a battery status or with an unknown charge state

struct SensorCalibration
{
  struct Axis
  {
    int bias, numer, denom;
    float apply(int raw) const { return float(raw - bias) * float(numer) / float(denom); }
  };
  Axis gyro[3], accel[3];  // results in 1/GYRO_RES_PER_DEG_S deg/s and 1/ACCEL_RES_PER_G g
  int gyroReportedBias[3]; // what the pad reports, kept for logging; Axis::apply subtracts gyro[i].bias

  SensorCalibration() { setDefaults(); }
  void setDefaults();
  float gyroDegS(int axis, int raw) const { return gyro[axis].apply(raw) / GYRO_RES_PER_DEG_S; }
  float accelG(int axis, int raw) const { return accel[axis].apply(raw) / ACCEL_RES_PER_G; }
};

// the DualShock 4 orders the gyro plus/minus fields differently over Bluetooth; false leaves the defaults in out, and an
// axis with a zero range keeps its default
bool parse_calibration(Model model, const uint8_t *report, int len, SensorCalibration &out);

struct FirmwareInfo
{
  uint32_t hwVersion = 0;
  uint32_t fwVersion = 0;
  uint16_t updateVersion = 0; // DualSense feature version, 0x0215 is 2.21
};
bool parse_firmware_info(Model model, const uint8_t *report, int len, FirmwareInfo &out); // out is reset either way
bool parse_mac_address(Model model, const uint8_t *report, int len, char out[18]);        // out is untouched when false

struct OutputState
{
  uint8_t rumbleLeft = 0;  // low frequency motor
  uint8_t rumbleRight = 0; // high frequency motor
  bool lightbarSet = false;
  uint8_t lightbar[3] = {0, 0, 0};
  bool playerLedsSet = false;
  uint8_t playerLeds = 0; // bit i lights LED i, left to right
  bool micLedSet = false;
  JoystickMicLed micLed = JOY_MIC_LED_OFF;
  TriggerEffectCommand trigger[TRIGGER_EFFECT_TRIGGER_NUM]; // sent with every report: the first one frees a leftover effect
  bool resetLightbar = false; // one shot, the caller clears it after the build: stops the pad's own connect animation
};

// returns the report length written to out, 0 when the model has no output report or cap is short; bt_seq is advanced
// per DualSense Bluetooth report, the DualShock 4 leaves that byte zero. rumble_v2 selects the DualSense's own classic
// rumble (the Edge, and a feature version of 2.21 and up, which the driver reads from FirmwareInfo::updateVersion);
// false selects the compatible mode, which halves the motor values. The DualShock 4 ignores the flag and takes the
// rumble and the lightbar only
int build_output_report(Model model, bool bluetooth, bool rumble_v2, const OutputState &st, uint8_t &bt_seq, uint8_t *out, int cap);

} // namespace sonypad
} // namespace HumanInput
