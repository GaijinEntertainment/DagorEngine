#include "and_sensor.h"
#include "joy_helper_android.h"

#include <humanInput/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <supp/dag_android_native_app_glue.h>
#include <util/dag_simpleString.h>

#include <EASTL/algorithm.h>

#include <android/looper.h>
#include <android/sensor.h>

constexpr int SENSOR_REFRESH_RATE_HZ = 100;
constexpr int32_t SENSOR_REFRESH_PERIOD_US = int32_t(1000000 / SENSOR_REFRESH_RATE_HZ);
extern SimpleString dagor_android_self_pkg_name;


namespace HumanInput
{
namespace
{
struct AndroidResources
{
  ASensorEventQueue *gyroEventQueue = nullptr;
  ASensorManager *sensorManager = nullptr;
  const ASensor *gyroSensor = nullptr;
  ALooper *looper = nullptr;
} android_resources;
} // namespace

void AndroidSensor::cleanupAndroidResources()
{
  debug("android:sensors: cleanup of android resources");
  if (android_resources.gyroEventQueue)
  {
    ASensorEventQueue_disableSensor(android_resources.gyroEventQueue, android_resources.gyroSensor);
    ASensorManager_destroyEventQueue(android_resources.sensorManager, android_resources.gyroEventQueue);
  }
  android_resources = AndroidResources{};
}

const char *AndroidSensor::axisName[AndroidSensor::AXES_NUM] = {
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
  "GyroX",
  "GyroY",
  "GyroZ",
  "stub",
  "stub",
  "stub",
  "stub",
  "stub",
};


AndroidSensor::AndroidSensor()
{
  client = NULL;

  sensorState = 0;
  sensorPendingState = 0;
  failState = false;
  enableGyroscope(::dgs_get_settings()->getBlockByNameEx("input")->getBool("gyroscopeEnabled", false));

  memset(&sensorsPollingData, 0, sizeof(sensorsPollingData));
  memset(&state, 0, sizeof(state));
  memset(accel, 0, sizeof(accel));
  memset(gyro, 0, sizeof(gyro));
  memset(gravity, 0, sizeof(gravity));
  for (int i = 0; i < JoystickRawState::MAX_POV_HATS; i++)
    state.povValues[i] = state.povValuesPrev[i] = -1;

  axis[0].val = &state.x;
  axis[1].val = &state.y;
  axis[2].val = &state.rx;
  axis[3].val = &state.ry;
  axis[4].val = &state.slider[0];
  axis[5].val = &state.slider[1];

  axis[6].val = &accel[0];
  axis[7].val = &accel[1];
  axis[8].val = &accel[2];
  axis[9].val = &gyro[0];
  axis[10].val = &gyro[1];
  axis[11].val = &gyro[2];
  axis[12].val = &gravity[0];
  axis[13].val = &gravity[1];
  axis[14].val = &gravity[2];

  axis[15].val = &gravity[3];
  axis[16].val = &gravity[4];
  for (int i = 0; i < AXES_NUM; i++)
  {
    axis[i].kMul = 1.0f / (float)JOY_ANDSENSOR_MAX_AXIS_VAL;
    axis[i].kAdd = 0.0f;
  }

  sensorsPollingData.gyroValues = gyro;
}


AndroidSensor::~AndroidSensor() { setClient(NULL); }


void AndroidSensor::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


int AndroidSensor::getAxisId(const char *name) const
{
  for (int i = 0; i < AXES_NUM; i++)
    if (strcmp(name, axisName[i]) == 0)
      return i;
  return -1;
}


float AndroidSensor::getAxisPos(int axis_id) const
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  return axis[axis_id].val[0] * axis[axis_id].kMul + axis[axis_id].kAdd;
}


int AndroidSensor::getAxisPosRaw(int axis_id) const
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  return axis[axis_id].val[0];
}


void AndroidSensor::setAxisLimits(int axis_id, float min_val, float max_val)
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  axis[axis_id].kMul = (max_val - min_val) / (2.0 * JOY_ANDSENSOR_MAX_AXIS_VAL);
  axis[axis_id].kAdd = min_val - (float)JOY_ANDSENSOR_MIN_AXIS_VAL * axis[axis_id].kMul;
}


bool AndroidSensor::getDeviceDesc(DataBlock &out_desc) const
{
  out_desc.clearData();
  out_desc.setBool("orientSensor", true);
  return true;
}


static inline int remap_gyro(float v)
{
  const float gyroBaseScale = 0.004f;
  const int val = int(gyroBaseScale * v * JOY_ANDSENSOR_MAX_AXIS_VAL);
  return eastl::clamp(val, (int)JOY_ANDSENSOR_MIN_AXIS_VAL, (int)JOY_ANDSENSOR_MAX_AXIS_VAL);
}


