// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>

#include "driver.h"
#include "shader_module.h"

namespace drv3d_vulkan
{

struct ShaderModule
{
  ShaderModuleBlob blob;
  VulkanShaderModuleHandle module;
  spirv::HashValue hash;
  uint32_t size;
  uint32_t id;

  ShaderModule() = default;
  ~ShaderModule() = default;

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  ShaderModule(ShaderModule &&) = default;
  ShaderModule &operator=(ShaderModule &&) = default;

  ShaderModule(uint32_t id, const ShaderModuleBlob &blob);
};

class ShaderModuleStorage
{
  dag::Vector<eastl::unique_ptr<ShaderModule>> data;

public:
  ShaderModuleStorage() = default;
  ~ShaderModuleStorage() {}

  void add(ShaderModuleBlob *sci, uint32_t id);
  void remove(uint32_t id);
  ShaderModule *find(uint32_t id);

  void onDeviceReset();
  void afterDeviceReset();

  eastl::unique_ptr<ShaderModule> *getData() { return data.data(); }
};

} // namespace drv3d_vulkan
