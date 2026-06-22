// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "keyboard_device.h"
#include "keyboard_names.h"
#include <supp/_platform.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiGlobals.h>
#include <osApiWrappers/gdk/gameinput.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/fixed_vector.h>
#include <debug/dag_debug.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace HumanInput;


static unsigned char key_to_shift_bit[256];
static char generated_key_name[256][5];
static const char *key_name[256];


void HumanInput::init_gameinput_keyboard_tables()
{
  memset(key_to_shift_bit, 0x1F, sizeof(key_to_shift_bit));
  key_to_shift_bit[DKEY_LSHIFT] = 0;
  key_to_shift_bit[DKEY_RSHIFT] = 1;
  key_to_shift_bit[DKEY_LCONTROL] = 2;
  key_to_shift_bit[DKEY_RCONTROL] = 3;
  key_to_shift_bit[DKEY_LALT] = 4;
  key_to_shift_bit[DKEY_RALT] = 5;

  for (int i = 0; i < 256; ++i)
  {
    snprintf(generated_key_name[i], sizeof(generated_key_name[i]), "K%03d", i);
    key_name[i] = generated_key_name[i];
  }
  for (const GameInputKeyName &kn : gameinput_key_names)
    key_name[kn.hid_id & 0xFF] = kn.name;
}


static constexpr uint32_t get_scancode(uint32_t scan_code)
{
  switch (scan_code)
  {
    case 0xE036: return DKEY_RSHIFT;
    case 0xE045: return DKEY_NUMLOCK;
    case 0xE11D: return DKEY_PAUSE;
    case 0x21D: return DKEY_RALT;
  }
  if ((scan_code & 0xFF00) == 0xE000)
    return 0x80 | (scan_code & 0xFF);
  return scan_code;
}


GameInputKeyboardDevice::GameInputKeyboardDevice() { add_wnd_proc_component(this); }


GameInputKeyboardDevice::~GameInputKeyboardDevice()
{
  setClient(nullptr);
  del_wnd_proc_component(this);
}


void GameInputKeyboardDevice::setClient(IGenKeyboardClient *cli)
{
  if (cli == client)
    return;
  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


int GameInputKeyboardDevice::getKeyCount() const { return DKEY__MAX_BUTTONS; }


const char *GameInputKeyboardDevice::getKeyName(int idx) const { return key_name[idx & 0xFF]; }


void GameInputKeyboardDevice::resetKeys()
{
  memset(&state.bitMask, 0, sizeof(state.bitMask));
  state.shifts = 0;
  memset(&raw_state_kbd.bitMask, 0, sizeof(raw_state_kbd.bitMask));
  raw_state_kbd.shifts = 0;
}


void GameInputKeyboardDevice::syncRawState()
{
  state.shifts = 0;
  for (int dkey = 0; dkey < DKEY__MAX_BUTTONS; ++dkey)
    if (state.isKeyDown(dkey))
      state.shifts |= 1 << key_to_shift_bit[dkey];

  raw_state_kbd = state;
}


void GameInputKeyboardDevice::onKeyDown(int dkey, wchar_t wc)
{
  unsigned shift = 1 << key_to_shift_bit[dkey];
  bool repeat = state.isKeyDown(dkey);

  state.setKeyDown(dkey);
  state.shifts |= shift;
  raw_state_kbd.setKeyDown(dkey);
  raw_state_kbd.shifts |= shift;
  if (client)
    client->gkcButtonDown(this, dkey, repeat, wc);
}


void GameInputKeyboardDevice::onKeyUp(int dkey)
{
  unsigned shift = 1 << key_to_shift_bit[dkey];

  state.clrKeyDown(dkey);
  state.shifts &= ~shift;
  raw_state_kbd.clrKeyDown(dkey);
  raw_state_kbd.shifts &= ~shift;
  if (client)
    client->gkcButtonUp(this, dkey);
}


IWndProcComponent::RetCode GameInputKeyboardDevice::process(void *, unsigned msg, uintptr_t wParam, intptr_t, intptr_t &result)
{
  G_UNREFERENCED(result);
  if (msg == WM_CHAR && client && wParam >= 0x20)
    client->gkcButtonDown(this, 0, false, wParam);
  return PROCEED_OTHER_COMPONENTS;
}


void GameInputKeyboardDevice::update()
{
  TIME_PROFILE(HID_GINP_updateKeyboard);

  gdk::gameinput::DevicesList devices;
  gdk::gameinput::get_devices(GameInputKindKeyboard, devices);

  KeyboardRawState newState = {};

  for (IGameInputDevice *dev : devices)
  {
    if (!dev)
      continue;

    gdk::gameinput::Reading reading = gdk::gameinput::get_current_reading(GameInputKindKeyboard, dev);
    if (!reading)
      continue;

    uint32_t keysCount = reading->GetKeyCount();
    if (!keysCount)
      continue;

    eastl::fixed_vector<GameInputKeyState, 16, true> keys(keysCount);
    keysCount = reading->GetKeyState(keysCount, keys.data());

    for (uint32_t i = 0; i < keysCount; ++i)
    {
      uint32_t code = get_scancode(keys[i].scanCode);
      if (code < DKEY_ESCAPE || code > DKEY_MEDIASELECT)
      {
        logwarn("[%s] UNKNOWN SCANCODE! 0x%X", __FUNCTION__, code);
        continue;
      }
      newState.setKeyDown(code);
    }
  }

  for (int dkey = 0; dkey < DKEY__MAX_BUTTONS; ++dkey)
    if (state.isKeyDown(dkey) && !newState.isKeyDown(dkey))
      onKeyUp(dkey);

  for (int dkey = 0; dkey < DKEY__MAX_BUTTONS; ++dkey)
    if (newState.isKeyDown(dkey) && !state.isKeyDown(dkey))
      onKeyDown(dkey, 0);
}
