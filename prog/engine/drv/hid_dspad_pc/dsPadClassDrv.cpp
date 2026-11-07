// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadClassDrv.h"
#include "dsPadDevice.h"
#include "dsPadSteam.h"
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiGlobals.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <string.h>

#if _TARGET_PC_WIN
#include <supp/_platform.h>
#include <dbt.h>
#endif

using namespace HumanInput;

static constexpr int IO_WAIT_MS = 50;
static constexpr int RESCAN_PERIOD_MS = 3000;
static constexpr int RETRY_SKIPPED_PERIOD_MS = 30000; // Linux delivers no device change event
static constexpr int BT_SILENCE_MS = 2000;            // a pad switched off over bluetooth keeps its read pending
static constexpr int FEATURE_READ_TRIES = 3;

static volatile int steam_remaps_dspad = 0;

bool HumanInput::isDsPadRemappedBySteamInput() { return interlocked_acquire_load(steam_remaps_dspad) != 0; }

class DsPadClassDriver::IoThread final : public DaThread
{
  DsPadClassDriver &drv;

public:
  IoThread(DsPadClassDriver &d) : DaThread("DsPadIO", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), drv(d) {}
  void execute() override { drv.ioLoop(); }
};


DsPadClassDriver::DsPadClassDriver()
{
  prevUpdateRefTime = ::ref_time_ticks();
  add_wnd_proc_component(this);
}

DsPadClassDriver::~DsPadClassDriver()
{
  del_wnd_proc_component(this);
  if (ioThread)
  {
    waitSet.wake();
    ioThread->terminate(true);
  }
  interlocked_release_store(steam_remaps_dspad, 0);
}

bool DsPadClassDriver::init()
{
  ioThread.reset(new IoThread(*this));
  if (!ioThread->start())
    return false;
  enable(true);
  return true;
}

void DsPadClassDriver::destroy()
{
  useDefClient(nullptr);
  setDefaultJoystick(nullptr);
  stg_joy.present = false;
  delete this;
}

void DsPadClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = ::ref_time_ticks();
}

IWndProcComponent::RetCode DsPadClassDriver::process(void *, unsigned msg, uintptr_t wParam, intptr_t, intptr_t &)
{
#if _TARGET_PC_WIN
  if (msg == WM_DEVICECHANGE && wParam == DBT_DEVNODES_CHANGED)
    requestRescan();
#else
  G_UNUSED(msg);
  G_UNUSED(wParam);
#endif
  return PROCEED_OTHER_COMPONENTS;
}

void DsPadClassDriver::requestRescan()
{
  interlocked_release_store(rescanRequested, 1);
  waitSet.wake();
}

// IO thread, cs held: the pad is left quiet for its next reader, the device object stays until the main thread
// refreshes the list
void DsPadClassDriver::loseSlot(Slot &s)
{
  uint8_t buf[256];
  int len = 0;
  if (s.dev->takeOutputReport(buf, sizeof(buf), len, /*quiet*/ true))
    s.hid->write(buf, len);
  s.hid->close();
  s.dev->setLost();
  for (int i = skippedPads.size() - 1; i >= 0; i--)
    if (strcmp(skippedPads[i].path, s.twinPath) == 0)
      erase_items(skippedPads, i, 1); // the bluetooth twin returns with the usb link gone
  interlocked_release_store(configChanged, 1);
}


static int read_feature(dspad::HidDevice &hid, uint8_t id, uint8_t *buf, int cap)
{
  // Steam and other readers share the pad, so a request can return another report; retry like hid-playstation
  for (int t = 0; t < FEATURE_READ_TRIES; t++)
  {
    const int n = hid.getFeature(id, buf, cap);
    if (n > 0 && buf[0] == id)
      return n;
    sleep_msec(2);
  }
  return -1;
}