static int get_sensor_events(int fd, int events, void *data)
{
  auto *pollingData = reinterpret_cast<SensorsPollingData *>(data);

  ASensorEvent event;
  float gyroX = 0.f;
  float gyroY = 0.f;
  float gyroZ = 0.f;

  while (ASensorEventQueue_getEvents(android_resources.gyroEventQueue, &event, 1) > 0)
  {
    if (event.type == ASENSOR_TYPE_GYROSCOPE)
    {
      gyroX += event.gyro.x;
      gyroY += event.gyro.y;
      gyroZ += event.gyro.z;
    };
  };

  if (HumanInput::JoyDeviceHelper::getRotation() == HumanInput::JoyDeviceHelper::ROTATION_90)
  {
    gyroX *= -1.0f;
    gyroY *= -1.0f;
  }

  pollingData->gyroValues[0] = remap_gyro(gyroX);
  pollingData->gyroValues[1] = remap_gyro(gyroY);
  pollingData->gyroValues[2] = remap_gyro(gyroZ);

  return 1;
}


void AndroidSensor::registerSensor()
{
  sensorsPollingData.gyroValues = gyro;

  if (android_resources.looper == NULL)
  {
    android_resources.looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
  }

  android_resources.sensorManager = ASensorManager_getInstanceForPackage(dagor_android_self_pkg_name.c_str());
  android_resources.gyroSensor = ASensorManager_getDefaultSensor(android_resources.sensorManager, ASENSOR_TYPE_GYROSCOPE);

  if (android_resources.gyroSensor)
  {
    android_resources.gyroEventQueue = ASensorManager_createEventQueue(android_resources.sensorManager, android_resources.looper,
      ALOOPER_POLL_CALLBACK, get_sensor_events, &sensorsPollingData);

    if (android_resources.gyroEventQueue)
    {
      const int res =
        ASensorEventQueue_registerSensor(android_resources.gyroEventQueue, android_resources.gyroSensor, SENSOR_REFRESH_PERIOD_US, 0);
    }
  }

  if (!android_resources.gyroEventQueue)
  {
    logerr("android:sensors: failed to initialize gyroscope");
    failState = true;
  }
  else
    debug("android:sensors: finished registration");
}


void AndroidSensor::disableGyroscope()
{
  cleanupAndroidResources();
  sensorsPollingData = SensorsPollingData{};
  raw_state_joy.rx = 0;
  raw_state_joy.ry = 0;
  raw_state_joy.rz = 0;
  gyro[0] = 0;
  gyro[1] = 0;
  gyro[2] = 0;
}


bool AndroidSensor::pollSensorsData()
{
  if (!android_resources.gyroEventQueue)
    registerSensor();

  if (android_resources.gyroEventQueue)
  {
    struct android_poll_source *source;
    int events;
    while (ALooper_pollOnce(0, NULL, &events, (void **)&source) >= 0) {}
    return true;
  }

  return false;
}


void AndroidSensor::processSensorPendingState()
{
  const int currentPendingState = interlocked_relaxed_load(sensorPendingState);

  if (currentPendingState == sensorState)
    return;

  const int gyroEnabled = currentPendingState & SENSOR_GYRO_ENABLED_FLAG;
  if (!gyroEnabled)
  {
    disableGyroscope();
    if (client)
      client->stateChanged(this, 0);
  }

  sensorState = currentPendingState;
}


bool AndroidSensor::updateState()
{
  // update state from joystick thread only
  // because we associate looper with it
  if (is_main_thread())
    return false;

  processSensorPendingState();

  if (!(sensorState & SENSOR_GYRO_ENABLED_FLAG))
    return false;

  if (!pollSensorsData())
    return false;

  bool changed = (state.rx != gyro[0] || state.ry != gyro[1] || state.rz != gyro[2]);
  if (changed)
  {
    raw_state_joy.rx = state.rx = gyro[0];
    raw_state_joy.ry = state.ry = gyro[1];
    raw_state_joy.rz = state.rz = gyro[2];

    if (client)
      client->stateChanged(this, 0);
  }

  return changed;
}


void AndroidSensor::enableGyroscope(bool enable)
{
  const bool currentPendingGyroState = (sensorPendingState & SENSOR_GYRO_ENABLED_FLAG) != 0;
  if (currentPendingGyroState != enable)
  {
    const int newOpId = (sensorPendingState >> SENSOR_STATE_BITS) + 1;
    const int gyroEnabled = enable ? 1 : 0;
    const int newPendingState = (newOpId << SENSOR_STATE_BITS) | (gyroEnabled << SENSOR_GYRO_ENABLED_BIT);
    interlocked_relaxed_store(sensorPendingState, newPendingState);
  }
}
} // namespace HumanInput
