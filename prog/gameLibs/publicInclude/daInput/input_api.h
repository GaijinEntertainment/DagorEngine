//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <string.h>
#include <generic/dag_tabFwd.h>
#include <humanInput/dag_hiDecl.h>

class DataBlock;
class String;
class SqModules;
namespace darg
{
struct IGuiScene;
}

namespace dainput
{
//! device types list (5bit uint)
enum
{
  DEV_none = 0,
  DEV_kbd,
  DEV_pointing,
  DEV_gamepad,
  DEV_joy,
  DEV_vr,

  DEV_nullstub = 31
};

//! action type (composed of 2 highest bits as group and 14 lowest bits as type)
enum
{
  TYPEGRP__MASK = 0x3 << 14,

  TYPEGRP_DIGITAL = 0 << 14,
  TYPE_BUTTON,

  TYPEGRP_AXIS = 1 << 14,
  TYPE_TRIGGER,
  TYPE_STEERWHEEL,

  TYPEGRP_STICK = 2 << 14,
  TYPE_SYSMOUSE,
  TYPE_ABSMOUSE,
  TYPE_STICK,
  TYPE_STICK_DELTA,
};

enum
{
  AXIS_NULL_ID = 0x7FFu,
  BTN_NULL_ID = AXIS_NULL_ID,
};

//! basic digital action that generally produces event when action occurs (but bState also may be watched to simplest types)
struct DigitalAction
{
  bool bState;
  bool bActive, bActivePrev;
  bool bEnabled;
};

// analogue action for single axis (triggers are 0..+1 and steerweels are -1..+1)
struct AnalogAxisAction
{
  float x;
  bool bActive, bActivePrev;
  bool bEnabled;
};

// analogue action for stick (2-axis); x and y are in -1..+1 range
struct AnalogStickAction
{
  float x, y;
  bool bActive, bActivePrev;
  bool bEnabled;
};


//! button ID (composed of device ID and button index)
struct SingleButtonId
{
  uint16_t devId : 5, btnId : 11;
};

//! controls binding for digital action
struct DigitalActionBinding
{
  enum
  {
    BTN_pressed = 0,    //< event on button press (front)
    BTN_pressed_long,   //< event on button long press (front)
    BTN_pressed2,       //< event on button double click (front)
    BTN_pressed3,       //< event on button triple click (front)
    BTN_released,       //< event on button release (back)
    BTN_released_short, //< event on button release after short press (back)
    BTN_released_long,  //< event on button release after long press (back)
  };

  //! main button (dev/btn or dev/trigger, depending on btnCtrl)
  uint16_t devId : 5, ctrlId : 11;

  uint16_t eventType : 3, //< type of event (BTN_*)
    axisCtrlThres : 3,    //< activation threshold for trigger (*12%)
    btnCtrl : 1,          //< btn sign (=1 means ctrlId is button idx, =0 means ctrlId is axis(trigger) index)
    stickyToggle : 1,     //< each event toggles bState, just like CapsLock (in comparison with Shift)
    unordCombo : 1,       //< main button and modifiers are interchangable (detect press in any order)
    _resv : 1;

  uint16_t modCnt : 2;   //< number of modifier buttons
  SingleButtonId mod[3]; //< IDs of modifier buttons

  DigitalActionBinding() { memset(this, 0, sizeof(*this)); }
};

//! controls binding for analogue axis
struct AnalogAxisActionBinding
{
  //! main axis ID
  uint16_t devId : 5, axisId : 11;
  uint16_t invAxis : 1, //< axis direction is inversed
    axisRelativeVal : 1, instantIncDecBtn : 1, quantizeValOnIncDecBtn : 1, _resv : 10;

  uint16_t modCnt : 2;   //< number of modifier buttons
  SingleButtonId mod[3]; //< IDs of modifier buttons
  SingleButtonId minBtn, //< (optional) button to force -maxVal axis deflection (has sense for stearwheel)
    maxBtn;              //< (optional) button to force maxVal axis deflection
  SingleButtonId incBtn, //< (optional) button to increment axis deflection (has sense for relative thrust)
    decBtn;              //< (optional) button to decrement axis deflection (has sense for relative thrust)
  float deadZoneThres,   //< axis dead zone threshold (0..1)
    nonLin,              //< coef of non-linearity (0..)
    maxVal;              //< max value applied when maxBtn is active
  float relIncScale;     //< relative inc/dec scale (incBtn, decBtn or axis with axisRelativeVal)

  AnalogAxisActionBinding()
  {
    memset(this, 0, sizeof(*this));
    maxVal = relIncScale = 1;
  }
};

//! controls binding for analogue stick
struct AnalogStickActionBinding
{
  uint16_t devId : 5, axisXId : 11, axisYId : 11, //< stick device and axis X and Y ID
    axisXinv : 1,                                 //< axis X direction is inversed
    axisYinv : 1,                                 //< axis Y direction is inversed
    modCnt : 2,                                   //< number of modifier buttons
    _resv : 1;