static void read_calibration(dspad::HidDevice &hid, DsPadDevice &dev)
{
  using namespace dspad;
  const Model model = dev.getModel();
  const bool bt = hid.getInfo().bluetooth;
  uint8_t buf[MAX_REPORT_SIZE];
  const uint8_t id = model == Model::DUALSHOCK4 ? (bt ? DS4_FEATURE_REPORT_CALIBRATION_BT : DS4_FEATURE_REPORT_CALIBRATION_USB)
                                                : DS_FEATURE_REPORT_CALIBRATION;
  SensorCalibration calib;
  const int n = read_feature(hid, id, buf, sizeof(buf));
  bool nonZero = false;
  for (int i = 1; i < n; i++)
    nonZero |= buf[i] != 0;
  const bool ok = nonZero && parse_calibration(model, buf, n, calib);
  if (ok)
  {
    debug("[HID][DSPAD] calibration: gyro numer/denom %d/%d %d/%d %d/%d reported bias %d %d %d, accel bias %d %d %d denom %d %d %d",
      calib.gyro[0].numer, calib.gyro[0].denom, calib.gyro[1].numer, calib.gyro[1].denom, calib.gyro[2].numer, calib.gyro[2].denom,
      calib.gyroReportedBias[0], calib.gyroReportedBias[1], calib.gyroReportedBias[2], calib.accel[0].bias, calib.accel[1].bias,
      calib.accel[2].bias, calib.accel[0].denom, calib.accel[1].denom, calib.accel[2].denom);
  }
  else
    debug("[HID][DSPAD] calibration report 0x%02x not available (%d bytes), using nominal sensor scales", id, n);
  dev.setCalibration(calib, ok);
}

// IO thread: open the pad, read its identity and calibration and queue the device for publishing
bool DsPadClassDriver::openPad(const dspad::HidDeviceInfo &info)
{
  using namespace dspad;
  eastl::unique_ptr<HidDevice> hid(new HidDevice);
  if (!hid->open(info) || !hid->armRead())
  {
    debug("[HID][DSPAD] cannot open %04x:%04x %s", info.vid, info.pid, info.path);
    return false;
  }

  DsPadDevice::InitInfo init;
  init.model = model_from_pid(info.pid);
  init.bluetooth = info.bluetooth;
  uint8_t buf[MAX_REPORT_SIZE];
  const bool ds4 = init.model == Model::DUALSHOCK4;
  // reading any feature report also switches a Bluetooth pad from the 10-byte report to the full one
  int n = read_feature(*hid, ds4 ? DS4_FEATURE_REPORT_PAIRING_INFO : DS_FEATURE_REPORT_PAIRING_INFO, buf, sizeof(buf));
  if (hid->isDead())
  {
    debug("[HID][DSPAD] %04x:%04x %s does not answer, left for a retry", info.vid, info.pid, info.path);
    return false;
  }
  if (!parse_mac_address(init.model, buf, n, init.mac))
    strcpy(init.mac, "unknown");
  n = read_feature(*hid, ds4 ? DS4_FEATURE_REPORT_FIRMWARE_INFO : DS_FEATURE_REPORT_FIRMWARE_INFO, buf, sizeof(buf));
  parse_firmware_info(init.model, buf, n, init.firmware);

  const HidDeviceInfo &opened = hid->getInfo();
  debug("[HID][DSPAD] opened %s over %s: vid:pid=%04x:%04x mac=%s hw=%08x fw=%08x upd=%04x reports in/out/feature=%d/%d/%d %s",
    model_name(init.model), opened.bluetooth ? "bluetooth" : "usb", opened.vid, opened.pid, init.mac, init.firmware.hwVersion,
    init.firmware.fwVersion, init.firmware.updateVersion, opened.inputReportLen, opened.outputReportLen, opened.featureReportLen,
    opened.path);
  eastl::unique_ptr<DsPadDevice> dev(new DsPadDevice(init, nextOrdId++));
  read_calibration(*hid, *dev);

  WinAutoLock lock(cs);
  // the same pad over both links reports twice; the USB link wins, as in SDL, and the swap is one locked change
  if (strcmp(init.mac, "unknown") != 0)
    for (Slot &s : slots)
      if (s.dev->isConnected() && strcmp(s.dev->getMac(), init.mac) == 0)
      {
        if (init.bluetooth)
        {
          debug("[HID][DSPAD] %s over bluetooth skipped, the same pad is on usb", model_name(init.model));
          strcpy(s.twinPath, info.path);
          return false;
        }
        debug("[HID][DSPAD] %s over bluetooth gives way to the usb link", s.dev->getName());
        loseSlot(s);
      }
  Slot &slot = slots.push_back();
  slot.hid = eastl::move(hid);
  slot.dev = eastl::move(dev);
  slot.lastReportTicks = ref_time_ticks();
  return true;
}

template <typename List>
static bool lists_path(const List &list, const char *path)
{
  for (const dspad::HidDeviceInfo &d : list)
    if (strcmp(d.path, path) == 0)
      return true;
  return false;
}

