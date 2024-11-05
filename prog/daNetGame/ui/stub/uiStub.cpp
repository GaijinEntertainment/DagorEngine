// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <squirrel.h>
#include <math/dag_TMatrix.h>
#include <util/dag_oaHashNameMap.h>

namespace dainput
{
typedef uint16_t action_set_handle_t;
}

namespace user_ui
{
void init() {}
void term() {}
void reload_user_ui_script(bool /*full_reinit*/) {}

HSQUIRRELVM get_vm() { return nullptr; }
bool is_fully_covering() { return false; }
void set_fully_covering(bool) {}
} // namespace user_ui

namespace uishared
{
bool is_ui_available_in_build() { return false; }

void init_early() {}
void init() {}
void term() {}
void update() {}

void on_settings_changed(const FastNameMap &, bool){};
void before_device_reset() {}
void after_device_reset() {}

void activate_ui_elem_action_set(dainput::action_set_handle_t ash, bool on);

TMatrix view_tm, view_itm;
} // namespace uishared

namespace uiinput
{
void update_joystick_input() {}
} // namespace uiinput


namespace uirender
{
void init() {}
} // namespace uirender

void pull_ui_das() {}
void register_game_rendobj_factories() {}
void dng_load_localization() {}
void unload_localization() {}


namespace darg
{
void register_std_rendobj_factories() {}
void register_blur_rendobj_factories(bool /*wt_compatibility_mode*/) {}
} // namespace darg

extern const size_t sq_autobind_pull_connectivity = 0;
