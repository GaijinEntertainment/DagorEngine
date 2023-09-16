// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiGlobals.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <string.h>

namespace HumanInput
{
class ScreenKeyboardDevice : public IGenKeyboard, public IWndProcComponent
{
public:
  ScreenKeyboardDevice()
  {
    client = NULL;
    memset(&state, 0, sizeof(state));
    memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));
    add_wnd_proc_component(this);
  }
  ~ScreenKeyboardDevice()
  {
    setClient(NULL);
    del_wnd_proc_component(this);
  }

  // IGenKeyboard interface implementation
  virtual const KeyboardRawState &getRawState() const { return state; }
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

  virtual int getKeyCount() const { return 0; }
  virtual const char *getKeyName(int idx) const { return "K?"; }

  virtual const char *getName() const { return "Keyboard@Screen-iOS"; }

  // IWndProcComponent interface implementation
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
  {
    if (msg == GPCM_Char && client)
      client->gkcButtonDown(this, 0, false, wParam);
    else if (msg == GPCM_KeyPress && client)
      client->gkcButtonDown(this, wParam, false, lParam);
    else if (msg == GPCM_KeyRelease && client)
      client->gkcButtonUp(this, wParam);
    return PROCEED_OTHER_COMPONENTS;
  }

protected:
  IGenKeyboardClient *client;
  KeyboardRawState state;
};
} // namespace HumanInput
