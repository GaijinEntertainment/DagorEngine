// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadHid.h"
#include <supp/_platform.h>
#include <hidsdi.h>
#include <winioctl.h>
#include <cfgmgr32.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_unicode.h>
#include <util/dag_globDef.h>
#include <math/dag_mathBase.h>
#include <string.h>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace HumanInput
{
namespace sonypad
{

// Bluetooth HID interfaces carry the HID service class uuid or the bthenum enumerator
static bool is_bluetooth_path(const char *path)
{
  return dd_stristr(path, "{00001124-0000-1000-8000-00805f9b34fb}") || dd_stristr(path, "bthenum") || dd_stristr(path, "bthledevice");
}

static bool query_caps(HANDLE h, HidDeviceInfo &info)
{
  PHIDP_PREPARSED_DATA pp = nullptr;
  if (!HidD_GetPreparsedData(h, &pp))
    return false;
  HIDP_CAPS caps;
  const bool ok = HidP_GetCaps(pp, &caps) == HIDP_STATUS_SUCCESS;
  HidD_FreePreparsedData(pp);
  if (!ok)
    return false;
  info.inputReportLen = caps.InputReportByteLength;
  info.outputReportLen = caps.OutputReportByteLength;
  info.featureReportLen = caps.FeatureReportByteLength;
  return true;
}

bool enumerate_hid_devices(Tab<HidDeviceInfo> &out)
{
  out.clear();
  GUID hidGuid;
  HidD_GetHidGuid(&hidGuid);
  ULONG len = 0;
  if (CM_Get_Device_Interface_List_SizeW(&len, &hidGuid, nullptr, CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS || len < 2)
    return false;
  Tab<wchar_t> list;
  list.resize(len);
  if (CM_Get_Device_Interface_ListW(&hidGuid, nullptr, list.data(), len, CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS)
    return false;

  for (const wchar_t *path = list.data(); *path; path += wcslen(path) + 1)
  {
    // access 0 opens keyboards and mice too, which refuse read or write access
    HANDLE h = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
      continue;
    HIDD_ATTRIBUTES attr;
    attr.Size = sizeof(attr);
    if (HidD_GetAttributes(h, &attr))
    {
      HidDeviceInfo &d = out.push_back();
      d.vid = attr.VendorID;
      d.pid = attr.ProductID;
      wcs_to_utf8(path, d.path, sizeof(d.path));
      d.bluetooth = is_bluetooth_path(d.path);
    }
    CloseHandle(h);
  }
  return true;
}

static constexpr DWORD IO_TIMEOUT_MS = 500;
static constexpr int IO_BUF_SIZE = 547; // HIDCLASS needs full-length buffers, 547 over Bluetooth
// IOCTL_HID_GET_FEATURE = HID_OUT_CTL_CODE(100) of hidclass.h (Windows SDK 8.1; 0x000B0192), a
// header the SDK of the build machines does not carry
static constexpr DWORD IOCTL_HID_GET_FEATURE_CODE = CTL_CODE(FILE_DEVICE_KEYBOARD, 100, METHOD_OUT_DIRECT, FILE_ANY_ACCESS);

// a request to a dead bluetooth link never completes, so its block is left to the kernel then
struct IoBlock
{
  OVERLAPPED ov;
  uint8_t buf[IO_BUF_SIZE];
};

// IO thread only, the thread that opens and closes a device (a closed one is destroyed on the main thread and leaves the
// list alone): the blocks the kernel kept past a cancel, freed once their request ends
static Tab<IoBlock *> abandoned_blocks;

static void reclaim_io()
{
  for (int i = abandoned_blocks.size() - 1; i >= 0; i--)
    if (HasOverlappedIoCompleted(&abandoned_blocks[i]->ov))
    {
      delete abandoned_blocks[i];
      erase_items(abandoned_blocks, i, 1);
    }
}

// the request state decides, since the auto-reset event can hold a stale signal
static bool wait_io(const OVERLAPPED &ov, DWORD timeout_ms)
{
  const ULONGLONG start = GetTickCount64();
  while (!HasOverlappedIoCompleted(&ov))
  {
    const ULONGLONG passed = GetTickCount64() - start;
    if (passed >= timeout_ms)
      return false;
    WaitForSingleObject(ov.hEvent, DWORD(timeout_ms - passed));
  }
  return true;
}

// cancels a request that did not end in time and waits once more; true when the kernel keeps the block, abandoned then
static bool give_up_io(HANDLE handle, IoBlock *io, const char *what, const char *path)
{
  CancelIoEx(handle, &io->ov);
  if (wait_io(io->ov, IO_TIMEOUT_MS))
    return false;
  debug("[HID][SONY] %s %s did not end in %u ms after the cancel, its block is left to the kernel", what, path, IO_TIMEOUT_MS);
  abandoned_blocks.push_back(io);
  return true;
}

bool HidDevice::open(const HidDeviceInfo &dev_info)
{
  close();
  info = dev_info;
  wchar_t wpath[512];
  if (MultiByteToWideChar(CP_UTF8, 0, info.path, -1, wpath, countof(wpath)) <= 0)
    return false;
  HANDLE h = CreateFileW(wpath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
    FILE_FLAG_OVERLAPPED, nullptr);
  if (h == INVALID_HANDLE_VALUE)
  {
    debug("[HID][SONY] CreateFile(%s) failed, err=%u", info.path, GetLastError());
    return false;
  }
  if (!query_caps(h, info))
  {
    CloseHandle(h);
    return false;
  }
  HidD_SetNumInputBuffers(h, 64);
  handle = h;
  readEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  writeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  return true;
}

void HidDevice::close()
{
  if (!handle)
    return;
  reclaim_io();
  if (readPending && give_up_io(handle, readIo, "the read of", info.path))
    readIo = nullptr;
  CloseHandle(handle);
  CloseHandle(readEvent);
  CloseHandle(writeEvent);
  delete readIo;
  handle = readEvent = writeEvent = nullptr;
  readIo = nullptr;
  readPending = dead = false;
}

bool HidDevice::armRead()
{
  if (!handle || dead || readPending)
    return handle && !dead;
  if (!readIo)
    readIo = new IoBlock{};
  readIo->ov = OVERLAPPED{};
  readIo->ov.hEvent = readEvent;
  const DWORD want = DWORD(clamp(info.inputReportLen, 1, IO_BUF_SIZE));
  if (!ReadFile(handle, readIo->buf, want, nullptr, &readIo->ov) && GetLastError() != ERROR_IO_PENDING)
  {
    debug("[HID][SONY] ReadFile(%s) failed, err=%u", info.path, GetLastError());
    return false;
  }
  readPending = true;
  return true;
}

int HidDevice::takeReport(uint8_t *buf, int cap)
{
  if (!handle)
    return -1;
  if (!readPending)
    return 0;
  DWORD n = 0;
  if (!GetOverlappedResult(handle, &readIo->ov, &n, FALSE))
    return GetLastError() == ERROR_IO_INCOMPLETE ? 0 : -1;
  readPending = false;
  const int len = min<int>(int(n), cap);
  memcpy(buf, readIo->buf, len);
  return len;
}

static IoBlock *begin_io(HANDLE event, const uint8_t *data, int len)
{
  reclaim_io();
  IoBlock *io = new IoBlock{};
  io->ov.hEvent = event;
  memcpy(io->buf, data, len);
  ResetEvent(event);
  return io;
}

// the completed block, to free after reading, or nullptr: a failed start is freed, a request the kernel keeps past the
// cancel marks the link dead
IoBlock *HidDevice::finishIo(IoBlock *io, bool started, DWORD &n)
{
  if (!started)
  {
    delete io;
    return nullptr;
  }
  if (!wait_io(io->ov, IO_TIMEOUT_MS) && give_up_io(handle, io, "a request to", info.path))
  {
    dead = true;
    return nullptr;
  }
  if (GetOverlappedResult(handle, &io->ov, &n, FALSE))
    return io;
  delete io;
  return nullptr;
}

bool HidDevice::write(const uint8_t *data, int len)
{
  const int total = max(len, info.outputReportLen);
  if (!handle || dead || total > IO_BUF_SIZE)
    return false;
  IoBlock *io = begin_io(writeEvent, data, len);
  const bool started = WriteFile(handle, io->buf, total, nullptr, &io->ov) || GetLastError() == ERROR_IO_PENDING;
  DWORD n = 0;
  IoBlock *done = finishIo(io, started, n);
  const bool written = done && n == (DWORD)total;
  delete done;
  return written;
}

int HidDevice::getFeature(uint8_t report_id, uint8_t *buf, int cap)
{
  const int len = info.featureReportLen ? info.featureReportLen : cap;
  if (!handle || dead || len > IO_BUF_SIZE)
    return -1;
  IoBlock *io = begin_io(writeEvent, &report_id, 1);
  const bool started = DeviceIoControl(handle, IOCTL_HID_GET_FEATURE_CODE, io->buf, len, io->buf, len, nullptr, &io->ov) ||
                       GetLastError() == ERROR_IO_PENDING;
  DWORD n = 0;
  IoBlock *done = finishIo(io, started, n);
  if (!done)
    return -1;
  const int copy = min<int>(int(n), cap);
  memcpy(buf, done->buf, copy);
  delete done;
  return copy;
}

HidWaitSet::HidWaitSet() { wakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr); }
HidWaitSet::~HidWaitSet() { CloseHandle(wakeEvent); }
void HidWaitSet::wake() { SetEvent(wakeEvent); }

void HidWaitSet::wait(HidDevice *const *devices, int count, int timeout_ms)
{
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  int n = 0;
  handles[n++] = wakeEvent;
  for (int i = 0; i < count && n < MAXIMUM_WAIT_OBJECTS; i++)
    if (devices[i]->getWaitHandle())
      handles[n++] = devices[i]->getWaitHandle();
  WaitForMultipleObjects(n, handles, FALSE, timeout_ms);
}

} // namespace sonypad
} // namespace HumanInput
