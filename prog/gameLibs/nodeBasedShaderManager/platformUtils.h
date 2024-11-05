// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

static const eastl::array<const char *, NodeBasedShaderManager::PLATFORM::COUNT> PLATFORM_LABELS = {{
  "dx11",
  "xb1",
  "ps4",
  "spirv",
  "dx12",
  "dx12x",
  "dx12xs",
  "ps5",
  "metal",
  "spirvb",
  "metalb",
}};

#if !NBSM_COMPILE_ONLY
static NodeBasedShaderManager::PLATFORM get_nbsm_platform()
{
  return d3d::get_driver_code()
    .map(d3d::xboxOne, NodeBasedShaderManager::PLATFORM::DX12_XBOX_ONE)
    .map(d3d::scarlett, NodeBasedShaderManager::PLATFORM::DX12_XBOX_SCARLETT)
    .map(d3d::ps4, NodeBasedShaderManager::PLATFORM::PS4)
    .map(d3d::ps5, NodeBasedShaderManager::PLATFORM::PS5)
    .map(d3d::dx11, NodeBasedShaderManager::PLATFORM::DX11)
    .map(d3d::dx12, NodeBasedShaderManager::PLATFORM::DX12)
    .map(d3d::vulkan, d3d::get_driver_desc().caps.hasBindless ? NodeBasedShaderManager::PLATFORM::VULKAN_BINDLESS
                                                              : NodeBasedShaderManager::PLATFORM::VULKAN)
    .map(d3d::metal,
      d3d::get_driver_desc().caps.hasBindless ? NodeBasedShaderManager::PLATFORM::MTL_BINDLESS : NodeBasedShaderManager::PLATFORM::MTL)
    .map(d3d::stub, ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBool("enableBindless", false)
                      ? NodeBasedShaderManager::PLATFORM::VULKAN_BINDLESS
                      : NodeBasedShaderManager::PLATFORM::VULKAN)
    .map(d3d::any, NodeBasedShaderManager::PLATFORM::COUNT);
}
#endif