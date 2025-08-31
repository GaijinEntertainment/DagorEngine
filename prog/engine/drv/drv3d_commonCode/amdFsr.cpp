// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_texture.h>
#include <ioSys/dag_dataBlock.h>

namespace amd
{

FSR *createD3D12Win();
FSR *createD3D12Xbox();
FSR *createPS5();
FSR *createVulkan();

FSR::UpscalingMode FSR::getUpscalingMode(const DataBlock &video)
{
  auto mode = video.getStr("amdfsr", "off");
  if (stricmp(mode, "NativeAA") == 0)
    return UpscalingMode::NativeAA;
  if (stricmp(mode, "Quality") == 0)
    return UpscalingMode::Quality;
  if (stricmp(mode, "Balanced") == 0)
    return UpscalingMode::Balanced;
  if (stricmp(mode, "Performance") == 0)
    return UpscalingMode::Performance;
  if (stricmp(mode, "UltraPerformance") == 0)
    return UpscalingMode::UltraPerformance;
  return UpscalingMode::Off;
}

using FormatTypeID = uint32_t;

static bool is_uav_load_supported(std::initializer_list<FormatTypeID> formats)
{
  for (auto format : formats)
  {
    if (!(d3d::get_texformat_usage(format) & d3d::USAGE_UNORDERED_LOAD))
      return false;
  }
  return true;
}

bool FSR::isSupported()
{
#if _TARGET_PC_WIN
  if (d3d::get_driver_desc().shaderModel >= 6.2_sm &&
      (d3d::get_driver_code().is(d3d::dx12) || d3d::get_driver_code().is(d3d::vulkan)) &&
      is_uav_load_supported(
        {TEXFMT_R8, TEXFMT_A16B16G16R16F, TEXFMT_R11G11B10F, TEXFMT_R16F, TEXFMT_G16R16F, TEXFMT_R32UI, TEXFMT_R8G8B8A8}))
    return true;
#elif _TARGET_XBOX
  return true;
#endif

  return false;
}

int FSR::getMaximumNumberOfGeneratedFrames()
{
#if _TARGET_PC_WIN
  if (d3d::get_driver_desc().shaderModel >= 6.2_sm && d3d::get_driver_code().is(d3d::dx12 || d3d::vulkan) &&
      is_uav_load_supported(
        {TEXFMT_R8, TEXFMT_A16B16G16R16F, TEXFMT_R11G11B10F, TEXFMT_R16F, TEXFMT_G16R16F, TEXFMT_R32UI, TEXFMT_R8G8B8A8}))
    return 1;
#elif _TARGET_XBOX
  return 0;
#endif

  return 0;
}

void FSR::applyUpscaling(const UpscalingArgs &args) { d3d::driver_command(Drv3dCommand::EXECUTE_FSR, this, (void *)&args); }

void FSR::scheduleGeneratedFrames(const FrameGenArgs &args) { d3d::driver_command(Drv3dCommand::EXECUTE_FSR_FG, this, (void *)&args); }

FSR *createFSR()
{
#if _TARGET_PC_WIN
#if AMDFSR_HAS_DX12
  if (d3d::get_driver_code().is(d3d::dx12))
    return createD3D12Win();
#endif
#if AMDFSR_HAS_VULKAN
  if (d3d::get_driver_code().is(d3d::vulkan))
    return createVulkan();
#endif
#elif _TARGET_XBOX
  return createD3D12Xbox();
#elif _TARGET_C2

#endif

  return nullptr;
}

} // namespace amd