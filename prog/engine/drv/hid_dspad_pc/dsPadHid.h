// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tab.h>

// Raw HID access to one gamepad: Windows hid.dll with overlapped reads, Linux hidraw.
// The driver IO thread owns the devices, waits on up to 16 connected ones inside HidWaitSet::wait and drains every
// one after each wake; other threads only call HidWaitSet::wake() and destroy a device the IO thread has closed,
// under the driver lock.
namespace HumanInput
{
namespace sonypad
{

struct HidDeviceInfo
{
  uint16_t vid = 0, pid = 0;
  bool bluetooth = false;
  int inputReportLen = 0, outputReportLen = 0, featureReportLen = 0; // from the descriptor on Windows, zero elsewhere
  char path[512] = {};
};

// every HID interface the process can see, with vid and pid; false when the system call failed
bool enumerate_hid_devices(Tab<HidDeviceInfo> &out);

class HidDevice
{
public:
  HidDevice() = default;
  ~HidDevice() { close(); }
  HidDevice(const HidDevice &) = delete;
  HidDevice &operator=(const HidDevice &) = delete;

  bool open(const HidDeviceInfo &info);
  void close();
  const HidDeviceInfo &getInfo() const { return info; }

  // Windows starts the next overlapped read here, hidraw reads inside takeReport; false when the device is gone
  bool armRead();
  // bytes of a finished read (report id first), 0 while it is pending, <0 when the device is gone
  int takeReport(uint8_t *buf, int cap);
  // a complete output report, report id first; Windows pads it with zeros to the descriptor length
  bool write(const uint8_t *data, int len);
  // feature report by id into buf; returns the byte count (report id first) or <0
  int getFeature(uint8_t report_id, uint8_t *buf, int cap);
  // a request was left to the kernel: the link is gone, and every later request fails at once
  bool isDead() const { return dead; }

#if _TARGET_PC_WIN
  void *getWaitHandle() const { return readEvent; }
#else
  int getFd() const { return fd; }
#endif

private:
  HidDeviceInfo info;
#if _TARGET_PC_WIN
  void *handle = nullptr;
  void *readEvent = nullptr;
  void *writeEvent = nullptr;
  struct IoBlock *readIo = nullptr; // the pending read; left to the kernel when the cancel does not end it
  bool readPending = false;
  IoBlock *finishIo(IoBlock *io, bool started, unsigned long &n);
#else
  int fd = -1;
#endif
  bool dead = false;
};

// wakes the IO thread when any device read completes or wake() is called
class HidWaitSet
{
public:
  HidWaitSet();
  ~HidWaitSet();
  void wake();
  // returns on a finished read, a wake() or the timeout
  void wait(HidDevice *const *devices, int count, int timeout_ms);

private:
#if _TARGET_PC_WIN
  void *wakeEvent = nullptr;
#else
  int wakeFd[2] = {-1, -1};
#endif
};

} // namespace sonypad
} // namespace HumanInput
