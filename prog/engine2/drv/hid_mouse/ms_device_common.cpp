#include "ms_device_common.h"
#include <humanInput/dag_hiGlobals.h>
#include <debug/dag_debug.h>
#include "api_wrappers.h"

using namespace HumanInput;

GenericMouseDevice::GenericMouseDevice() : hidden(true)
{
  client = NULL;
  memset(&state, 0, sizeof(state));
  accDz = 0;
  clip.l = 0;
  clip.t = 0;
  clip.r = 1024;
  clip.b = 1024;
  areButtonsSwapped = false;
}


GenericMouseDevice::~GenericMouseDevice() { setClient(NULL); }

bool GenericMouseDevice::clampStateCoord()
{
  bool clipped = false;

  if (state.mouse.x < clip.l)
  {
    state.mouse.x = clip.l;
    clipped = true;
  }
  if (state.mouse.y < clip.t)
  {
    state.mouse.y = clip.t;
    clipped = true;
  }
  if (state.mouse.x > clip.r)
  {
    state.mouse.x = clip.r;
    clipped = true;
  }
  if (state.mouse.y > clip.b)
  {
    state.mouse.y = clip.b;
    clipped = true;
  }

  return clipped;
}

void GenericMouseDevice::setClient(IGenPointingClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


int GenericMouseDevice::getBtnCount() const { return 7; }
const char *GenericMouseDevice::getBtnName(int idx) const
{
  if (idx < 0 || idx >= 7)
    return NULL;
  static const char *btn_names[7] = {"LMB", "RMB", "MMB", "M4B", "M5B", "MWUp", "MWDown"};
  if (areButtonsSwapped && idx <= 1)
  {
    idx = idx == 0 ? 1 : 0;
  }
  return btn_names[idx];
}

void GenericMouseDevice::setClipRect(int l, int t, int r, int b)
{
  clip.l = l;
  clip.t = t;
  clip.r = r;
  clip.b = b;
  // debug_ctx ( "set clip rect: (%d,%d)-(%d,%d)", l, t, r, b );
  if (clampStateCoord())
    setPosition(state.mouse.x, state.mouse.y);
}

void GenericMouseDevice::setMouseCapture(void *handle) { ::mouse_api_SetCapture(handle); }
void GenericMouseDevice::releaseMouseCapture() { ::mouse_api_ReleaseCapture(); }

void GenericMouseDevice::hideMouseCursor(bool hide)
{
  ::mouse_api_hide_cursor(hide);
  hidden = hide;
}
