// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_KEYBOARD_KBD_DEVICE_COMMON_H
#define _GAIJIN_DRV_HID_KEYBOARD_KBD_DEVICE_COMMON_H
#pragma once

#include <humanInput/dag_hiKeyboard.h>

namespace HumanInput
{

class GenericKeyboardDevice : public IGenKeyboard
{
public:
  GenericKeyboardDevice();
  ~GenericKeyboardDevice();

  void onKeyDown(int dkey, wchar_t wc);
  void onKeyUp(int dkey);
  void onLayoutChanged();
  void onLocksChanged();

  // IGenKeyboard interface implementation
  virtual const KeyboardRawState &getRawState() const { return state; }
  virtual void setClient(IGenKeyboardClient *cli);
  virtual IGenKeyboardClient *getClient() const;

  virtual void resetKeys();
  virtual void syncRawState();

  virtual int getKeyCount() const;
  virtual const char *getKeyName(int idx) const;

protected:
  IGenKeyboardClient *client;
  KeyboardRawState state;
  const char *layout;
  unsigned locks;
};
} // namespace HumanInput

#endif
