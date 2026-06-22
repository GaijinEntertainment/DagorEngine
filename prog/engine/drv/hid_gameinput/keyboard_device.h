// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiKeybData.h>
#include <osApiWrappers/dag_wndProcComponent.h>

namespace HumanInput
{
void init_gameinput_keyboard_tables();


class GameInputKeyboardDevice final : public IGenKeyboard, public IWndProcComponent
{
public:
  GameInputKeyboardDevice();
  ~GameInputKeyboardDevice();

  void update();

  // IGenKeyboard interface implementation
  const char *getName() const override { return "Keyboard@GameInput"; }
  const KeyboardRawState &getRawState() const override { return state; }
  void setClient(IGenKeyboardClient *cli) override;
  IGenKeyboardClient *getClient() const override { return client; }
  void resetKeys() override;
  void syncRawState() override;
  int getKeyCount() const override;
  const char *getKeyName(int idx) const override;

  // IWndProcComponent interface implementation (character input)
  RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override;

private:
  void onKeyDown(int dkey, wchar_t wc);
  void onKeyUp(int dkey);

  IGenKeyboardClient *client = nullptr;
  KeyboardRawState state = {};
};
} // namespace HumanInput
