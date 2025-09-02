// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "input/inputControls.h"
#include "input/globInput.h"
#include "input/uiInput.h"
#include "input/touchInput.h"
#include <ecs/core/entitySystem.h>

ECS_DEF_PULL_VAR(input);
size_t framework_input_pulls = 0;
TouchInput touch_input;

void init_glob_input() {}
void destroy_glob_input() {}
void pull_input_das() {}
bool have_glob_input() { return false; }
bool glob_input_process_top_level_key(bool, int, unsigned) { return false; }

void controls::bind_script_api(SqModules *) {}
void controls::init_drivers() {}
void controls::init_control(const char *) {}
void controls::process_input(double) {}
void controls::destroy() {}
void controls::global_init() {}
void controls::global_destroy() {}

void uiinput::mask_dainput_buttons(dag::ConstSpan<darg::HotkeyButton>, bool, int) {}
void uiinput::mask_dainput_pointers(int, int) {}
void uiinput::update_joystick_input() {}

void TouchInput::setStickValue(dainput::action_handle_t, const Point2 &) {}
void TouchInput::releaseButton(dainput::action_handle_t) {}
void TouchInput::setAxisValue(dainput::action_handle_t, float const &) {}
void TouchInput::pressButton(dainput::action_handle_t) {}
bool TouchInput::isButtonPressed(dainput::action_handle_t) { return false; }

void register_hid_event_handler(ecs::IGenHidEventHandler *, unsigned) {}
void unregister_hid_event_handler(ecs::IGenHidEventHandler *) {}