void DsPadClassDriver::scanDevices(bool retry_skipped, bool device_change)
{
  using namespace dspad;
  HidDeviceList all;
  if (!enumerate_hid_devices(all))
  {
    debug("[HID][DSPAD] HID enumeration failed");
    return;
  }

  bool steamRemapsDsPads = steam_input_remaps_vendor(VID_SONY);
#if _TARGET_PC_WIN
  // Steam Input hides the pads it remaps
  for (const HidDeviceInfo &d : all)
    steamRemapsDsPads &= !is_dspad(d.vid, d.pid);
#endif
  if (!scanLogged || steamRemapsDsPads != isDsPadRemappedBySteamInput())
  {
    scanLogged = true;
    debug("[HID][DSPAD] %d HID interfaces, Steam Input remaps dsPads %d", all.size(), steamRemapsDsPads);
  }
  interlocked_release_store(steam_remaps_dspad, steamRemapsDsPads ? 1 : 0);

  {
    WinAutoLock lock(cs);
    for (Slot &s : slots)
      if (s.dev->isConnected() && (steamRemapsDsPads || !lists_path(all, s.hid->getInfo().path)))
      {
        debug("[HID][DSPAD] %s %s (%s)", s.dev->getName(), steamRemapsDsPads ? "left to Steam Input" : "removed",
          s.hid->getInfo().path);
        loseSlot(s);
      }
  }

  // retried on a replug, a device change and every RETRY_SKIPPED_PERIOD_MS
  for (int i = skippedPads.size() - 1; i >= 0; i--)
    if (retry_skipped || !lists_path(all, skippedPads[i].path))
      erase_items(skippedPads, i, 1);

  for (const HidDeviceInfo &d : all)
  {
    if (device_change && d.vid == VID_SONY && !is_dspad(d.vid, d.pid))
      debug("[HID][DSPAD] unknown Sony device %04x:%04x left to the generic driver (%s)", d.vid, d.pid, d.path);
    if (!is_dspad(d.vid, d.pid) || lists_path(skippedPads, d.path))
      continue;
    bool known = false;
    {
      WinAutoLock lock(cs);
      for (Slot &s : slots)
        known |= s.dev->isConnected() && strcmp(d.path, s.hid->getInfo().path) == 0;
    }
    if (known)
      continue;
    // Steam already turns this pad into its virtual Xbox controller, a second reader would double it
    if (steamRemapsDsPads)
      debug("[HID][DSPAD] skip %04x:%04x %s: Steam Input remaps it", d.vid, d.pid, d.path);
    if (steamRemapsDsPads || !openPad(d))
      skippedPads.push_back(d);
  }
}

void DsPadClassDriver::ioLoop()
{
  using namespace dspad;
  uint8_t buf[256];
  int64_t lastScan = 0, lastRetry = 0;
  while (!ioThread->isThreadTerminating())
  {
    const bool rescanAsked = interlocked_exchange(rescanRequested, 0) != 0;
    const bool retrySkipped = rescanAsked || get_time_usec(lastRetry) > RETRY_SKIPPED_PERIOD_MS * 1000;
    if (retrySkipped || get_time_usec(lastScan) > RESCAN_PERIOD_MS * 1000)
    {
      lastScan = ref_time_ticks();
      if (retrySkipped)
        lastRetry = lastScan;
      scanDevices(retrySkipped, rescanAsked);
    }

    HidDevice *waitList[16];
    int waitNum = 0;
    {
      WinAutoLock lock(cs);
      for (Slot &s : slots)
        if (s.dev->isConnected() && waitNum < (int)countof(waitList))
          waitList[waitNum++] = s.hid.get();
    }
    waitSet.wait(waitList, waitNum, IO_WAIT_MS);

    WinAutoLock lock(cs);
    for (Slot &s : slots)
    {
      if (!s.dev->isConnected())
        continue;
      for (;;)
      {
        const int n = s.hid->takeReport(buf, sizeof(buf));
        if (n == 0)
          break;
        if (n < 0)
        {
          debug("[HID][DSPAD] %s read failed, treating as removed", s.dev->getName());
          loseSlot(s);
          break;
        }
        s.lastReportTicks = ref_time_ticks();
        // a pad is published while it reports; the DualShock 4 USB adapter answers the calibration request only then
        if (s.dev->onInputReport(buf, n))
        {
          if (s.dev->hasPad())
            read_calibration(*s.hid, *s.dev);
          interlocked_release_store(configChanged, 1);
        }
        if (!s.hid->armRead())
        {
          debug("[HID][DSPAD] %s read request failed, treating as removed", s.dev->getName());
          loseSlot(s);
          break;
        }
      }
      if (s.dev->isConnected() && s.hid->getInfo().bluetooth && get_time_usec(s.lastReportTicks) > BT_SILENCE_MS * 1000)
      {
        debug("[HID][DSPAD] %s sent no report for %d ms over bluetooth, treating as removed", s.dev->getName(), BT_SILENCE_MS);
        loseSlot(s);
        skippedPads.push_back(s.hid->getInfo()); // the stale interface stays listed, so retry it like a skipped pad
      }
      int len = 0;
      if (s.dev->isConnected() && s.dev->takeOutputReport(buf, sizeof(buf), len) && !s.hid->write(buf, len))
        debug("[HID][DSPAD] %s output report write failed", s.dev->getName());
    }
  }
  WinAutoLock lock(cs);
  for (Slot &s : slots)
    if (s.dev->isConnected())
      loseSlot(s);
}


