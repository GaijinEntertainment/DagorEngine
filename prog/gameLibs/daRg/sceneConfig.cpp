// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sceneConfig.h"
#include "hotkeys.h"
#include "guiScene.h"

#include <drv/hid/dag_hiXInputMappings.h>

namespace darg
{

SceneConfig::SceneConfig(GuiScene *scene_) : scene(scene_)
{
  clickButtons.resize(1);
  int joyClickBtn = scene->useCircleAsActionButton() ? HumanInput::JOY_XINPUT_REAL_BTN_B : HumanInput::JOY_XINPUT_REAL_BTN_A;
  clickButtons[0] = HotkeyButton(DEVID_JOYSTICK, joyClickBtn);
}


SQInteger SceneConfig::setClickButtons(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *>(vm))
    return SQ_ERROR;

  using namespace HumanInput;

  Sqrat::Var<SceneConfig *> self(vm, 1);

  SQInteger len = sq_getsize(vm, 2);
  if (SQ_FAILED(len))
    return SQ_ERROR;

  Tab<HotkeyButton> buttonIds;
  buttonIds.reserve(len);

  const Hotkeys &hotkeys = self.value->scene->getHotkeys();

  const char *jBtnA = hotkeys.getButtonName(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_A));
  G_ASSERT(jBtnA);
  String swappableActionButton(0, "~%s", jBtnA ? jBtnA : "???");

  const char *btnName = nullptr;
  for (SQInteger i = 0; i < len; ++i)
  {
    sq_pushinteger(vm, i);
    G_VERIFY(SQ_SUCCEEDED(sq_rawget(vm, -2)));
    if (SQ_FAILED(sq_getstring(vm, -1, &btnName)))
      return SQ_ERROR;

    if (strcmp(btnName, swappableActionButton) == 0)
    {
      int actionBtnId =
        self.value->scene->useCircleAsActionButton() ? HumanInput::JOY_XINPUT_REAL_BTN_B : HumanInput::JOY_XINPUT_REAL_BTN_A;
      buttonIds.push_back(HotkeyButton(DEVID_JOYSTICK, actionBtnId));
    }
    else
    {
      HotkeyButton hkBtn = hotkeys.resolveButton(btnName);
      if (hkBtn.devId == DEVID_NONE)
        return sq_throwerror(vm, String(0, "Unknown button '%s'", btnName));
      buttonIds.push_back(hkBtn);
    }

    sq_poptop(vm);
  }

  self.value->clickButtons.swap(buttonIds);
  return 0;
}


SQInteger SceneConfig::getClickButtons(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *>(vm))
    return SQ_ERROR;

  Sqrat::Var<SceneConfig *> self(vm, 1);
  const Hotkeys &hotkeys = self.value->scene->getHotkeys();

  sq_newarray(vm, self.value->clickButtons.size());
  for (int i = 0; i < self.value->clickButtons.size(); ++i)
  {
    sq_pushinteger(vm, i);
    const char *btnName = hotkeys.getButtonName(self.value->clickButtons[i]);
    sq_pushstring(vm, btnName ? btnName : "???", -1);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  return 1;
}


bool SceneConfig::isClickButton(InputDevice dev_id, int btn_id) const
{
  return find_value_idx(clickButtons, HotkeyButton(dev_id, btn_id)) >= 0;
}


SQInteger SceneConfig::setDefaultFontId(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *, SQInteger>(vm))
    return SQ_ERROR;
  SQInteger fontId = 0;
  sq_getinteger(vm, 2, &fontId);
  if (!StdGuiRender::get_font(fontId))
    return sqstd_throwerrorf(vm, "Invalid font #%d", int(fontId));

  Sqrat::Var<SceneConfig *> self(vm, 1);
  self.value->defaultFont = fontId;
  return 0;
}

SQInteger SceneConfig::getDefaultFontId(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *>(vm))
    return SQ_ERROR;

  Sqrat::Var<SceneConfig *> self(vm, 1);
  sq_pushinteger(vm, self.value->defaultFont);
  return 1;
}


SQInteger SceneConfig::setDefaultFontSize(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *, SQFloat>(vm))
    return SQ_ERROR;
  float fontSize = 0;
  sq_getfloat(vm, 2, &fontSize);


  Sqrat::Var<SceneConfig *> self(vm, 1);
  self.value->defaultFontSize = clamp(fontSize, 1.0f, floorf(StdGuiRender::screen_height() / 10.0f));
  return 0;
}


SQInteger SceneConfig::getDefaultFontSize(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SceneConfig *>(vm))
    return SQ_ERROR;

  Sqrat::Var<SceneConfig *> self(vm, 1);
  sq_pushfloat(vm, self.value->defaultFontSize);
  return 1;
}


} // namespace darg
