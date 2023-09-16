#include "kbd_device_common.h"
#include "kbd_private.h"
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiGlobals.h>
#include <math/dag_math3d.h>
#include <debug/dag_debug.h>
using namespace HumanInput;


GenericKeyboardDevice::GenericKeyboardDevice()
{
  client = NULL;
  memset(&state, 0, sizeof(state));
  layout = "";
  locks = 0;
}


GenericKeyboardDevice::~GenericKeyboardDevice() { setClient(NULL); }

void GenericKeyboardDevice::onKeyDown(int dkey, wchar_t wc)
{
#if _TARGET_PC_WIN
#if DAGOR_DBGLEVEL > 0
  if (stg_kbd.magicKeysEnabled && client)
  {
    if (dkey == DKEY_END)
    {
      if ((state.isKeyDown(DKEY_RCONTROL) || state.isKeyDown(DKEY_LCONTROL)) &&
          (state.isKeyDown(DKEY_RALT) || state.isKeyDown(DKEY_LALT)))
      {
        client->gkcMagicCtrlAltEnd();
      }
    }
    else if (dkey == DKEY_F)
    {
      if (
        (state.isKeyDown(DKEY_RCONTROL) || state.isKeyDown(DKEY_LCONTROL)) &&
        ((state.isKeyDown(DKEY_RALT) || state.isKeyDown(DKEY_LALT)) || (state.isKeyDown(DKEY_RSHIFT) || state.isKeyDown(DKEY_LSHIFT))))
      {
        client->gkcMagicCtrlAltF();
      }
    }
  }
#endif

  if ((dkey == DKEY_TAB || dkey == DKEY_ESCAPE) && (state.isKeyDown(DKEY_LALT) || state.isKeyDown(DKEY_RALT)))
    return;
#endif

  unsigned shift = 1 << key_to_shift_bit[dkey];

  // key pressed
  bool repeat = state.isKeyDown(dkey);

  state.setKeyDown(dkey);
  state.shifts |= shift;
  raw_state_kbd.setKeyDown(dkey);
  raw_state_kbd.shifts |= shift;
  if (client)
    client->gkcButtonDown(this, dkey, repeat, wc);
}

void GenericKeyboardDevice::onKeyUp(int dkey)
{
  unsigned shift = 1 << key_to_shift_bit[dkey];

#if _TARGET_PC_WIN
  if (!state.isKeyDown(dkey) && dkey != DKEY_SYSRQ)
    return; // incorrect data?
#endif

  state.clrKeyDown(dkey);
  state.shifts &= ~shift;
  raw_state_kbd.clrKeyDown(dkey);
  raw_state_kbd.shifts &= ~shift;
  if (client)
    client->gkcButtonUp(this, dkey);
}

void GenericKeyboardDevice::onLayoutChanged()
{
  if (client)
    client->gkcLayoutChanged(this, layout);
}

void GenericKeyboardDevice::onLocksChanged()
{
  if (client)
    client->gkcLocksChanged(this, locks);
}

void GenericKeyboardDevice::setClient(IGenKeyboardClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


IGenKeyboardClient *GenericKeyboardDevice::getClient() const { return client; }


int GenericKeyboardDevice::getKeyCount() const { return DKEY__MAX_BUTTONS; }


const char *GenericKeyboardDevice::getKeyName(int idx) const { return key_name[idx & 0xFF]; }


void GenericKeyboardDevice::resetKeys()
{
  memset(&state.bitMask, 0, sizeof(state.bitMask));
  state.shifts = 0;
  memset(&raw_state_kbd.bitMask, 0, sizeof(raw_state_kbd.bitMask));
  raw_state_kbd.shifts = 0;
}


void GenericKeyboardDevice::syncRawState()
{
  state.shifts = 0;
  for (int dkey = 0; dkey < DKEY__MAX_BUTTONS; ++dkey)
  {
    if (!state.isKeyDown(dkey))
      continue;
    unsigned shift = 1 << key_to_shift_bit[dkey];
    state.shifts |= shift;
  }

  raw_state_kbd = state;
}
