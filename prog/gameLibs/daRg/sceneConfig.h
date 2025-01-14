// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>
#include <generic/dag_tab.h>
#include <sqrat.h>
#include <drv/hid/dag_hiXInputMappings.h>

#include <daRg/dag_guiScene.h>


namespace darg
{

class Cursor;
class GuiScene;

class SceneConfig
{
public:
  SceneConfig(GuiScene *scene_);

  GuiScene *scene = nullptr;

  int defaultFont = 0;
  float defaultFontSize = 0;
  bool gamepadCursorControl = true;
  bool kbCursorControl = false;
  float gamepadCursorSpeed = 1.0f;
  float gamepadCursorDeadZone = 0.15f;
  float gamepadCursorNonLin = 2.5f;
  float gamepadCursorHoverMaxTime = 1.0f;
  float gamepadCursorHoverMinMul = 0.15f;
  float gamepadCursorHoverMaxMul = 0.8f;
  float moveClickThreshold = -1.0f;

  E3DCOLOR defSceneBgColor = E3DCOLOR(10, 10, 10, 160);
  E3DCOLOR defTextColor = E3DCOLOR(160, 160, 160, 160);
  bool reportNestedWatchedUpdate = false;

  int gamepadCursorAxisH = HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H;
  int gamepadCursorAxisV = HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V;

  int joystickScrollAxisH = HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_H;
  int joystickScrollAxisV = HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_V;

  bool clickRumbleEnabled = true;
  float clickRumbleLoFreq = 0.6f;
  float clickRumbleHiFreq = 0.0f;
  float clickRumbleDuration = 0.04f;

  float dirPadRepeatDelay = 0.6f;
  float dirPadRepeatTime = 0.2f;

  float buttonTouchMargin = 0; // initialized in GuiScene constructor

  bool useDefaultCursor = false;
  Sqrat::Object defaultCursor;

  Tab<HotkeyButton> clickButtons;

  bool actionClickByBehavior = false;

  void setDefSceneBgColor(SQInteger color) { defSceneBgColor = E3DCOLOR((unsigned int)color); }
  SQInteger getDefSceneBgColor() const { return defSceneBgColor.u; }

  void setDefTextColor(SQInteger color) { defTextColor = E3DCOLOR((unsigned int)color); }
  SQInteger getDefTextColor() const { return defTextColor.u; }

  bool isClickButton(InputDevice dev_id, int btn_id) const;

  static SQInteger setClickButtons(HSQUIRRELVM vm);
  static SQInteger getClickButtons(HSQUIRRELVM vm);

  static SQInteger setDefaultFontId(HSQUIRRELVM vm);
  static SQInteger getDefaultFontId(HSQUIRRELVM vm);

  static SQInteger setDefaultFontSize(HSQUIRRELVM vm);
  static SQInteger getDefaultFontSize(HSQUIRRELVM vm);
};

} // namespace darg
