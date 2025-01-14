//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>

class Point3;

namespace sound
{

namespace sqapi
{
void play_sound(const char *name, const Sqrat::Object &params, float volume, const Point3 *pos);
int get_num_event_instances(const char *name);
bool is_preset_loaded(const char *preset_name);

void release_vm(HSQUIRRELVM vm);
void on_record_devices_list_changed();
void on_output_devices_list_changed();
} // namespace sqapi

} // namespace sound
