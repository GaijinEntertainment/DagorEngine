//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sys/socket.h> // Per msqr
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>

namespace apple
{

namespace hardware
{

/*
 Platforms

 iFPGA ->        ??

 iPhone5,1 ->    iPhone Next Gen, TBD
 iPhone5,1 ->    iPhone Next Gen, TBD
 iPhone5,1 ->    iPhone Next Gen, TBD

 iPod3,1   ->    iPod touch 3G, N18
 iPod4,1   ->    iPod touch 4G, N80

 iPad4,1   ->    (iPad 4G, WiFi)
 iPad4,2   ->    (iPad 4G, GSM)
 iPad4,3   ->    (iPad 4G, CDMA)

 AppleTV6,1 ->   AppleTV 4, Full HD
 AppleTV6,2 ->   AppleTV 4, 4k
*/

#define IFPGA_NAMESTRING "iFPGA"

#define IPHONE_5_NAMESTRING       "iPhone 5"
#define IPHONE_UNKNOWN_NAMESTRING "Unknown iPhone"

#define IPOD_4G_NAMESTRING      "iPod touch 4G"
#define IPOD_UNKNOWN_NAMESTRING "Unknown iPod"

#define IPAD_3G_NAMESTRING      "iPad 3G"
#define IPAD_4G_NAMESTRING      "iPad 4G"
#define IPAD_UNKNOWN_NAMESTRING "Unknown iPad"

#define APPLETV4_4_NAMESTRING       "Apple TV4"
#define APPLETV4_4F_NAMESTRING      "Apple TV4 FHD"
#define APPLETV4_4K_NAMESTRING      "Apple TV4 4k"
#define APPLETV4_UNKNOWN_NAMESTRING "Unknown Apple TV"

#define IOS_FAMILY_UNKNOWN_DEVICE "Unknown iOS device"

enum Platform
{
  PLATFORM_UNKNOWN,

  PLATFORM_IPHONE_5G,
  PLATFORM_IPHONE_UNKNOWN,

  PLATFORM_IPOD_4G,
  PLATFORM_IPOD_UNKNOWN,

  PLATFORM_IPAD_3G,
  PLATFORM_IPAD_4G,
  PLATFORM_IPAD_UNKNOWN,

  PLATFORM_APPLETV4_FHD, // full hd
  PLATFORM_APPLETV4_4K,  // 4k
  PLATFORM_APPLETV4_UNKNOWN,

  PLATFORM_IFPGA
};

enum Family
{
  FAMILY_UNKNOWN,
  FAMILY_IPHONE,
  FAMILY_IPOD,
  FAMILY_IPAD,
  FAMILY_APPLETV,
  FAMILY_IFPGA,
};

int getSysInfoByName(const char *typeSpecifier, char *buffer, size_t size)
{
  size_t nsize;
  sysctlbyname(typeSpecifier, NULL, &nsize, NULL, 0);

  if (nsize >= size)
    return -1;

  sysctlbyname(typeSpecifier, buffer, &nsize, NULL, 0);
  return nsize;
}

int getMachineInfo(char *buffer, size_t size) { return getSysInfoByName("hw.machine", buffer, size); }

int getHwModel(char *buffer, size_t size) { return getSysInfoByName("hw.model", buffer, size); }

uint32_t getSysInfo(uint32_t typeSpecifier)
{
  size_t size = sizeof(int);
  int results;
  int mib[2] = {CTL_HW, typeSpecifier};
  sysctl(mib, 2, &results, &size, NULL, 0);
  return results;
}

uint32_t getCpuFrequency() { return getSysInfo(HW_CPU_FREQ); }

uint32_t getBusFrequency() { return getSysInfo(HW_BUS_FREQ); }

uint32_t getCpuCount() { return getSysInfo(HW_NCPU); }

uint32_t getTotalMemory() { return getSysInfo(HW_PHYSMEM); }

uint32_t getUserMemory() { return getSysInfo(HW_USERMEM); }

uint32_t getMaxSocketBufferSize() { return getSysInfo(KIPC_MAXSOCKBUF); }

/*uint32_t getTotalDiskSpace()
{
    NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
    return [[fattributes objectForKey:NSFileSystemSize] integerValue];
}

uint32_t getFreeDiskSpace()
{
    NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
    return [[fattributes objectForKey:NSFileSystemFreeSize] integerValue];
}*/

uint32_t getPlatformId()
{
  char platform[1024];
  getMachineInfo(platform, 1024);

  // The ever mysterious iFPGA
  if (strncmp(platform, "iFPGA", 5) == 0)
    return PLATFORM_IFPGA;

  // iPhone
  if (strncmp(platform, "iPhone5", 7) == 0)
    return PLATFORM_IPHONE_5G;

  // iPod
  if (strncmp(platform, "iPod4", 5) == 0)
    return PLATFORM_IPOD_4G;

  // iPad
  if (strncmp(platform, "iPad3", 5) == 0)
    return PLATFORM_IPAD_3G;
  if (strncmp(platform, "iPad4", 5) == 0)
    return PLATFORM_IPAD_4G;

  // Apple TV
  if (strncmp(platform, "AppleTV5", 8) == 0)
    return PLATFORM_APPLETV4_FHD;
  if (strncmp(platform, "AppleTV6", 8) == 0)
    return PLATFORM_APPLETV4_4K;
  if (strncmp(platform, "AppleTV", 7) == 0)
    return PLATFORM_APPLETV4_UNKNOWN;

  return PLATFORM_UNKNOWN;
}

const char *getPlatformName()
{
  switch (getPlatformId())
  {
    case PLATFORM_IPHONE_5G: return IPHONE_5_NAMESTRING;
    case PLATFORM_IPHONE_UNKNOWN: return IPHONE_UNKNOWN_NAMESTRING;

    case PLATFORM_IPOD_4G: return IPOD_4G_NAMESTRING;
    case PLATFORM_IPOD_UNKNOWN: return IPOD_UNKNOWN_NAMESTRING;

    case PLATFORM_IPAD_3G: return IPAD_3G_NAMESTRING;
    case PLATFORM_IPAD_4G: return IPAD_4G_NAMESTRING;
    case PLATFORM_IPAD_UNKNOWN: return IPAD_UNKNOWN_NAMESTRING;

    case PLATFORM_APPLETV4_FHD: return APPLETV4_4F_NAMESTRING;
    case PLATFORM_APPLETV4_4K: return APPLETV4_4K_NAMESTRING;
    case PLATFORM_APPLETV4_UNKNOWN: return APPLETV4_UNKNOWN_NAMESTRING;

    case PLATFORM_IFPGA: return IFPGA_NAMESTRING;

    default: return IOS_FAMILY_UNKNOWN_DEVICE;
  }
}

/*bool hasRetinaDisplay()
{
    return ([UIScreen mainScreen].scale == 2.0f);
}*/

uint32_t getFamily()
{
  char platform[1024];
  getMachineInfo(platform, 1024);
  if (strncmp(platform, "iPhone", 6) == 0)
    return FAMILY_IPHONE;
  if (strncmp(platform, "iPod", 4) == 0)
    return FAMILY_IPOD;
  if (strncmp(platform, "iPad", 4) == 0)
    return FAMILY_IPAD;
  if (strncmp(platform, "AppleTV", 7) == 0)
    return FAMILY_APPLETV;

  return FAMILY_UNKNOWN;
}

} // end namespace hardware

} // namespace apple
