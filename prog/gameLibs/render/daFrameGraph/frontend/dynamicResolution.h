// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <common/dynamicResolution.h>

namespace dafg
{

struct InternalRegistry;

DynamicResolutionUpdates collect_dynamic_resolution_updates(const InternalRegistry &registry);

void track_applied_dynamic_resolution_updates(InternalRegistry &registry, const DynamicResolutionUpdates &updates);

DynamicResolutions collect_applied_dynamic_resolutions(const InternalRegistry &registry);

} // namespace dafg
