// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader_info.h"
#include "shader.h"
#include "device_context.h"
#include "backend/cmd/resources.h"

using namespace drv3d_vulkan;

void UniqueShaderModule::addToContext(DeviceContext &ctx, uint32_t idx, const CreationInfo &info)
{
  id = idx;
  ctx.dispatchCmd<CmdAddShaderModule>({eastl::make_unique<ShaderModuleBlob>(info).release(), id});
}

void UniqueShaderModule::removeFromContext(DeviceContext &ctx, uint32_t idx)
{
  inRemoval = true;
  ctx.dispatchCmd<CmdRemoveShaderModule>({idx});
}
