// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_android.h"
#include "and_sensor.h"
#include "joy_acc_gyro_classdrv.h"
#include "joy_acc_gyro_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include "joy_helper_android.h"
#include <string.h>
#include <startup/dag_globalSettings.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_mutex.h>
#include <android/input.h>
#include <EASTL/array.h>
#include <atomic>

static std::atomic<int> gamepad_device_id = HumanInput::GAMEPAD_DEVICE_ID_NONE;
static int pressed_gamepad_device_id = HumanInput::GAMEPAD_DEVICE_ID_NONE;

int HumanInput::getGamepadDeviceId_android() { return gamepad_device_id.load(std::memory_order_relaxed); }

namespace
{
struct RecordedGamepad
{
  int deviceId = HumanInput::GAMEPAD_DEVICE_ID_NONE;
  uint32_t lastEvent = 0;
  bool hasJoystickAxes = false;
  HumanInput::AndroidGamepadInput input;
  HumanInput::AndroidGamepadInput::KeyCodes pressedKeyCodes;
};
} // namespace

static dag::mutex gamepad_input_mutex;
static bool gamepad_input_enabled = false;
static eastl::array<RecordedGamepad, 8> recorded_gamepads;
static uint32_t recorded_event_counter = 0;

static void reset_gamepad_input(bool enabled)
{
  dag::lock_guard lock(gamepad_input_mutex);
  gamepad_input_enabled = enabled;
  gamepad_device_id = HumanInput::GAMEPAD_DEVICE_ID_NONE;
  pressed_gamepad_device_id = HumanInput::GAMEPAD_DEVICE_ID_NONE;
  recorded_gamepads = {};
  recorded_event_counter = 0;
}

static RecordedGamepad &record_gamepad(int device_id)
{
  const int currentDeviceId = gamepad_device_id.load(std::memory_order_relaxed);
  RecordedGamepad *oldest = nullptr;
  for (RecordedGamepad &record : recorded_gamepads)
  {
    if (record.deviceId == device_id)
      return record;
    const bool current = currentDeviceId != HumanInput::GAMEPAD_DEVICE_ID_NONE && record.deviceId == currentDeviceId;
    if (!current && (!oldest || record.lastEvent < oldest->lastEvent))
      oldest = &record;
  }
  *oldest = RecordedGamepad{};
  oldest->deviceId = device_id;
  return *oldest;
}

