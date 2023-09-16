//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <util/dag_simpleString.h>

enum
{
  CONTROL_CYCLED_VALUES = 0x100,
  CONTROL_APROXIMATION_MASK = 0xFF,

  CURVE_LINEAR_APP = 0,
  CURVE_CMR_APP,
  CURVE_NURB_APP,
  CURVE_QUAD_APP,
  CURVE_CUBICPOLYNOM_APP,
  CURVE_CUBIC_P_SPLINE_APP,

  CURVE_MIN_MAX_POINTS = 1,
  CURVE_MIN_MAX_X,
  CURVE_MIN_MAX_Y,

  RADIO_SELECT_NONE = 0x7FFFFFFF,

  FS_DIALOG_NONE = 0x00,
  FS_DIALOG_OPEN_FILE = 0x01,
  FS_DIALOG_SAVE_FILE = 0x02,
  FS_DIALOG_DIRECTORY = 0x03,

  EXT_BUTTON_NONE = 0,
  EXT_BUTTON_INSERT,
  EXT_BUTTON_APPEND,
  EXT_BUTTON_REMOVE,
  EXT_BUTTON_UP,
  EXT_BUTTON_DOWN,
  EXT_BUTTON_RENAME,
};

enum
{
  GRADIENT_COLOR_MODE_H = 0,
  GRADIENT_COLOR_MODE_S,
  GRADIENT_COLOR_MODE_V,

  GRADIENT_COLOR_MODE_HSV,

  GRADIENT_COLOR_MODE_R,
  GRADIENT_COLOR_MODE_G,
  GRADIENT_COLOR_MODE_B,

  GRADIENT_COLOR_MODE_RGB,

  GRADIENT_MODE_NONE,

  GRADIENT_AXIS_MODE_X = 0x100,
  GRADIENT_AXIS_MODE_Y = 0x200,
  GRADIENT_AXIS_MODE_XY = 0x400,

  COLOR_MASK = 0x00FF,
  AXIS_MASK = 0xFF00,
};

constexpr float TRACKBAR_DEFAULT_POWER = 1.0f;

struct GradientKey
{
  GradientKey(){};
  GradientKey(float _position, E3DCOLOR _color) : position(_position), color(_color) {}

  float position = 0;
  E3DCOLOR color = 0;
};

struct TextGradientKey
{
  TextGradientKey(){};
  TextGradientKey(float _position, const char *_text) : position(_position), text(_text) {}
  TextGradientKey(const TextGradientKey &new_key) { operator=(new_key); }
  TextGradientKey &operator=(const TextGradientKey &new_key)
  {
    position = new_key.position;
    text = new_key.text;
    return *this;
  }

  float position = 0;
  SimpleString text;
};

typedef Tab<GradientKey> Gradient, *PGradient;
typedef Tab<TextGradientKey> TextGradient;

typedef struct Pp2OpaqueLeafHandle *TLeafHandle;
typedef void *TFontHandle;
typedef void *TInstHandle;