  SingleButtonId mod[3];  //< IDs of modifier buttons
  SingleButtonId minXBtn, //< (optional) button to force -maxVal axis X deflection
    maxXBtn;              //< (optional) button to force +maxVal axis X deflection
  SingleButtonId minYBtn, //< (optional) button to force -maxVal axis Y deflection
    maxYBtn;              //< (optional) button to force +maxVal axis Y deflection
  float deadZoneThres,    //< stick center circular dead zone threshold (0..1)
    axisSnapAngK,         //< tangent of half-angle for axis snapping (0..1)
    nonLin,               //< coef of non-linearity (0..)
    maxVal,               //< max value applied when maxBtn is active
    sensScale;            //< stick deflection scale (for sensitivity setup)

  AnalogStickActionBinding()
  {
    memset(this, 0, sizeof(*this));
    maxVal = sensScale = 1;
  }
};

//! handle for single action
typedef uint16_t action_handle_t;
//! handle for action set
typedef uint16_t action_set_handle_t;
enum
{
  BAD_ACTION_HANDLE = 0xFFFF,
  BAD_ACTION_SET_HANDLE = 0xFFFF
};

typedef void (*action_triggered_t)(action_handle_t action, bool action_terminated, int dur_ms);

struct DigitalActionProgress
{
  enum
  {
    ST_in_progress,
    ST_finished,
    ST_cancelled
  };

  action_handle_t action;
  int16_t state;   // ST_in_progress, etc.
  int16_t evType;  // BTN_pressed, etc.
  int startT0msec; // time of action progress started, based on get_time_msec()
};
typedef void (*action_progress_changed_t)(dag::ConstSpan<DigitalActionProgress> actions);

typedef void (*actions_processed_t)(float dt_passed);
} // namespace dainput

