// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <daInput/input_api.h>
#include <math/dag_TMatrix.h>
#include <util/dag_oaHashNameMap.h>

struct Driver3dPerspective;

namespace darg
{
class JoystickHandler;
}

namespace uishared
{
void init_early();
bool is_ui_available_in_build();

void init();
void term();

void update();

void before_device_reset();
void after_device_reset();

void on_settings_changed(const FastNameMap &changed_fields, bool apply_after_device_reset);
void window_resized();

void activate_ui_elem_action_set(dainput::action_set_handle_t ash, bool on);

void invalidate_world_3d_view();
void save_world_3d_view(const Driver3dPerspective &persp, const TMatrix &view_tm, const TMatrix &view_itm);
bool get_world_3d_persp(Driver3dPerspective &persp);
bool has_valid_world_3d_view();
extern TMatrix view_tm, view_itm;

extern eastl::unique_ptr<darg::JoystickHandler> joystick_handler; // inited once at start
} // namespace uishared

void pull_ui_das();
