// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "joy_android.h"

#include <math/dag_Point3.h>
#include <util/dag_globDef.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <string.h>

#include <atomic>

struct ASensorEventQueue;
struct ASensorManager;
struct ASensor;
struct ALooper;

namespace HumanInput
{
enum
{
  JOY_ANDSENSOR_MIN_AXIS_VAL = -32000,
  JOY_ANDSENSOR_MAX_AXIS_VAL = 32000
};

struct SensorsPollingData
{
  struct PendingData
  {
    Point3 values;
    bool hasEvents;
  };

  std::atomic<PendingData> pendingGyro;
  std::atomic<PendingData> pendingGravity;
};

/*
  AndroidSensor mimicks JoyAccelGyroInpDevice
  thus it has all 17 axis
  but in fact uses only 3 of them: GyroX GyroY GyroZ.
  It allows us to use the same axis bindings for android sensor and joystick

  It is expected to register android sensor
  from joystick thread because event queue will be associated with it
*/
class AndroidSensor final : public IAndroidJoystick
{
  constexpr static int AXES_NUM = 11 + 6;

public:
  AndroidSensor();
  virtual ~AndroidSensor() override;

  bool updateState() override;

  // IGenJoystick interface implementation
  const char *getName() const override { return "android-sensor"; }
  const char *getDeviceID() const override { return "and:gamepad-sensor"; }
  bool getDeviceDesc(DataBlock &out_desc) const override;
  unsigned short getUserId() const override { return 0; }
  const JoystickRawState &getRawState() const override { return state; }

  void setClient(IGenJoystickClient *cli) override;
  IGenJoystickClient *getClient() const override { return client; }

  int getButtonCount() const override { return 0; }
  const char *getButtonName(int idx) const override { return ""; }
  int getButtonId(const char *name) const override { return -1; }
  bool getButtonState(int btn_id) const override { return false; }

  int getAxisCount() const override { return AXES_NUM; }
  const char *getAxisName(int idx) const override { return axisName[idx]; }
  int getAxisId(const char *name) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;
  void setAxisLimits(int axis_id, float min_val, float max_val) override;

  int getPovHatCount() const override { return 0; }
  const char *getPovHatName(int idx) const override { return NULL; }
  int getPovHatId(const char *name) const override { return -1; }
  int getPovHatAngle(int axis_id) const override { return -1; }

  bool isConnected() override { return !failState; }

  void enableGyroscope(bool enable) override;
  static void cleanupAndroidResources();

private:
  void registerSensor();
  void disableGyroscope();
  bool pollSensorsData();
  void processSensorPendingState();

private:
  IGenJoystickClient *client;
  JoystickRawState state;

  enum
  {
    SENSOR_GYRO_ENABLED_FLAG = 1,
    SENSOR_GYRO_ENABLED_BIT = 0,

    SENSOR_STATE_FLAGS_MASK = 1,
    SENSOR_STATE_BITS = 1
  };

  // processed ops | state flags
  int sensorState;
  // pending ops | state flags
  int sensorPendingState;

  bool failState;

  SensorsPollingData sensorsPollingData;

  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];
  int accel[3], gyro[3], gravity[5];

  static const char *axisName[AXES_NUM];
};
} // namespace HumanInput
