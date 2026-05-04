// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "module.h"
#include "timelines.h"
#include "frame_info.h"
#include "globals.h"
#include "backend.h"

using namespace drv3d_vulkan;

ShaderModule::ShaderModule(uint32_t id, const ShaderModuleBlob &blob) : blob(blob), size(blob.size), id(id)
{
  module = Globals::pipelines.makeVkModule(&blob, hash);
}

void ShaderModuleStorage::add(ShaderModuleBlob *sci, uint32_t id)
{
  data.push_back(eastl::make_unique<ShaderModule>(id, *sci));
  delete sci;
}

void ShaderModuleStorage::remove(uint32_t id)
{
  // add-remove order is important, as we use tight packing for module ids
  auto moduleRef = eastl::find_if(begin(data), end(data), [id](const eastl::unique_ptr<ShaderModule> &sm) { return sm->id == id; });
  Backend::gpuJob->deletedShaderModules.push_back(eastl::move(*moduleRef));
  *moduleRef = eastl::move(data.back());
  data.pop_back();
}

ShaderModule *ShaderModuleStorage::find(uint32_t id)
{
  auto ref = eastl::find_if(begin(data), end(data), [id](const eastl::unique_ptr<ShaderModule> &module) { return id == module->id; });
  return ref->get();
}

void ShaderModuleStorage::onDeviceReset()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  for (auto &module : data)
    VULKAN_LOG_CALL(vkDev.vkDestroyShaderModule(vkDev.get(), module->module, VKALLOC(shader_module)));
}

void ShaderModuleStorage::afterDeviceReset()
{
  spirv::HashValue hv;
  for (auto &module : data)
    module->module = Globals::pipelines.makeVkModule(&module->blob, hv);
}
