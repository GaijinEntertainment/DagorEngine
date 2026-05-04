// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/history.h>

#include <backend/intermediateRepresentation.h>
#include <frontend/internalRegistry.h>

#include <render/daFrameGraph/detail/access.h>
#include <render/daFrameGraph/detail/resourceType.h>

#include <drv/3d/dag_driver.h>
#include <EASTL/optional.h>


namespace dafg
{

enum class ValidateUsageResult
{
  Invalid,
  IncompatibleActivation,
  EmptyActivation,
  OK
};

enum class DesiredActivationBehaviour
{
  Discard,
  Clear,
};

ValidateUsageResult validate_usage(ResourceUsage usage, ResourceType res_type, DesiredActivationBehaviour history, bool is_int);

ResourceBarrier barrier_for_transition(intermediate::ResourceUsage usage_before, intermediate::ResourceUsage usage_after);

eastl::optional<ResourceActivationAction> get_activation_from_usage(DesiredActivationBehaviour behavior,
  intermediate::ResourceUsage usage, ResourceType res_type, bool is_int);
__forceinline eastl::optional<ResourceActivationAction> get_activation_from_usage(DesiredActivationBehaviour history,
  ResourceUsage usage, ResourceType res_type, bool is_int)
{
  return get_activation_from_usage(history, intermediate::ResourceUsage{usage.type, usage.access, usage.stage}, res_type, is_int);
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type);
ResourceActivationAction get_history_activation(DesiredActivationBehaviour behavior, ResourceActivationAction res_activation,
  bool is_int = false);

} // namespace dafg
