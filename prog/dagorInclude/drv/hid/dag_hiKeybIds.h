//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace HumanInput
{

// Keyboard button IDs (scancodes).
enum
{
  DKEY_ESCAPE = 0x01,
  DKEY_1 = 0x02,
  DKEY_2 = 0x03,
  DKEY_3 = 0x04,
  DKEY_4 = 0x05,
  DKEY_5 = 0x06,
  DKEY_6 = 0x07,
  DKEY_7 = 0x08,
  DKEY_8 = 0x09,
  DKEY_9 = 0x0A,
  DKEY_0 = 0x0B,
  DKEY_MINUS = 0x0C, /* - on main keyboard */
  DKEY_EQUALS = 0x0D,
  DKEY_BACK = 0x0E, /* backspace */
  DKEY_TAB = 0x0F,
  DKEY_Q = 0x10,
  DKEY_W = 0x11,
  DKEY_E = 0x12,
  DKEY_R = 0x13,
  DKEY_T = 0x14,
  DKEY_Y = 0x15,
  DKEY_U = 0x16,
  DKEY_I = 0x17,
  DKEY_O = 0x18,
  DKEY_P = 0x19,
  DKEY_LBRACKET = 0x1A,
  DKEY_RBRACKET = 0x1B,
  DKEY_RETURN = 0x1C, /* Enter on main keyboard */
  DKEY_LCONTROL = 0x1D,
  DKEY_A = 0x1E,
  DKEY_S = 0x1F,
  DKEY_D = 0x20,
  DKEY_F = 0x21,
  DKEY_G = 0x22,
  DKEY_H = 0x23,
  DKEY_J = 0x24,
  DKEY_K = 0x25,
  DKEY_L = 0x26,
  DKEY_SEMICOLON = 0x27,
  DKEY_APOSTROPHE = 0x28,
  DKEY_GRAVE = 0x29, /* accent grave */
  DKEY_LSHIFT = 0x2A,
  DKEY_BACKSLASH = 0x2B,
  DKEY_Z = 0x2C,
  DKEY_X = 0x2D,
  DKEY_C = 0x2E,
  DKEY_V = 0x2F,
  DKEY_B = 0x30,
  DKEY_N = 0x31,
  DKEY_M = 0x32,
  DKEY_COMMA = 0x33,
  DKEY_PERIOD = 0x34, /* . on main keyboard */
  DKEY_SLASH = 0x35,  /* / on main keyboard */
  DKEY_RSHIFT = 0x36,
  DKEY_MULTIPLY = 0x37, /* * on numeric keypad */
  DKEY_LALT = 0x38,     /* left Alt */
  DKEY_SPACE = 0x39,
  DKEY_CAPITAL = 0x3A,
  DKEY_F1 = 0x3B,
  DKEY_F2 = 0x3C,
  DKEY_F3 = 0x3D,
  DKEY_F4 = 0x3E,
  DKEY_F5 = 0x3F,
  DKEY_F6 = 0x40,
  DKEY_F7 = 0x41,
  DKEY_F8 = 0x42,
  DKEY_F9 = 0x43,
  DKEY_F10 = 0x44,
  DKEY_NUMLOCK = 0x45,
  DKEY_SCROLL = 0x46, /* Scroll Lock */
  DKEY_NUMPAD7 = 0x47,
  DKEY_NUMPAD8 = 0x48,
  DKEY_NUMPAD9 = 0x49,
  DKEY_SUBTRACT = 0x4A, /* - on numeric keypad */
  DKEY_NUMPAD4 = 0x4B,
  DKEY_NUMPAD5 = 0x4C,
  DKEY_NUMPAD6 = 0x4D,
  DKEY_ADD = 0x4E, /* + on numeric keypad */
  DKEY_NUMPAD1 = 0x4F,
  DKEY_NUMPAD2 = 0x50,
  DKEY_NUMPAD3 = 0x51,
  DKEY_NUMPAD0 = 0x52,
  DKEY_DECIMAL = 0x53, /* . on numeric keypad */
  DKEY_OEM_102 = 0x56, /* <> or \| on RT 102-key keyboard (Non-U.S.) */
  DKEY_F11 = 0x57,
  DKEY_F12 = 0x58,
  DKEY_F13 = 0x64,          /*                     (NEC PC98) */
  DKEY_F14 = 0x65,          /*                     (NEC PC98) */
  DKEY_F15 = 0x66,          /*                     (NEC PC98) */
  DKEY_KANA = 0x70,         /* (Japanese keyboard)            */
  DKEY_ABNT_C1 = 0x73,      /* /? on Brazilian keyboard */
  DKEY_CONVERT = 0x79,      /* (Japanese keyboard)            */
  DKEY_NOCONVERT = 0x7B,    /* (Japanese keyboard)            */
  DKEY_YEN = 0x7D,          /* (Japanese keyboard)            */
  DKEY_ABNT_C2 = 0x7E,      /* Numpad . on Brazilian keyboard */
  DKEY_NUMPADEQUALS = 0x8D, /* = on numeric keypad (NEC PC98) */
  DKEY_PREVTRACK = 0x90,    /* Previous Track (DIK_CIRCUMFLEX on Japanese keyboard) */
  DKEY_AT = 0x91,           /*                     (NEC PC98) */
  DKEY_COLON = 0x92,        /*                     (NEC PC98) */
  DKEY_UNDERLINE = 0x93,    /*                     (NEC PC98) */
  DKEY_KANJI = 0x94,        /* (Japanese keyboard)            */
  DKEY_STOP = 0x95,         /*                     (NEC PC98) */
  DKEY_AX = 0x96,           /*                     (Japan AX) */
  DKEY_UNLABELED = 0x97,    /*                        (J3100) */
  DKEY_NEXTTRACK = 0x99,    /* Next Track */
  DKEY_NUMPADENTER = 0x9C,  /* Enter on numeric keypad */
  DKEY_RCONTROL = 0x9D,
  DKEY_MUTE = 0xA0,        /* Mute */
  DKEY_CALCULATOR = 0xA1,  /* Calculator */
  DKEY_PLAYPAUSE = 0xA2,   /* Play / Pause */
  DKEY_MEDIASTOP = 0xA4,   /* Media Stop */
  DKEY_VOLUMEDOWN = 0xAE,  /* Volume - */
  DKEY_VOLUMEUP = 0xB0,    /* Volume + */
  DKEY_WEBHOME = 0xB2,     /* Web home */
  DKEY_NUMPADCOMMA = 0xB3, /* , on numeric keypad (NEC PC98) */
  DKEY_DIVIDE = 0xB5,      /* / on numeric keypad */
  DKEY_SYSRQ = 0xB7,
  DKEY_RALT = 0xB8,         /* right Alt */
  DKEY_PAUSE = 0xC5,        /* Pause */
  DKEY_HOME = 0xC7,         /* Home on arrow keypad */
  DKEY_UP = 0xC8,           /* UpArrow on arrow keypad */
  DKEY_PRIOR = 0xC9,        /* PgUp on arrow keypad */
  DKEY_LEFT = 0xCB,         /* LeftArrow on arrow keypad */
  DKEY_RIGHT = 0xCD,        /* RightArrow on arrow keypad */
  DKEY_END = 0xCF,          /* End on arrow keypad */
  DKEY_DOWN = 0xD0,         /* DownArrow on arrow keypad */
  DKEY_NEXT = 0xD1,         /* PgDn on arrow keypad */
  DKEY_INSERT = 0xD2,       /* Insert on arrow keypad */
  DKEY_DELETE = 0xD3,       /* Delete on arrow keypad */
  DKEY_LWIN = 0xDB,         /* Left Windows key */
  DKEY_RWIN = 0xDC,         /* Right Windows key */
  DKEY_APPS = 0xDD,         /* AppMenu key */
  DKEY_POWER = 0xDE,        /* System Power */
  DKEY_SLEEP = 0xDF,        /* System Sleep */
  DKEY_WAKE = 0xE3,         /* System Wake */
  DKEY_WEBSEARCH = 0xE5,    /* Web Search */
  DKEY_WEBFAVORITES = 0xE6, /* Web Favorites */
  DKEY_WEBREFRESH = 0xE7,   /* Web Refresh */
  DKEY_WEBSTOP = 0xE8,      /* Web Stop */
  DKEY_WEBFORWARD = 0xE9,   /* Web Forward */
  DKEY_WEBBACK = 0xEA,      /* Web Back */
  DKEY_MYCOMPUTER = 0xEB,   /* My Computer */
  DKEY_MAIL = 0xEC,         /* Mail */
  DKEY_MEDIASELECT = 0xED,  /* Media Select */

  DKEY__MAX_BUTTONS
};

} // namespace HumanInput
