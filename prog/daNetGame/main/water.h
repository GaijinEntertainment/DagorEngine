// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>

class FFTWater;

// water is singleton, it can be only one
ECS_DECLARE_BOXED_TYPE(FFTWater);

bool is_underwater();
bool is_water_hidden();
float get_waterlevel_for_camera_pos();
