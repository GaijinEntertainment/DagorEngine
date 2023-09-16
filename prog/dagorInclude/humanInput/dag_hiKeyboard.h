//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>
#include <humanInput/dag_hiHidClassDrv.h>
#include <humanInput/dag_hiKeybData.h>

namespace HumanInput
{

// generic keyboard interface
class IGenKeyboard
{
public:
  // general routines
  virtual const char *getName() const = 0;
  virtual const KeyboardRawState &getRawState() const = 0;
  virtual void setClient(IGenKeyboardClient *cli) = 0;
  virtual IGenKeyboardClient *getClient() const = 0;
  virtual void enableIme(bool on) { (void)on; }
  virtual bool isImeActive() const { return false; }
  virtual void enableLayoutChangeTracking(bool en) { (void)en; }
  virtual void enableLocksChangeTracking(bool en) { (void)en; }

  virtual void resetKeys() {}
  virtual void syncRawState() {}

  // keys description
  virtual int getKeyCount() const = 0;
  virtual const char *getKeyName(int idx) const = 0;
};


// simple keyboard client interface
class IGenKeyboardClient
{
public:
  virtual void attached(IGenKeyboard *kbd) = 0;
  virtual void detached(IGenKeyboard *kbd) = 0;

  virtual void gkcButtonDown(IGenKeyboard *kbd, int btn_idx, bool repeat, wchar_t wchar) = 0;
  virtual void gkcButtonUp(IGenKeyboard *kbd, int btn_idx) = 0;
  virtual void gkcLayoutChanged(IGenKeyboard *kbd, const char *layoutLanguage)
  {
    (void)kbd;
    (void)layoutLanguage;
  }
  virtual void gkcLocksChanged(IGenKeyboard *kbd, unsigned locks)
  {
    (void)kbd;
    (void)locks;
  }
  virtual void gkcMagicCtrlAltEnd() = 0;
  virtual void gkcMagicCtrlAltF() = 0;
};


// generic keyboard class driver interface
class IGenKeyboardClassDrv : public IGenHidClassDrv
{
public:
  virtual IGenKeyboard *getDevice(int idx) const = 0;
  virtual bool isDeviceConfigChanged() const = 0;
  virtual void useDefClient(IGenKeyboardClient *cli) = 0;
};
} // namespace HumanInput