void DsPadClassDriver::refreshDeviceList()
{
  WinAutoLock lock(cs);
  interlocked_release_store(configChanged, 0);
  for (int i = slots.size() - 1; i >= 0; i--)
  {
    Slot &s = slots[i];
    const bool gone = !s.dev->isConnected();
    if (gone || !s.dev->hasPad())
    {
      // the DualShock 4 USB adapter stays open without a pad, with no device published
      if (defJoy == s.dev.get())
        setDefaultJoystick(nullptr);
      erase_item_by_value(devices, s.dev.get());
      if (gone)
        erase_items(slots, i, 1);
      else
        s.dev->setClient(nullptr);
    }
    else if (find_value_idx(devices, s.dev.get()) < 0)
    {
      for (int st = 0; st < 2; st++)
        s.dev->setStickDeadZoneScale(st, stickDeadZoneScale[st]);
      s.dev->enableGyroscope(gyroEnabled);
      s.dev->setClient(defClient);
      devices.push_back(s.dev.get());
      s.dev->updateState(0, s.dev.get() == defJoy);
      debug("[HID][DSPAD] adding joystick #%d - [%s] '%s' (%d axes, %d buttons)", devices.size() - 1, s.dev->getDeviceID(),
        s.dev->getName(), s.dev->getAxisCount(), s.dev->getButtonCount());
    }
  }
  if (getDeviceCount() > 0)
  {
    stg_joy.present = true;
    stg_joy.enabled = enabled;
    if (!defJoy)
      setDefaultJoystick(devices[0]);
  }
  else
  {
    stg_joy.present = false;
    stg_joy.enabled = false;
  }
  prevUpdateRefTime = ::ref_time_ticks();
}

void DsPadClassDriver::updateDevices()
{
  if (!enabled)
    return;
  const int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  for (DsPadDevice *dev : devices)
    if (dev->updateState(dt, dev == defJoy) && !defJoy && enableAutoDef)
    {
      setDefaultJoystick(dev);
      if (dev->getClient())
        dev->getClient()->stateChanged(dev, dev->getUserId());
    }
}

IGenJoystick *DsPadClassDriver::getDevice(int idx) const { return idx >= 0 && idx < devices.size() ? devices[idx] : nullptr; }

IGenJoystick *DsPadClassDriver::getDeviceByUserId(unsigned short user_id) const
{
  for (DsPadDevice *dev : devices)
    if (dev->getUserId() == user_id)
      return dev;
  return nullptr;
}

void DsPadClassDriver::useDefClient(IGenJoystickClient *cli)
{
  defClient = cli;
  for (DsPadDevice *dev : devices)
    dev->setClient(cli);
}

void DsPadClassDriver::setDefaultJoystick(IGenJoystick *ref)
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

float DsPadClassDriver::getStickDeadZoneScale(int stick_idx, bool) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneScale[stick_idx];
}

void DsPadClassDriver::setStickDeadZoneScale(int stick_idx, bool main_dev, float scale)
{
  if (!main_dev)
    return;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
  for (DsPadDevice *dev : devices)
    dev->setStickDeadZoneScale(stick_idx, scale);
}

float DsPadClassDriver::getStickDeadZoneAbs(int stick_idx, bool main_dev, IGenJoystick *for_joy) const
{
  if (!main_dev)
    return 0;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneScale[stick_idx] * DsPadDevice::DEFAULT_STICK_DEADZONE / STICK_MAX_VAL;
}

void DsPadClassDriver::enableGyroscope(bool enable, bool main_dev)
{
  if (!main_dev)
    return;
  gyroEnabled = enable;
  for (DsPadDevice *dev : devices)
    dev->enableGyroscope(enable);
}
