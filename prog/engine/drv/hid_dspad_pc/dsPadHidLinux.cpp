// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadHid.h"
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_globDef.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/input.h>

namespace HumanInput
{
namespace sonypad
{

// sysfs shows the ids without node access
bool enumerate_hid_devices(Tab<HidDeviceInfo> &out)
{
  out.clear();
  DIR *dir = opendir("/sys/class/hidraw");
  if (!dir)
    return false;
  while (dirent *e = readdir(dir))
  {
    if (strncmp(e->d_name, "hidraw", 6) != 0)
      continue;
    char uevent[320];
    snprintf(uevent, sizeof(uevent), "/sys/class/hidraw/%s/device/uevent", e->d_name);
    file_ptr_t f = df_open(uevent, DF_READ | DF_IGNORE_MISSING);
    if (!f)
      continue;
    unsigned bus = 0, vid = 0, pid = 0;
    char line[256];
    while (df_gets(line, sizeof(line), f))
      if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &vid, &pid) == 3)
        break;
    df_close(f);
    HidDeviceInfo &d = out.push_back();
    snprintf(d.path, sizeof(d.path), "/dev/%s", e->d_name);
    d.vid = uint16_t(vid);
    d.pid = uint16_t(pid);
    d.bluetooth = bus == BUS_BLUETOOTH;
  }
  closedir(dir);
  return true;
}

bool HidDevice::open(const HidDeviceInfo &dev_info)
{
  close();
  info = dev_info;
  fd = ::open(info.path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0)
    debug("[HID][SONY] open(%s) failed: %s (add the Valve 60-steam-input.rules udev file for hidraw access)", info.path,
      strerror(errno));
  return fd >= 0;
}

void HidDevice::close()
{
  if (fd >= 0)
    ::close(fd);
  fd = -1;
}

bool HidDevice::armRead() { return fd >= 0; }

int HidDevice::takeReport(uint8_t *buf, int cap)
{
  if (fd < 0)
    return -1;
  const int n = ::read(fd, buf, cap);
  if (n < 0)
    return (errno == EAGAIN || errno == EINTR) ? 0 : -1;
  return n;
}

bool HidDevice::write(const uint8_t *data, int len) { return fd >= 0 && ::write(fd, data, len) == len; }

int HidDevice::getFeature(uint8_t report_id, uint8_t *buf, int cap)
{
  if (fd < 0 || cap < 1)
    return -1;
  buf[0] = report_id;
  return ioctl(fd, HIDIOCGFEATURE(cap), buf);
}

HidWaitSet::HidWaitSet()
{
  if (pipe2(wakeFd, O_NONBLOCK | O_CLOEXEC) != 0)
    debug("[HID][SONY] wake pipe failed: %s", strerror(errno));
}

HidWaitSet::~HidWaitSet()
{
  for (int f : wakeFd)
    if (f >= 0)
      ::close(f);
}

void HidWaitSet::wake()
{
  const char byte = 1;
  if (wakeFd[1] >= 0)
  {
    // A full pipe means a wake is already pending; gcc ignores a (void) cast here.
    [[maybe_unused]] const ssize_t written = ::write(wakeFd[1], &byte, 1);
  }
}

void HidWaitSet::wait(HidDevice *const *devices, int count, int timeout_ms)
{
  pollfd fds[64];
  int n = 0;
  fds[n++] = {wakeFd[0], POLLIN, 0};
  for (int i = 0; i < count && n < (int)countof(fds); i++)
    if (devices[i]->getFd() >= 0)
      fds[n++] = {devices[i]->getFd(), POLLIN, 0};
  const int r = poll(fds, n, timeout_ms);
  if (r > 0 && (fds[0].revents & POLLIN))
  {
    char drain[16];
    while (::read(wakeFd[0], drain, sizeof(drain)) > 0)
      ;
  }
}

} // namespace sonypad
} // namespace HumanInput
