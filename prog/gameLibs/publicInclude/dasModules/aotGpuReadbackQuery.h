#pragma once

#include <dasModules/dasModulesCommon.h>
#include <gpuReadbackQuery/gpuReadbackResult.h>

DAS_BIND_ENUM_CAST(GpuReadbackResultState);

DAS_BASE_BIND_ENUM(GpuReadbackResultState, GpuReadbackResultState, IN_PROGRESS, SUCCEEDED, FAILED, ID_NOT_FOUND,
  SYSTEM_NOT_INITIALIZED);