namespace dainput
{
//! register squirrel API
void bind_sq_api(SqModules *moduleMgr);
//! register daRg scene as consumer of input
void register_darg_scene(int idx, darg::IGuiScene *scn);
//! unregister daRg scene
void unregister_darg_scene(int idx);

//! reset registered input devices
void reset_devices();
//! register keyboard device as dev1
void register_dev1_kbd(HumanInput::IGenKeyboard *kbd);
//! register mouse device as dev2
void register_dev2_pnt(HumanInput::IGenPointing *pnt);
//! register gamepad device as dev3
void register_dev3_gpad(HumanInput::IGenJoystick *gamepad);
//! register joystick device as dev4
void register_dev4_joy(HumanInput::IGenJoystick *joy);
//! register VR input device as dev5
void register_dev5_vr(HumanInput::VrInput *vr);

//! install event handler to receive triggered digital actions
void set_event_receiver(action_triggered_t on_action_triggered);

//! install event handler to receive triggered digital actions
void set_digital_event_progress_monitor(action_progress_changed_t on_progress_changed);

//! clear all actions and action sets
void reset_actions();
//! init actions and action sets (data-based, from BLK)
void init_actions(const DataBlock &blk);
int get_actions_config_version();

//! setup duration of long press (500 ms by default)
void set_long_press_time(int dur_ms);
//! returns duration of long press
int get_long_press_time();
//! setup max duration between clicks to detect double/triple click (250 ms by default)
void set_double_click_time(int dur_ms);
//! returns max duration between clicks
int get_double_click_time();

//! reset all control bindings for all actions
void reset_actions_binding();
//! load control bindings to new column (for all actions) from BLK
void append_actions_binding(const DataBlock &blk);
//! clears control bindings for specified column
void clear_actions_binding(int column);
//! load control bindings for specified column (for all actions) from BLK;
//! (optional) on_config_ver_differ() is called when versions differ and may alter blk before it is parsed
void load_actions_binding(const DataBlock &blk, int column,
  void (*on_config_ver_differ)(DataBlock &blk, int config_ver, int bindings_ver) = nullptr);
//! save control bindings for specified column (for all actions) to BLK
void save_actions_binding(DataBlock &blk, int column);
bool save_actions_binding_ex(DataBlock &blk, int column, const DataBlock *base_preset);
//! returns number of columns for control bindings
int get_actions_binding_columns();
//! get tag string for tag id
const char *get_tag_str(unsigned tag);

//! sets binding column as active/inactive
void set_actions_binding_column_active(unsigned column_idx, bool active);
//! returns whether binding column as active
bool get_actions_binding_column_active(unsigned column_idx);

//! find action by name and return handle to it (optionally type group of action may be checked)
action_handle_t get_action_handle(const char *action_name, uint16_t required_type_grp = 0xFFFF);
//! returns type of action
uint16_t get_action_type(action_handle_t action);
//! returns name of action
const char *get_action_name(action_handle_t action);
//! returns true when action is active (i.e., .bActive=true)
bool is_action_active(action_handle_t action);
bool is_action_enabled(action_handle_t action);
void set_action_enabled(action_handle_t action, bool is_enabled);
//! set a mask immediate flag for action
void set_action_mask_immediate(action_handle_t action, bool is_enabled);
//! returns a mask immediate flag for action (false if the action is invalid)
bool is_action_mask_immediate(action_handle_t action);
//! returns true when action has "internal" flag (e.g. such action should not be shown in controls setup UI)
bool is_action_internal(action_handle_t action);
//! returns group tag for action (or 0 when not set in config)
unsigned get_group_tag_for_action(action_handle_t action);
//! returns group tag string for action (or "" when not set in config)
inline const char *get_group_tag_str_for_action(action_handle_t action) { return get_tag_str(get_group_tag_for_action(action)); }
//! returns digital action state
const DigitalAction &get_digital_action_state(action_handle_t action);
//! returns analogue axis action state
const AnalogAxisAction &get_analog_axis_action_state(action_handle_t action);
//! returns analogue stick action state
const AnalogStickAction &get_analog_stick_action_state(action_handle_t action);

//! sets value for analogue axis action (stateful trigger only!)
bool set_analog_axis_action_state(action_handle_t action, float val);

//! find action set by name and return handle to it
action_set_handle_t get_action_set_handle(const char *action_set_name);
//! returns name of action set
const char *get_action_set_name(action_set_handle_t set);
//! returns list of actions in action set
dag::ConstSpan<action_handle_t> get_action_set_actions(action_set_handle_t set);

//! dynamically registers action set with specified name actions list
action_set_handle_t setup_action_set(const char *action_set_name, dag::ConstSpan<action_handle_t> actions);
//! clears action set (previously registered with setup_action_set)
void clear_action_set_actions(action_set_handle_t set);

//! clear stack of active action sets
void reset_action_set_stack();
//! activate or inactivate action set in active stack
void activate_action_set(action_set_handle_t set, bool activate = true);
//! returns current depth of activ action sets stack
int get_action_set_stack_depth();
//! returns action set from stack by index (top-down)
action_set_handle_t get_action_set_stack_item(int top_down_index);
//! returns current action set (topmost set in stack)
action_set_handle_t get_current_action_set();

//! sets specified action set as 'breaking' meaning that data processing stops after such set when it is active
//! returns previous 'breaking' action set handle
action_set_handle_t set_breaking_action_set(action_set_handle_t set);

//! returns control bindings for specified action and column as BLK
void get_action_binding(action_handle_t action, int column, DataBlock &out_binding);
//! set control bindings for specified action and column from BLK
void set_action_binding(action_handle_t action, int column, const DataBlock &binding);
//! reset control bindings for specified action and specified column (or -1 to reset all columns)
void reset_action_binding(action_handle_t action, int column);
//! returns control bindings for specified action and column (digital)
DigitalActionBinding *get_digital_action_binding(action_handle_t action, int column);
//! returns control bindings for specified action and column (analogue axis)
AnalogAxisActionBinding *get_analog_axis_action_binding(action_handle_t action, int column);
//! returns control bindings for specified action and column (analogue stick)
AnalogStickActionBinding *get_analog_stick_action_binding(action_handle_t action, int column);

//! returns smoothValue of stick action
float get_analog_stick_action_smooth_value(action_handle_t action);
//! returns smoothValue of stick action
void set_analog_stick_action_smooth_value(action_handle_t action, float val);

//! returns total actions count
int get_actions_count();
//! returns action by ordinal index
action_handle_t get_action_handle_by_ord(int ord_idx);
//! returns total action sets count
int get_action_sets_count();
//! returns action set by ordinal index
action_set_handle_t get_action_set_handle_by_ord(int ord_idx);

//! starts bindings recording for specified action (results are stored to temp buffer and retrived with finish_recording_bindings)
void start_recording_bindings(action_handle_t action);
//! starts bindings recording for SingleButtonId (results are stored to temp buffer and retrived with finish_recording_bindings)
void start_recording_bindings_for_single_button();
//! return true when recording is started and not finished
bool is_recording_in_progress();
//! return true when recording is complete
bool is_recording_complete();
//! finishes bindings recording and returns true when binding is complete (bindings is store to out_binding)
bool finish_recording_bindings(DataBlock &out_binding);

//! checks whether specified binding for specified action conflicts with other actions (in relevant actionsets)
bool check_bindings_conflicts(action_handle_t action, const DataBlock &binding, Tab<action_handle_t> &out_a, Tab<int> &out_c);
//! checks whether specified binding for specified action conflicts with another action
bool check_bindings_conflicts_one(action_handle_t a1, int column1, action_handle_t a2, int column2);
//! checks whether specified binding for specified action hides some actions (in other actionset layers)
bool check_bindings_hides_action(action_handle_t action, const DataBlock &binding, Tab<action_handle_t> &out_a, Tab<int> &out_c);


//! setup initial usage mask (to filter out conflicting actions) to be used actions processing (by default mask is empty); ord_idx=0..3
void set_initial_usage_mask(dag::ConstSpan<SingleButtonId> disable_used_buttons, int ord_idx = 0);

//! resets sticky toggle for digital action (if it is not 0); return false when nothing changed
bool reset_digital_action_sticky_toggle(action_handle_t action);

//! programmatically sends event for specified action
void send_action_event(action_handle_t action);
//! programmatically sends event for specified action (action terminated)
void send_action_terminated_event(action_handle_t action, int dur_msec = 0);

//! per-frame update to process actions
void process_actions_tick(actions_processed_t on_input_sample_processed = nullptr);

//! notify dainput on keyboard event (button press/release)
void kbd_btn_event_occurs(int64_t t_usec, int btn_id, bool pressed);
//! notify dainput on mouse event (button press/release)
void mouse_btn_event_occurs(int64_t t_usec, int btn_id, bool pressed);
//! notify dainput on mouse wheel event (wheel up/down)
void mouse_wheel_event_occurs(int64_t t_usec, int wheel_inc);
//! notify dainput on mouse move event
void mouse_move_event_occurs(int64_t t_usec, float dx, float dy, int scr_x, int scr_y);
//! notify dainput on touch event (we need only touch begin event for switch control type)
void touch_event_occurs(int64_t t_usec);
//! notify dainput on gamepad event
void gpad_change_event_occurs(int64_t t_usec, HumanInput::IGenJoystick *gamepad);
//! notify dainput on joystick event
void joy_change_event_occurs(int64_t t_usec, HumanInput::IGenJoystick *joy);
//! notify dainput on regular tick
void tick_event_occurs(int64_t t_usec);

enum
{
  DEV_USED_mouse = 1,
  DEV_USED_kbd = 2,
  DEV_USED_gamepad = 4,
  DEV_USED_touch = 8
};
//! returns mask of drivers that reported any event for recent 'past_frames_threshold' frames
unsigned get_last_used_device_mask(unsigned past_frames_threshold = 100);

//! returns count of button clicks (press+release) since dainput init for given device mask
unsigned get_overall_button_clicks_count(unsigned dev_used_mask = DEV_USED_mouse | DEV_USED_kbd | DEV_USED_gamepad);
//! enable/disable reporting darg events for each button click (for specified device mask)
void enable_darg_events_for_button_clicks(bool enable,
  unsigned dev_used_mask = DEV_USED_mouse | DEV_USED_kbd | DEV_USED_gamepad | DEV_USED_touch);

//! returns true when input msg buffer level is sufficient to receive messages
bool is_safe_to_sample_input();
} // namespace dainput

