// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daInput/input_api.h>
#include <daInput/config_api.h>
#include <ioSys/dag_dataBlock.h>

dainput::action_handle_t dainput::get_action_handle(const char *, uint16_t) { return BAD_ACTION_HANDLE; }
bool dainput::is_action_active(dainput::action_handle_t) { return false; }
bool dainput::reset_digital_action_sticky_toggle(dainput::action_handle_t) { return false; }
const dainput::DigitalAction &dainput::get_digital_action_state(dainput::action_handle_t)
{
  static DigitalAction stub = {false, false};
  return stub;
}
const dainput::AnalogAxisAction &dainput::get_analog_axis_action_state(dainput::action_handle_t)
{
  static AnalogAxisAction stub = {0, false};
  return stub;
}
const dainput::AnalogStickAction &dainput::get_analog_stick_action_state(dainput::action_handle_t)
{
  static AnalogStickAction stub = {0, 0, false};
  return stub;
}

dainput::action_set_handle_t dainput::get_action_set_handle(const char *) { return BAD_ACTION_SET_HANDLE; }
void dainput::set_action_enabled(dainput::action_handle_t, bool) {}
void dainput::set_action_mask_immediate(dainput::action_handle_t, bool) {}
bool dainput::is_action_mask_immediate(dainput::action_handle_t) { return false; }
void dainput::reset_action_set_stack() {}
void dainput::activate_action_set(action_set_handle_t, bool) {}

void dainput::set_default_preset_prefix(const char *) {}
const char *dainput::get_default_preset_prefix() { return ""; }
int dainput::get_default_preset_column_count() { return 0; }
bool dainput::load_user_config(const DataBlock &) { return false; }
bool dainput::save_user_config(DataBlock &, bool) { return false; }
const char *dainput::get_user_config_base_preset() { return ""; }
bool dainput::is_user_config_customized() { return false; }
void dainput::reset_user_config_to_preset(const char *, bool) {}
void dainput::reset_user_config_to_currest_preset() {}
DataBlock &dainput::get_user_props()
{
  static DataBlock b;
  return b;
}
bool dainput::is_user_props_customized() { return false; }
void dainput::term_user_config() {}

int dainput::get_actions_count() { return 0; }
int dainput::get_action_sets_count() { return 0; }
uint16_t dainput::get_action_type(dainput::action_handle_t) { return 0xFFFF; }
dainput::action_set_handle_t dainput::set_breaking_action_set(dainput::action_set_handle_t) { return BAD_ACTION_SET_HANDLE; }
dainput::action_set_handle_t dainput::setup_action_set(const char *, dag::ConstSpan<dainput::action_handle_t>)
{
  return BAD_ACTION_SET_HANDLE;
}

unsigned dainput::get_last_used_device_mask(unsigned) { return 0; }
int dainput::get_action_set_stack_depth() { return 0; }
dainput::action_set_handle_t dainput::get_action_set_stack_item(int) { return BAD_ACTION_SET_HANDLE; }
void dainput::set_control_thread_id(int64_t) {}