// called on the input thread for each input event
static void on_android_input_event(const AInputEvent *event)
{
  const int32_t type = AInputEvent_getType(event);
  const int32_t source = AInputEvent_getSource(event);
  const bool gamepadKey = type == AINPUT_EVENT_TYPE_KEY && (source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD;
  const bool joystickMotion = type == AINPUT_EVENT_TYPE_MOTION && (source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK;
  if (!gamepadKey && !joystickMotion)
    return;

  const int imeStatus = HumanInput::getScreenKeyboardStatus_android();
  const int deviceId = AInputEvent_getDeviceId(event);
  if (deviceId < 0)
    return;

  dag::lock_guard lock(gamepad_input_mutex);
  if (!gamepad_input_enabled)
    return;

  if (joystickMotion)
  {
    if (imeStatus != 0)
      return;
    RecordedGamepad &record = record_gamepad(deviceId);
    record.lastEvent = ++recorded_event_counter;
    record.hasJoystickAxes = true;
    for (int i = 0; i < HumanInput::AndroidGamepadInput::AXIS_COUNT; i++)
      record.input.axes[i] = AMotionEvent_getAxisValue(event, HumanInput::AndroidGamepadInput::ANDROID_AXES[i], 0);
    return;
  }

  const int32_t keyCode = AKeyEvent_getKeyCode(event);
  const int32_t action = AKeyEvent_getAction(event);
  if (keyCode < 0 || keyCode >= int32_t(HumanInput::AndroidGamepadInput::KeyCodes::kSize) ||
      (action == AKEY_EVENT_ACTION_DOWN && imeStatus == 1))
    return;

  RecordedGamepad &record = record_gamepad(deviceId);
  record.lastEvent = ++recorded_event_counter;
  if (action == AKEY_EVENT_ACTION_UP)
    record.input.keyCodes.reset(keyCode);
  else if (action == AKEY_EVENT_ACTION_DOWN)
  {
    record.input.keyCodes.set(keyCode);
    record.pressedKeyCodes.set(keyCode);

    if (AKeyEvent_getRepeatCount(event) == 0 && gamepad_device_id.load(std::memory_order_relaxed) != deviceId)
    {
      if (record.hasJoystickAxes)
        gamepad_device_id.store(deviceId, std::memory_order_relaxed);
      pressed_gamepad_device_id = deviceId;
    }
  }
}

void HumanInput::read_android_gamepad_input(AndroidGamepadInput &out_input)
{
  out_input = AndroidGamepadInput{};
  dag::lock_guard lock(gamepad_input_mutex);
  const int deviceId = gamepad_device_id.load(std::memory_order_relaxed);
  for (RecordedGamepad &record : recorded_gamepads)
  {
    if (deviceId != GAMEPAD_DEVICE_ID_NONE && record.deviceId == deviceId)
    {
      out_input = record.input;
      out_input.keyCodes |= record.pressedKeyCodes;
    }
    else if (record.deviceId == pressed_gamepad_device_id)
      continue;
    record.pressedKeyCodes.reset();
  }
}

static bool gamepad_exist_in_block(const DataBlock *blk, int vid, int pid, const char *blockName)
{
  if (const DataBlock *layoutBlk = blk->getBlockByName(blockName))
  {
    for (int blockNo = 0; blockNo < layoutBlk->blockCount(); blockNo++)
    {
      const DataBlock *configBlk = layoutBlk->getBlock(blockNo);
      int v = configBlk->getInt("vendor", -1);
      int p = configBlk->getInt("product", -1);
      if (v == vid && (p == pid || p == -1))
        return true;
    }
  }
  return false;
}

static int get_gamepad_layout(int vendor, int product)
{
  if (vendor == HumanInput::GAMEPAD_VENDOR_NINTENDO || vendor == HumanInput::GAMEPAD_VENDOR_SONY ||
      vendor == HumanInput::GAMEPAD_VENDOR_MICROSOFT)
    return vendor;

  const DataBlock *modelsBlk = dgs_get_settings()->getBlockByNameEx("gamepad_layout");
  if (modelsBlk->isEmpty())
  {
    logdbg("gamepad vendors list are not found");
    return HumanInput::GAMEPAD_VENDOR_UNKNOWN;
  }
  logdbg("gamepad: searching gamepad layouts for device vendor %d product %d", vendor, product);

  const int res = gamepad_exist_in_block(modelsBlk, vendor, product, "nintendo") ? HumanInput::GAMEPAD_VENDOR_NINTENDO
                  : gamepad_exist_in_block(modelsBlk, vendor, product, "sony")   ? HumanInput::GAMEPAD_VENDOR_SONY
                  : gamepad_exist_in_block(modelsBlk, vendor, product, "xbox")   ? HumanInput::GAMEPAD_VENDOR_MICROSOFT
                                                                                 : HumanInput::GAMEPAD_VENDOR_UNKNOWN;

  if (res != HumanInput::GAMEPAD_VENDOR_UNKNOWN)
  {
    logdbg("gamepad: found layout - %d", res);
  }
  return res;
}


HumanInput::JoyAccelGyroInpClassDriver::JoyAccelGyroInpClassDriver()
{
  enabled = false;
  defClient = nullptr;
  sensor = nullptr;
  device = nullptr;
  defJoy = nullptr;
  vendorId = GAMEPAD_VENDOR_UNKNOWN;
}

HumanInput::JoyAccelGyroInpClassDriver::~JoyAccelGyroInpClassDriver() { destroyDevices(); }

bool HumanInput::JoyAccelGyroInpClassDriver::init()
{
  stg_joy.enabled = false;
  reset_gamepad_input(true);
  if (!android::add_input_callback(&on_android_input_event))
    logwarn("gamepad: no free input callback slot, the gamepad device gets no input");
  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

void HumanInput::JoyAccelGyroInpClassDriver::destroyGamepad()
{
  if (device != sensor)
    delete device;
  device = nullptr;
}

void HumanInput::JoyAccelGyroInpClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  destroyGamepad();
  del_it(sensor);
}

void HumanInput::JoyAccelGyroInpClassDriver::refreshDeviceList()
{
  JoyDeviceHelper::clearDeviceChanges();
  setDefaultJoystick(NULL);
  enable(false);
  vendorId = GAMEPAD_VENDOR_UNKNOWN;

  if (!sensor)
    sensor = new (inimem) AndroidSensor;
  else
    sensor->allowRegistrationRetry();

  int gamepadDeviceId = GAMEPAD_DEVICE_ID_NONE, vendor = GAMEPAD_VENDOR_UNKNOWN, product = GAMEPAD_VENDOR_UNKNOWN;
  const bool gamepadConnected = JoyDeviceHelper::getConnectedGamepadIds(gamepadDeviceId, vendor, product);
  {
    dag::lock_guard lock(gamepad_input_mutex);
    if (pressed_gamepad_device_id == GAMEPAD_DEVICE_ID_NONE)
      gamepad_device_id = gamepadDeviceId;
  }
  if (gamepadConnected)
  {
    vendorId = get_gamepad_layout(vendor, product);
    if (!device || device == sensor)
    {
      device = new (inimem) JoyAccelGyroInpDevice(sensor);
      sensor->setClient(nullptr);
    }
    static_cast<JoyAccelGyroInpDevice *>(device)->setVendorId(vendorId);
  }
  else
  {
    destroyGamepad();
    device = sensor;
  }
  defJoy = device;

  stg_joy.present = true;
  enable(true);
  useDefClient(defClient);
}

// generic hid class driver interface
void HumanInput::JoyAccelGyroInpClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
}
void HumanInput::JoyAccelGyroInpClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;
  android::del_input_callback(&on_android_input_event);
  reset_gamepad_input(false);

  JoyDeviceHelper::detach();

  delete this;
}

