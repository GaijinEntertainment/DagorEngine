// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <squirrel.h>
#include <math/dag_TMatrix.h>
#include <util/dag_oaHashNameMap.h>
#include <daInput/input_api.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_joystick.h>
#include "ui/uiRender.h"
#include "ui/userUi.h"
#include "ui/uiShared.h"
#include "ui/xrayUiOrder.h"
#include "ui/loadingUi.h"
#include "net/netStat.h"


eastl::unique_ptr<darg::IGuiScene> user_ui::gui_scene;
eastl::unique_ptr<darg::JoystickHandler> uishared::joystick_handler;
TMatrix uishared::view_tm, uishared::view_itm;
bool uirender::multithreaded = false;


void user_ui::init() {}
void user_ui::term() {}
void user_ui::reload_user_ui_script(bool /*full_reinit*/) {}
void user_ui::before_render() {}
HSQUIRRELVM user_ui::get_vm() { return nullptr; }
bool user_ui::is_fully_covering() { return false; }
void user_ui::set_fully_covering(bool) {}

bool uishared::is_ui_available_in_build() { return false; }
void uishared::init_early() { joystick_handler.reset(new darg::JoystickHandler()); }
void uishared::init() {}
void uishared::term() {}
void uishared::update() {}
void uishared::on_settings_changed(const FastNameMap &, bool) {}
void uishared::before_device_reset() {}
void uishared::after_device_reset() {}
void uishared::window_resized() {}
void uishared::invalidate_world_3d_view() {}
void uishared::save_world_3d_view(const Driver3dPerspective &, const TMatrix &, const TMatrix &) {}
void uishared::activate_ui_elem_action_set(dainput::action_set_handle_t ash, bool on);

void uirender::init() {}
void uirender::update_all_gui_scenes_mainthread(float) {}
void uirender::start_ui_before_render_job() {}
void uirender::skip_ui_render_job() {}
void uirender::start_ui_render_job(bool) {}
void uirender::wait_ui_before_render_job_done() {}
void uirender::wait_ui_render_job_done() {}
void uirender::before_render(float, const TMatrix &, const TMatrix &, const TMatrix4 &) {}
uirender::all_darg_scenes_t uirender::get_all_scenes() { return {}; }

bool ui::xray_ui_order::should_render_xray_before_ui() { return false; }

void netstat::toggle_render() {}
void netstat::render() {}

bool loading_ui::is_fully_covering() { return false; }

void pull_ui_das() {}
void register_game_rendobj_factories() {}
void dng_load_localization() {}
void unload_localization() {}


extern const size_t sq_autobind_pull_connectivity = 0;
