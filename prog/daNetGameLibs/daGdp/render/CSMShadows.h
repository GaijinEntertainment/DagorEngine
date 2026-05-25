// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entitySystem.h>

namespace dagdp
{

// Dummy type. Maybe will be useful in the future.
class CSMShadowsManager
{};

struct ViewPerFrameData;
void populate_csm_viewports(ViewPerFrameData &view_data, ecs::EntityManager &manager, int max_viewports);

dafg::NodeHandle create_csm_shadows_provider(const dafg::NameSpace &ns, ecs::EntityManager &manager, int max_viewports);

} // namespace dagdp

ECS_DECLARE_RELOCATABLE_TYPE(dagdp::CSMShadowsManager);