void HumanInput::JoyAccelGyroInpClassDriver::updateDevices()
{
  int pressedDeviceId;
  {
    dag::lock_guard lock(gamepad_input_mutex);
    pressedDeviceId = pressed_gamepad_device_id;
  }
  if (pressedDeviceId != GAMEPAD_DEVICE_ID_NONE)
  {
    int currentDeviceId = pressedDeviceId, vendor, product;
    if (!JoyDeviceHelper::selectGamepad(pressedDeviceId) && !JoyDeviceHelper::getConnectedGamepadIds(currentDeviceId, vendor, product))
      currentDeviceId = GAMEPAD_DEVICE_ID_NONE;

    dag::lock_guard lock(gamepad_input_mutex);
    if (pressed_gamepad_device_id == pressedDeviceId)
    {
      pressed_gamepad_device_id = GAMEPAD_DEVICE_ID_NONE;
      gamepad_device_id = currentDeviceId;
    }
  }

  if (sensor && sensor->isConnected())
    sensor->updateState();
  if (device && device != sensor)
    device->updateState();
}

// generic joystick class driver interface
HumanInput::IGenJoystick *HumanInput::JoyAccelGyroInpClassDriver::getDevice(int idx) const { return idx == 0 ? device : nullptr; }

void HumanInput::JoyAccelGyroInpClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}

HumanInput::IGenJoystick *HumanInput::JoyAccelGyroInpClassDriver::getDeviceByUserId(unsigned short userId) const
{
  return device->getUserId() == userId ? device : nullptr;
}

bool HumanInput::JoyAccelGyroInpClassDriver::isDeviceConfigChanged() const { return JoyDeviceHelper::isDeviceChangePending(); }

void HumanInput::JoyAccelGyroInpClassDriver::setDefaultJoystick(IGenJoystick *ref)
{
  if (ref != defJoy)
  {
    if (ref)
      raw_state_joy = ref->getRawState();
    else
      memset(&raw_state_joy, 0, sizeof(raw_state_joy));
  }

  defJoy = ref;
}

void HumanInput::JoyAccelGyroInpClassDriver::enableGyroscope(bool enable, bool)
{
  if (sensor)
    sensor->enableGyroscope(enable);
}

int HumanInput::JoyAccelGyroInpClassDriver::getVendorId() const { return vendorId; }