namespace dainput
{
//! dumps actions sets and actions to log
void dump_action_sets();
//! dumps control bindings for all actions to log
void dump_action_bindings();
//! dumps current active stack of action setst to log
void dump_action_sets_stack();

//! build canonical name for button/axis to out_name and return out_name.str()
const char *build_ctrl_name(String &out_name, int dev, int btn_or_axis, bool is_btn = true);
//! build canonical name for button to out_name and return out_name.str()
inline const char *build_btn_name(String &out_name, SingleButtonId id) { return build_ctrl_name(out_name, id.devId, id.btnId); }
//! returns text name for DigitalActionBinding event type (BTN_*)
const char *get_digital_button_type_name(int eventType);

//! enables or disables debug traces for actionset management and events (with visuallog)
void enable_debug_traces(bool enable_actionset_logs, bool enable_event_logs);

//! sets stick deadzone [0, 1)
void set_stick_dead_zone(int stick_idx, bool main_dev, float dzone);
//! returns current stick deadzone
float get_stick_dead_zone(int stick_idx, bool main_dev);
//! returns current stick deadzone scale
float get_stick_dead_zone_scale(int stick_idx, bool main_dev);
//! returns actual stick deadzone in device
float get_stick_dead_zone_abs(int stick_idx, bool main_dev);
//! reapply current deadzones (useful after device refresh)
void reapply_stick_dead_zones();

//! enable/disable gyroscope in device
void enable_gyroscope(bool enable, bool main_dev);
} // namespace dainput
