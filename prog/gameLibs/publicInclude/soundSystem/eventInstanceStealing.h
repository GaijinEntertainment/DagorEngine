//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/handle.h>

namespace sndsys
{
// seems like stolen or virtual instances in 3d are not being updated by fmod.
// stolen or virtual instance will/may not wake when listener arrives near it.
// stolen stopped instance will/may never be started.
// currently there is no solution how to make fmod handle this properly.
//
// made to manage non-oneshot instances of different types and big hearing range.
// fmod event stealing supposed to be set to 'virtualize' and max instances should be greater than target_instances.

static constexpr float stealing_default_fade_in_out_speed = 1.f;
static constexpr int stealing_default_target_instances = 4;

int create_event_instance_stealing_group(const char *group_name, int target_instances = stealing_default_target_instances,
  float fade_in_out_speed = stealing_default_fade_in_out_speed);

void update_event_instance_stealing(EventHandle handle, int group_idx, float dt);
} // namespace sndsys
