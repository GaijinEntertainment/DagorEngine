//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace ecs
{
class Object;
}
struct AdaptationSettings;

// Layer an ECS config object's adaptation__* members on top of `settings`.
void overrideAdaptationSetting(AdaptationSettings &settings, const ecs::Object &config);
