// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <osApiWrappers/dag_miscApi.h>
#include "emu_hooks.h"
#include <string.h>

using namespace HumanInput;

class KeyboardEmuDrv final : public IGenKeyboardClassDrv, public IGenKeyboard
{
public:
  KeyboardEmuDrv()
  {
    enabled = false;
    client = NULL;
  }
  ~KeyboardEmuDrv() { destroy(); }

  // IGenKeyboardClassDrv interface
  virtual void enable(bool en)
  {
    enabled = en;
    stg_kbd.enabled = en;
  }
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy()
  {
    enable(false);
    setClient(NULL);
  }
  virtual void refreshDeviceList() {}
  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices()
  {
    if (!is_main_thread()) // skip calls from gamepad updateDevice() in polling thread
      return;
    unsigned bw0 = raw_state_joy.buttons.getWord0(), bpw0 = lastButtons0;
    lastButtons0 = bw0;
    if ((bw0 & 0x102F) == (bpw0 & 0x102F))
      return;

    // UP = Dpad Up
    if ((bw0 & 0x1) && !(bpw0 & 0x1))
      onKeyDown(DKEY_UP);
    else if (!(bw0 & 0x1) && (bpw0 & 0x1))
      onKeyUp(DKEY_UP);

    // DOWN = Dpad Down
    if ((bw0 & 0x2) && !(bpw0 & 0x2))
      onKeyDown(DKEY_DOWN);
    else if (!(bw0 & 0x2) && (bpw0 & 0x2))
      onKeyUp(DKEY_DOWN);

    // LEFT = Dpad Left
    if ((bw0 & 0x4) && !(bpw0 & 0x4))
      onKeyDown(DKEY_LEFT);
    else if (!(bw0 & 0x4) && (bpw0 & 0x4))
      onKeyUp(DKEY_LEFT);

    // RIGHT = Dpad Right
    if ((bw0 & 0x8) && !(bpw0 & 0x8))
      onKeyDown(DKEY_RIGHT);
    else if (!(bw0 & 0x8) && (bpw0 & 0x8))
      onKeyUp(DKEY_RIGHT);

    // ENTER = button A
    if ((bw0 & 0x1000) && !(bpw0 & 0x1000))
      onKeyDown(DKEY_RETURN);
    else if (!(bw0 & 0x1000) && (bpw0 & 0x1000))
      onKeyUp(DKEY_RETURN);

    // ESC = button Exit
    if ((bw0 & 0x20) && !(bpw0 & 0x20))
      onKeyDown(DKEY_ESCAPE);
    else if (!(bw0 & 0x20) && (bpw0 & 0x20))
      onKeyUp(DKEY_ESCAPE);
  }

  virtual IGenKeyboard *getDevice(int idx) const { return (KeyboardEmuDrv *)this; }
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual void useDefClient(IGenKeyboardClient *cli) { setClient(cli); }

  // IGenKeyaboard interface
  virtual const char *getName() const { return "Kbd @ Gamepad360"; }
  virtual const KeyboardRawState &getRawState() const { return raw_state_kbd; }
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
    switch (idx)
    {
      case DKEY_ESCAPE: return "Esc";
      case DKEY_RETURN: return "Enter";
      case DKEY_UP: return "Up";
      case DKEY_LEFT: return "Left";
      case DKEY_RIGHT: return "Right";
      case DKEY_DOWN: return "Down";
    }
    return NULL;
  }

private:
  IGenKeyboardClient *client;
  bool enabled;
  uint32_t lastButtons0 = 0;

  void onKeyDown(int dkey)
  {
    bool repeat = raw_state_kbd.isKeyDown(dkey);

    raw_state_kbd.setKeyDown(dkey);
    if (client)
      client->gkcButtonDown(this, dkey, repeat, 0);
  }

  void onKeyUp(int dkey)
  {
    if (!raw_state_kbd.isKeyDown(dkey))
      return; // incorrect data?

    raw_state_kbd.clrKeyDown(dkey);
    if (client)
      client->gkcButtonUp(this, dkey);
  }
};

static KeyboardEmuDrv drv;

IGenKeyboardClassDrv *HumanInput::createKeyboardEmuClassDriver()
{
  memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));
  drv.enable(true);
  humaninputxbox360::kbd_emu = &drv;
  return &drv;
}
