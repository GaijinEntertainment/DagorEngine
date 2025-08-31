//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>

class DaSkies;


DaSkies *get_daskies_impl();
int get_daskies_seed();
void set_daskies_seed(int);

void validate_volumetric_clouds_settings();
bool is_panorama_forced();


ECS_BROADCAST_EVENT_TYPE(EventSkiesLoaded);
ECS_BROADCAST_EVENT_TYPE(CmdSkiesInvalidate);
