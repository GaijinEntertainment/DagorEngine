// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "driver.h"

namespace drv3d_vulkan
{

struct ShaderModule
{
  VulkanShaderModuleHandle module;
  spirv::HashValue hash;
  uint32_t size;
  uint32_t id;

  ShaderModule() = default;
  ~ShaderModule() = default;

  ShaderModule(const ShaderModule &) = default;
  ShaderModule &operator=(const ShaderModule &) = default;

  ShaderModule(ShaderModule &&) = default;
  ShaderModule &operator=(ShaderModule &&) = default;

  ShaderModule(VulkanShaderModuleHandle m, uint32_t i, const spirv::HashValue &hv, uint32_t sz) : module(m), id(i), hash(hv), size(sz)
  {}
};

class ShaderModuleStorage
{
  eastl::vector<eastl::unique_ptr<ShaderModule>> data;

public:
  ShaderModuleStorage() = default;
  ~ShaderModuleStorage() {}

  void add(const ShaderModuleBlob *sci, uint32_t id);
  void remove(uint32_t id);
  ShaderModule *find(uint32_t id);

  eastl::unique_ptr<ShaderModule> *getData() { return data.data(); }
};

} // namespace drv3d_vulkan
