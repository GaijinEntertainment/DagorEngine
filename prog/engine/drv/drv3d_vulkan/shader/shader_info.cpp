// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader_info.h"
#include "shader.h"
#include "device_context.h"

using namespace drv3d_vulkan;

void UniqueShaderModule::addToContext(DeviceContext &ctx, uint32_t idx, const CreationInfo &info)
{
  id = idx;
  ctx.addShaderModule(id, eastl::make_unique<ShaderModuleBlob>(info));
}

void UniqueShaderModule::removeFromContext(DeviceContext &ctx, uint32_t idx)
{
  inRemoval = true;
  ctx.removeShaderModule(idx);
}