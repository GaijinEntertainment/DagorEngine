// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entitySystem.h>

namespace dagdp
{

// Dummy type. Maybe will be useful in the future.
class CSMShadowsManager
{};

dafg::NodeHandle create_csm_shadows_provider(const dafg::NameSpace &ns, int max_cascades);

} // namespace dagdp

ECS_DECLARE_RELOCATABLE_TYPE(dagdp::CSMShadowsManager);
