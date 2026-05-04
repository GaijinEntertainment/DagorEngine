// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler.h"

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <drv/3d/dag_info.h>
#include <EASTL/string_view.h>

namespace auto_graphics::auto_quality_preset::detail
{

static VramGb get_dedicated_gpu_memory_rounded_gb()
{
  const auto dedicatedGpuMemMb = d3d::get_dedicated_gpu_memory_size_kb() >> 10;
  return (dedicatedGpuMemMb + 1023) >> 10; // round up to next GB
}

static String get_filtered_device_name(eastl::string_view raw_device_name)
{
  // skip:
  // - series of spaces
  // - info in parantheses (copyright, driver name, revision, chip name and other useless info)
  // and replace space with underscore
  String deviceName;
  int parenthesesCounter = 0; // there was no detected device names with nested parentheses, but use int for safety
  for (auto c : raw_device_name)
  {
    if (c == ' ')
    {
      if (!deviceName.empty() && deviceName.back() != '_')
        deviceName.push_back('_');
    }
    else if (c == '(')
      ++parenthesesCounter;
    else if (c == ')')
      --parenthesesCounter;
    else if (parenthesesCounter == 0)
    {
      if (c == '-')
        c = '_';
      deviceName.push_back(c);
    }
  }

  // complete c-string, delete trailing underscore
  // AMD Radeon RX 570 (POLARIS10) -> AMD_Radeon_RX_570
  if (!deviceName.empty() && deviceName.back() == '_')
    deviceName.back() = '\0';
  else
    deviceName.push_back('\0');
  deviceName.updateSz();

  // NVIDIA_RTX_2080 -> nvidia_rtx_2080
  deviceName.toLower();

  if (auto underscoreBeforeGB = deviceName.find("_gb"); underscoreBeforeGB != nullptr)
    deviceName.erase(underscoreBeforeGB); // nvidia_rtx_3050_8_gb -> nvidia_rtx_3050_8gb

  // filter laptop GPUs:
  // mobile_something -> laptop_gpu_something
  // mobile_gpu_something -> laptop_gpu_something
  // laptop_something -> laptop_gpu_somethings
  if (deviceName.find("mobile") != nullptr)
    deviceName.replace("mobile", "laptop");
  const char LAPTOP_SUBSTR[] = "laptop";
  const size_t LAPTOP_SUBSTR_OFFSET = sizeof(LAPTOP_SUBSTR) - 1;
  if (auto laptopSubsstr = deviceName.find(LAPTOP_SUBSTR);
      laptopSubsstr && !deviceName.find("gpu", laptopSubsstr + LAPTOP_SUBSTR_OFFSET))
    deviceName.replace(LAPTOP_SUBSTR, "laptop_gpu");

  // replace ^radeon to ^amd_radeon
  if (deviceName.find("radeon") == deviceName.begin())
    deviceName.replace("radeon", "amd_radeon");

  // remove amd specific naming
  deviceName.replace("_series", "");

  // some intel and amd GPUs have "_graphics" suffix, remove it
  deviceName.replace("_graphics", "");

  return deviceName;
}

eastl::optional<GpuInfo> get_gpu_info()
{
  DataBlock gpuDatabase;
  if (!gpuDatabase.load("config/gpu_database.blk"))
  {
    logerr("can't load gpu_database.blk, auto graphical settings will not be applied");
    return eastl::nullopt;
  }

  auto deviceName = get_filtered_device_name(d3d::get_device_name());

  const auto vramGb = get_dedicated_gpu_memory_rounded_gb();
  GpuScore gpuScore = gpuDatabase.getInt(deviceName.c_str(), 0);
  if (gpuScore == 0)
  {
    bool foundAlias = false;
    // try to find by memory size
    if (gpuDatabase.blockExists(deviceName.c_str()))
    {
      String gb;
      gb.aprintf(16, "_%dgb", vramGb);
      deviceName += gb;

      gpuScore = gpuDatabase.getInt(deviceName.c_str(), 0);
      foundAlias = gpuScore != 0;
    }

    if (!foundAlias)
    {
      logwarn("can't find device '%s' in gpu_database.blk, auto graphical settings will not be applied", deviceName.c_str());
      return eastl::nullopt;
    }
  }

  return eastl::make_pair(gpuScore, vramGb);
}

} // namespace auto_graphics::auto_quality_preset::detail
