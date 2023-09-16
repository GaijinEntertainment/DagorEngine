//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>


#include <gui/dag_eventClass.h>
#include <humanInput/dag_hiPointingData.h>

class ShortcutGroup;

class DataBlock;


namespace HumanInput
{
class IGenKeyboard;
class IGenPointing;
class IGenJoystick;
}; // namespace HumanInput

enum
{
  NULL_INPUT_DEVICE_ID = 0,

  STD_MOUSE_DEVICE_ID,
  STD_KEYBOARD_DEVICE_ID,
  DEF_JOYSTICK_DEVICE_ID,
  STD_GESTURE_DEVICE_ID = 36, // Because IDs in scriptedGuiBhv have to match. TODO: fix copypasta
};

void init_shortcuts();
void shutdown_shortcuts();

ShortcutGroup *create_shortcut_group(const char *group_name, const EventClassId *events, int num_events);

void release_shortcut_group(ShortcutGroup *group);


ShortcutGroup *get_shortcut_group(const char *group_name);


// Puts shortcut group on top of the stack, this group will check events
// for shortcuts first in check_shortcut(), before any other groups below
// it on the stack.
void activate_shortcut_group(ShortcutGroup *group);

// Removes shortcut group from the stack.
void deactivate_shortcut_group(ShortcutGroup *group);

// Returns shortcut id, or NULL_NAME_ID if event doesn't trigger a shortcut.
// Shortcut is triggered when any of its buttons are released after all of
// them being pressed.
// Only shortcut groups currently on the stack are checked for shortcuts.
EventClassId check_shortcut();


// Returns true if specified shortcut is active (all buttons assigned to it
// are pressed). Uses current state of input devices.
// You don't need to activate shortcut group in order to check whether the
// short cut is active or not. All registered groups are checked.
bool is_shortcut_active(const EventClassId &shortcut_id, bool use_device_mask = true);


#define MAX_SHORTCUT_BUTTONS 3

typedef int UserInputDeviceId;

struct ShortcutButtonId
{
  UserInputDeviceId deviceId;
  int buttonId;
};

bool is_button_pressed(UserInputDeviceId device_id, int button_id, bool use_device_mask = true);

void add_shortcut(const EventClassId &shortcut_id, ShortcutButtonId *buttons, int num_buttons);

void remove_shortcut(const EventClassId &shortcut_id, ShortcutButtonId *buttons, int num_buttons);

void get_shortcut_buttons(const EventClassId &shortcut_id, int combo_index, ShortcutButtonId *buttons, int &num_buttons,
  bool *joy_combo = NULL);

void get_shortcut_buttons_names(const EventClassId &shortcut_id, int combo_index, String &text);

int get_shortcut_combos_count(const EventClassId &shortcut_id);


void remove_shortcuts(const EventClassId &shortcut_id);


void get_shortcuts_text(const EventClassId &shortcut_id, class String &text);

void set_shortcut_group_mask(const EventClassId &shortcut_id, uint32_t group_mask);


// Returns false if no shortcuts to save.
bool save_shortcuts(DataBlock &blk);

void load_shortcuts(DataBlock &blk);


// Array of buttons must be of size MAX_SHORTCUT_BUTTONS.
void input_shortcut(class GuiNode *node, ShortcutButtonId *buttons, int &num_buttons, int cancel_key = 0);

int groupsNumber();

int getGroupId(const char *name);

char *groupName(int);

int shortcutsNumber(int);

const char *shortcutName(int, int);

int buttonsNumber(int, int);

String *buttonName(int, int, int);

void removeButton(int, int, int);

void removeAllButtons(int group, int shortcut);


void on_shortcuts_kbd_down(HumanInput::IGenKeyboard *kbd, int btn_idx, wchar_t wc);
void on_shortcuts_kbd_up(HumanInput::IGenKeyboard *kbd, int btn_idx);

void on_shortcuts_mouse_down(HumanInput::IGenPointing *mouse, int btn_idx);
void on_shortcuts_mouse_up(HumanInput::IGenPointing *mouse, int btn_idx);
void on_shortcuts_mouse_wheel(HumanInput::IGenPointing *mouse, bool down);
void on_shortcuts_gesture(HumanInput::IGenPointing *, HumanInput::Gesture::State s, const HumanInput::Gesture &g);

void on_shortcuts_joystick_down(HumanInput::IGenJoystick *joy, int btn_id);
void on_shortcuts_joystick_up(HumanInput::IGenJoystick *joy, int btn_id);

void clear_shortcuts_state_buffer();
void clear_shortcuts_keyboard();

// Returns name of button for given device
const char *get_shortcut_button_name(UserInputDeviceId device_id, int button_id);

void set_shortcut_device_mask(unsigned char mask = 0xFF);
