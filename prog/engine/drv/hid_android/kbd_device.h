// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiKeybIds.h>
#include <android/keycodes.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <string.h>
// #include <debug/dag_debug.h>

namespace HumanInput
{
class ScreenKeyboardDevice : public IGenKeyboard, public IWndProcComponent
{
public:
  ScreenKeyboardDevice()
  {
    client = NULL;
    memset(&state, 0, sizeof(state));
    memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));
    add_wnd_proc_component(this);
  }
  ~ScreenKeyboardDevice()
  {
    setClient(NULL);
    del_wnd_proc_component(this);
  }

  // IGenKeyboard interface implementation
  virtual const KeyboardRawState &getRawState() const { return state; }
  virtual void setClient(IGenKeyboardClient *cli)
  {
    if (cli == client)
      return;

    if (client)
      client->detached(this);
    client = cli;
    if (client)
      client->attached(this);
  }
  virtual IGenKeyboardClient *getClient() const { return client; }

  virtual int getKeyCount() const { return DKEY__MAX_BUTTONS; }
  virtual const char *getKeyName(int idx) const
  {
    static const char *keyNames[DKEY__MAX_BUTTONS] = {0};
    if (keyNames[0])
      return keyNames[idx];

    struct DrvMappedKeyNames
    {
      int hid_id;
      const char *name;
    };
    static DrvMappedKeyNames kbdMappedNames[] = {
      {HumanInput::DKEY_ESCAPE, "Esc"},
      {HumanInput::DKEY_1, "1"},
      {HumanInput::DKEY_2, "2"},
      {HumanInput::DKEY_3, "3"},
      {HumanInput::DKEY_4, "4"},
      {HumanInput::DKEY_5, "5"},
      {HumanInput::DKEY_6, "6"},
      {HumanInput::DKEY_7, "7"},
      {HumanInput::DKEY_8, "8"},
      {HumanInput::DKEY_9, "9"},
      {HumanInput::DKEY_0, "0"},
      {HumanInput::DKEY_MINUS, "-"},
      {HumanInput::DKEY_EQUALS, "="},
      {HumanInput::DKEY_BACK, "BackSpace"},
      {HumanInput::DKEY_TAB, "Tab"},
      {HumanInput::DKEY_Q, "Q"},
      {HumanInput::DKEY_W, "W"},
      {HumanInput::DKEY_E, "E"},
      {HumanInput::DKEY_R, "R"},
      {HumanInput::DKEY_T, "T"},
      {HumanInput::DKEY_Y, "Y"},
      {HumanInput::DKEY_U, "U"},
      {HumanInput::DKEY_I, "I"},
      {HumanInput::DKEY_O, "O"},
      {HumanInput::DKEY_P, "P"},
      {HumanInput::DKEY_LBRACKET, "["},
      {HumanInput::DKEY_RBRACKET, "]"},
      {HumanInput::DKEY_RETURN, "Enter"},
      {HumanInput::DKEY_LCONTROL, "ctrl"},
      {HumanInput::DKEY_A, "A"},
      {HumanInput::DKEY_S, "S"},
      {HumanInput::DKEY_D, "D"},
      {HumanInput::DKEY_F, "F"},
      {HumanInput::DKEY_G, "G"},
      {HumanInput::DKEY_H, "H"},
      {HumanInput::DKEY_J, "J"},
      {HumanInput::DKEY_K, "K"},
      {HumanInput::DKEY_L, "L"},
      {HumanInput::DKEY_SEMICOLON, ";"},
      {HumanInput::DKEY_APOSTROPHE, "'"},
      {HumanInput::DKEY_GRAVE, "~"},
      {HumanInput::DKEY_LSHIFT, "L.Shift"},
      {HumanInput::DKEY_BACKSLASH, "\\"},
      {HumanInput::DKEY_Z, "Z"},
      {HumanInput::DKEY_X, "X"},
      {HumanInput::DKEY_C, "C"},
      {HumanInput::DKEY_V, "V"},
      {HumanInput::DKEY_B, "B"},
      {HumanInput::DKEY_N, "N"},
      {HumanInput::DKEY_M, "M"},
      {HumanInput::DKEY_COMMA, ","},
      {HumanInput::DKEY_PERIOD, "."},
      {HumanInput::DKEY_SLASH, "/"},
      {HumanInput::DKEY_RSHIFT, "R.Shift"},
      {HumanInput::DKEY_MULTIPLY, "*"},
      {HumanInput::DKEY_LALT, "L.Alt"},
      {HumanInput::DKEY_SPACE, "Space"},
      {HumanInput::DKEY_CAPITAL, "CapsLock"},
      {HumanInput::DKEY_F1, "F1"},
      {HumanInput::DKEY_F2, "F2"},
      {HumanInput::DKEY_F3, "F3"},
      {HumanInput::DKEY_F4, "F4"},
      {HumanInput::DKEY_F5, "F5"},
      {HumanInput::DKEY_F6, "F6"},
      {HumanInput::DKEY_F7, "F7"},
      {HumanInput::DKEY_F8, "F8"},
      {HumanInput::DKEY_F9, "F9"},
      {HumanInput::DKEY_F10, "F10"},
      {HumanInput::DKEY_NUMLOCK, "NumLock"},
      {HumanInput::DKEY_SCROLL, "ScrLock"},
      {HumanInput::DKEY_NUMPAD7, "Num7"},
      {HumanInput::DKEY_NUMPAD8, "Num8"},
      {HumanInput::DKEY_NUMPAD9, "Num9"},
      {HumanInput::DKEY_SUBTRACT, "Num-"},
      {HumanInput::DKEY_NUMPAD4, "Num4"},
      {HumanInput::DKEY_NUMPAD5, "Num5"},
      {HumanInput::DKEY_NUMPAD6, "Num6"},
      {HumanInput::DKEY_ADD, "Num+"},
      {HumanInput::DKEY_NUMPAD1, "Num1"},
      {HumanInput::DKEY_NUMPAD2, "Num2"},
      {HumanInput::DKEY_NUMPAD3, "Num3"},
      {HumanInput::DKEY_NUMPAD0, "Num0"},
      {HumanInput::DKEY_DECIMAL, "Num."},
      {HumanInput::DKEY_F11, "F11"},
      {HumanInput::DKEY_F12, "F12"},
      {HumanInput::DKEY_NUMPADENTER, "NumEnter"},
      {HumanInput::DKEY_DIVIDE, "Num/"},
      {HumanInput::DKEY_SYSRQ, "SysRq"},
      {HumanInput::DKEY_RALT, "R.Alt"},
      {HumanInput::DKEY_PAUSE, "Pause"},
      {HumanInput::DKEY_HOME, "Home"},
      {HumanInput::DKEY_UP, "Up"},
      {HumanInput::DKEY_PRIOR, "PageUp"},
      {HumanInput::DKEY_LEFT, "Left"},
      {HumanInput::DKEY_RIGHT, "Right"},
      {HumanInput::DKEY_END, "End"},
      {HumanInput::DKEY_DOWN, "Down"},
      {HumanInput::DKEY_NEXT, "PageDown"},
      {HumanInput::DKEY_INSERT, "Insert"},
      {HumanInput::DKEY_DELETE, "Delete"},
      {HumanInput::DKEY_LWIN, "L.Cmd"},
      {HumanInput::DKEY_RWIN, "R.Cmd"},
      {HumanInput::DKEY_APPS, "AppMenu"},
    };

    memset(keyNames, 0, sizeof(keyNames));
    keyNames[0] = "";
    for (int i = 0; i < sizeof(kbdMappedNames) / sizeof(kbdMappedNames[0]); i++)
      keyNames[kbdMappedNames[i].hid_id] = kbdMappedNames[i].name;
    return keyNames[idx];
  }

  virtual const char *getName() const { return "Keyboard@Screen-android"; }

  // IWndProcComponent interface implementation
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
  {
    static short key_remap[] = {
      0,                      // AKEYCODE_UNKNOWN
      0,                      // AKEYCODE_SOFT_LEFT
      0,                      // AKEYCODE_SOFT_RIGHT
      0,                      // AKEYCODE_HOME
      DKEY_ESCAPE,            // AKEYCODE_BACK
      0,                      // AKEYCODE_CALL
      0,                      // AKEYCODE_ENDCALL
      DKEY_0,                 // AKEYCODE_0
      DKEY_1,                 // AKEYCODE_1
      DKEY_2,                 // AKEYCODE_2
      DKEY_3,                 // AKEYCODE_3
      DKEY_4,                 // AKEYCODE_4
      DKEY_5,                 // AKEYCODE_5
      DKEY_6,                 // AKEYCODE_6
      DKEY_7,                 // AKEYCODE_7
      DKEY_8,                 // AKEYCODE_8
      DKEY_9,                 // AKEYCODE_9
      0,                      // AKEYCODE_STAR
      0,                      // AKEYCODE_POUND
      DKEY_UP,                // AKEYCODE_DPAD_UP
      DKEY_DOWN,              // AKEYCODE_DPAD_DOWN
      DKEY_LEFT,              // AKEYCODE_DPAD_LEFT
      DKEY_RIGHT,             // AKEYCODE_DPAD_RIGHT
      0,                      // AKEYCODE_DPAD_CENTER
      DKEY_VOLUMEUP,          // AKEYCODE_VOLUME_UP
      DKEY_VOLUMEDOWN,        // AKEYCODE_VOLUME_DOWN
      DKEY_POWER,             // AKEYCODE_POWER
      0,                      // AKEYCODE_CAMERA
      0,                      // AKEYCODE_CLEAR
      DKEY_A,                 // AKEYCODE_A
      DKEY_B,                 // AKEYCODE_B
      DKEY_C,                 // AKEYCODE_C
      DKEY_D,                 // AKEYCODE_D
      DKEY_E,                 // AKEYCODE_E
      DKEY_F,                 // AKEYCODE_F
      DKEY_G,                 // AKEYCODE_G
      DKEY_H,                 // AKEYCODE_H
      DKEY_I,                 // AKEYCODE_I
      DKEY_J,                 // AKEYCODE_J
      DKEY_K,                 // AKEYCODE_K
      DKEY_L,                 // AKEYCODE_L
      DKEY_M,                 // AKEYCODE_M
      DKEY_N,                 // AKEYCODE_N
      DKEY_O,                 // AKEYCODE_O
      DKEY_P,                 // AKEYCODE_P
      DKEY_Q,                 // AKEYCODE_Q
      DKEY_R,                 // AKEYCODE_R
      DKEY_S,                 // AKEYCODE_S
      DKEY_T,                 // AKEYCODE_T
      DKEY_U,                 // AKEYCODE_U
      DKEY_V,                 // AKEYCODE_V
      DKEY_W,                 // AKEYCODE_W
      DKEY_X,                 // AKEYCODE_X
      DKEY_Y,                 // AKEYCODE_Y
      DKEY_Z,                 // AKEYCODE_Z
      DKEY_COMMA,             // AKEYCODE_COMMA
      DKEY_PERIOD,            // AKEYCODE_PERIOD
      DKEY_LALT,              // AKEYCODE_ALT_LEFT
      DKEY_RALT,              // AKEYCODE_ALT_RIGHT
      DKEY_LSHIFT,            // AKEYCODE_SHIFT_LEFT
      DKEY_RSHIFT,            // AKEYCODE_SHIFT_RIGHT
      DKEY_TAB,               // AKEYCODE_TAB
      DKEY_SPACE,             // AKEYCODE_SPACE
      0,                      // AKEYCODE_SYM
      0,                      // AKEYCODE_EXPLORER
      0,                      // AKEYCODE_ENVELOPE
      DKEY_RETURN,            // AKEYCODE_ENTER
      DKEY_BACK,              // AKEYCODE_DEL
      DKEY_GRAVE,             // AKEYCODE_GRAVE
      DKEY_MINUS,             // AKEYCODE_MINUS
      DKEY_EQUALS,            // AKEYCODE_EQUALS
      DKEY_LBRACKET,          // AKEYCODE_LEFT_BRACKET
      DKEY_RBRACKET,          // AKEYCODE_RIGHT_BRACKET
      DKEY_BACKSLASH,         // AKEYCODE_BACKSLASH
      DKEY_SEMICOLON,         // AKEYCODE_SEMICOLON
      DKEY_APOSTROPHE,        // AKEYCODE_APOSTROPHE
      DKEY_SLASH,             // AKEYCODE_SLASH
      DKEY_AT,                // AKEYCODE_AT
      0,                      // AKEYCODE_NUM
      0,                      // AKEYCODE_HEADSETHOOK
      0,                      // AKEYCODE_FOCUS
      0,                      // AKEYCODE_PLUS
      DKEY_APPS,              // AKEYCODE_MENU
      0,                      // AKEYCODE_NOTIFICATION
      DKEY_WEBSEARCH,         // AKEYCODE_SEARCH
      DKEY_PLAYPAUSE,         // AKEYCODE_MEDIA_PLAY_PAUSE
      DKEY_MEDIASTOP,         // AKEYCODE_MEDIA_STOP
      DKEY_NEXTTRACK,         // AKEYCODE_MEDIA_NEXT
      DKEY_PREVTRACK,         // AKEYCODE_MEDIA_PREVIOUS
      0,                      // AKEYCODE_MEDIA_REWIND
      0,                      // AKEYCODE_MEDIA_FAST_FORWARD
      DKEY_MUTE,              // AKEYCODE_MUTE
      DKEY_PRIOR,             // AKEYCODE_PAGE_UP
      DKEY_NEXT,              // AKEYCODE_PAGE_DOWN
      0,                      // AKEYCODE_PICTSYMBOLS
      0,                      // AKEYCODE_SWITCH_CHARSET
      AKEYCODE_BUTTON_A,      // AKEYCODE_BUTTON_A
      AKEYCODE_BUTTON_B,      // AKEYCODE_BUTTON_B
      AKEYCODE_BUTTON_C,      // AKEYCODE_BUTTON_C
      AKEYCODE_BUTTON_X,      // AKEYCODE_BUTTON_X
      AKEYCODE_BUTTON_Y,      // AKEYCODE_BUTTON_Y
      AKEYCODE_BUTTON_Z,      // AKEYCODE_BUTTON_Z
      AKEYCODE_BUTTON_L1,     // AKEYCODE_BUTTON_L1
      AKEYCODE_BUTTON_R1,     // AKEYCODE_BUTTON_R1
      AKEYCODE_BUTTON_L2,     // AKEYCODE_BUTTON_L2
      AKEYCODE_BUTTON_R2,     // AKEYCODE_BUTTON_R2
      AKEYCODE_BUTTON_THUMBL, // AKEYCODE_BUTTON_THUMBL
      AKEYCODE_BUTTON_THUMBR, // AKEYCODE_BUTTON_THUMBR
      DKEY_RETURN,            // AKEYCODE_BUTTON_START
      AKEYCODE_BUTTON_SELECT, // AKEYCODE_BUTTON_SELECT
      AKEYCODE_BUTTON_MODE,   // AKEYCODE_BUTTON_MODE
      DKEY_ESCAPE,            // AKEYCODE_ESCAPE
      DKEY_DELETE,            // AKEYCODE_FORWARD_DEL
      DKEY_LCONTROL,          // AKEYCODE_CTRL_LEFT
      DKEY_RCONTROL,          // AKEYCODE_CTRL_RIGHT
      DKEY_CAPITAL,           // AKEYCODE_CAPS_LOCK
      DKEY_SCROLL,            // AKEYCODE_SCROLL_LOCK
      DKEY_LWIN,              // AKEYCODE_META_LEFT
      DKEY_RWIN,              // AKEYCODE_META_RIGHT
      0,                      // AKEYCODE_FUNCTION
      DKEY_SYSRQ,             // AKEYCODE_SYSRQ
      DKEY_PAUSE,             // AKEYCODE_BREAK
      DKEY_HOME,              // AKEYCODE_MOVE_HOME
      DKEY_END,               // AKEYCODE_MOVE_END
      DKEY_INSERT,            // AKEYCODE_INSERT
      0,                      // AKEYCODE_FORWARD
      0,                      // AKEYCODE_MEDIA_PLAY
      0,                      // AKEYCODE_MEDIA_PAUSE
      0,                      // AKEYCODE_MEDIA_CLOSE
      0,                      // AKEYCODE_MEDIA_EJECT
      0,                      // AKEYCODE_MEDIA_RECORD
      DKEY_F1,                // AKEYCODE_F1
      DKEY_F2,                // AKEYCODE_F2
      DKEY_F3,                // AKEYCODE_F3
      DKEY_F4,                // AKEYCODE_F4
      DKEY_F5,                // AKEYCODE_F5
      DKEY_F6,                // AKEYCODE_F6
      DKEY_F7,                // AKEYCODE_F7
      DKEY_F8,                // AKEYCODE_F8
      DKEY_F9,                // AKEYCODE_F9
      DKEY_F10,               // AKEYCODE_F10
      DKEY_F11,               // AKEYCODE_F11
      DKEY_F12,               // AKEYCODE_F12
      DKEY_NUMLOCK,           // AKEYCODE_NUM_LOCK
      DKEY_NUMPAD0,           // AKEYCODE_NUMPAD_0
      DKEY_NUMPAD1,           // AKEYCODE_NUMPAD_1
      DKEY_NUMPAD2,           // AKEYCODE_NUMPAD_2
      DKEY_NUMPAD3,           // AKEYCODE_NUMPAD_3
      DKEY_NUMPAD4,           // AKEYCODE_NUMPAD_4
      DKEY_NUMPAD5,           // AKEYCODE_NUMPAD_5
      DKEY_NUMPAD6,           // AKEYCODE_NUMPAD_6
      DKEY_NUMPAD7,           // AKEYCODE_NUMPAD_7
      DKEY_NUMPAD8,           // AKEYCODE_NUMPAD_8
      DKEY_NUMPAD9,           // AKEYCODE_NUMPAD_9
      DKEY_DIVIDE,            // AKEYCODE_NUMPAD_DIVIDE
      DKEY_MULTIPLY,          // AKEYCODE_NUMPAD_MULTIPLY
      DKEY_SUBTRACT,          // AKEYCODE_NUMPAD_SUBTRACT
      DKEY_ADD,               // AKEYCODE_NUMPAD_ADD
      DKEY_DECIMAL,           // AKEYCODE_NUMPAD_DOT
      DKEY_NUMPADCOMMA,       // AKEYCODE_NUMPAD_COMMA
      DKEY_NUMPADENTER,       // AKEYCODE_NUMPAD_ENTER
      DKEY_NUMPADEQUALS,      // AKEYCODE_NUMPAD_EQUALS
      0,                      // AKEYCODE_NUMPAD_LEFT_PAREN
      0,                      // AKEYCODE_NUMPAD_RIGHT_PAREN
      DKEY_MUTE,              // AKEYCODE_VOLUME_MUTE
      0,                      // AKEYCODE_INFO
      0,                      // AKEYCODE_CHANNEL_UP
      0,                      // AKEYCODE_CHANNEL_DOWN
      0,                      // AKEYCODE_ZOOM_IN
      0,                      // AKEYCODE_ZOOM_OUT
      0,                      // AKEYCODE_TV
      0,                      // AKEYCODE_WINDOW
      0,                      // AKEYCODE_GUIDE
      0,                      // AKEYCODE_DVR
      0,                      // AKEYCODE_BOOKMARK
      0,                      // AKEYCODE_CAPTIONS
      0,                      // AKEYCODE_SETTINGS
      0,                      // AKEYCODE_TV_POWER
      0,                      // AKEYCODE_TV_INPUT
      0,                      // AKEYCODE_STB_POWER
      0,                      // AKEYCODE_STB_INPUT
      0,                      // AKEYCODE_AVR_POWER
      0,                      // AKEYCODE_AVR_INPUT
      0,                      // AKEYCODE_PROG_RED
      0,                      // AKEYCODE_PROG_GREEN
      0,                      // AKEYCODE_PROG_YELLOW
      0,                      // AKEYCODE_PROG_BLUE
      0,                      // AKEYCODE_APP_SWITCH
      0,                      // AKEYCODE_BUTTON_1
      0,                      // AKEYCODE_BUTTON_2
      0,                      // AKEYCODE_BUTTON_3
      0,                      // AKEYCODE_BUTTON_4
      0,                      // AKEYCODE_BUTTON_5
      0,                      // AKEYCODE_BUTTON_6
      0,                      // AKEYCODE_BUTTON_7
      0,                      // AKEYCODE_BUTTON_8
      0,                      // AKEYCODE_BUTTON_9
      0,                      // AKEYCODE_BUTTON_10
      0,                      // AKEYCODE_BUTTON_11
      0,                      // AKEYCODE_BUTTON_12
      0,                      // AKEYCODE_BUTTON_13
      0,                      // AKEYCODE_BUTTON_14
      0,                      // AKEYCODE_BUTTON_15
      0,                      // AKEYCODE_BUTTON_16
      0,                      // AKEYCODE_LANGUAGE_SWITCH
      0,                      // AKEYCODE_MANNER_MODE
      0,                      // AKEYCODE_3D_MODE
      0,                      // AKEYCODE_CONTACTS
      0,                      // AKEYCODE_CALENDAR
      0,                      // AKEYCODE_MUSIC
      0,                      // AKEYCODE_CALCULATOR
      0,                      // AKEYCODE_ZENKAKU_HANKAKU
      0,                      // AKEYCODE_EISU
      0,                      // AKEYCODE_MUHENKAN
      0,                      // AKEYCODE_HENKAN
      0,                      // AKEYCODE_KATAKANA_HIRAGANA
      0,                      // AKEYCODE_YEN
      0,                      // AKEYCODE_RO
      0,                      // AKEYCODE_KANA
      0,                      // AKEYCODE_ASSIST
      0,                      // AKEYCODE_BRIGHTNESS_DOWN
      0,                      // AKEYCODE_BRIGHTNESS_UP
      0,                      // AKEYCODE_MEDIA_AUDIO_TRACK
    };
    if (msg == GPCM_KeyPress || msg == GPCM_KeyRelease)
    {
      // debug("%s %d", msg == GPCM_KeyPress ? "dn" : "up", wParam);
      int keyIdx = -1;
      if (wParam > 0 && wParam < sizeof(key_remap) / sizeof(short) && key_remap[wParam])
        keyIdx = key_remap[wParam];
      // debug("key %d -> %d", wParam, keyIdx);
      if (keyIdx >= 0)
      {
        if (msg == GPCM_KeyPress)
        {
          state.setKeyDown(keyIdx);
          raw_state_kbd.setKeyDown(keyIdx);
          if (client)
            client->gkcButtonDown(this, keyIdx, false, lParam);
        }
        else
        {
          state.clrKeyDown(keyIdx);
          raw_state_kbd.clrKeyDown(keyIdx);
          if (client)
            client->gkcButtonUp(this, keyIdx);
        }
      }
    }
    else if (msg == GPCM_Char && client)
      client->gkcButtonDown(this, 0, false, wParam);
    return PROCEED_OTHER_COMPONENTS;
  }

protected:
  IGenKeyboardClient *client;
  KeyboardRawState state;
};
} // namespace HumanInput
