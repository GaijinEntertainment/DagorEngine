// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "inputControls.h"
#include "uiInput.h"
#include <daInput/input_api.h>
#include <util/dag_console.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>

namespace
{
struct ActionPress
{
  dainput::action_handle_t action;
  int holdMsec;
  int t0Msec;
};
} // namespace

static eastl::vector<ActionPress> queued_presses;
static eastl::vector<ActionPress> held_presses;

void controls::send_console_actions()
{
  const int now = get_time_msec();
  for (auto it = held_presses.begin(); it != held_presses.end();)
    if (now - it->t0Msec >= it->holdMsec)
    {
      dainput::send_action_terminated_event(it->action, eastl::min(now - it->t0Msec, 0x7FFF));
      it = held_presses.erase(it);
    }
    else
      ++it;

  eastl::vector<ActionPress> fresh;
  fresh.swap(queued_presses);
  for (ActionPress &press : fresh)
  {
    dainput::send_action_event(press.action);
    press.t0Msec = now;
    held_presses.push_back(press);
  }
}

void controls::clear_console_actions()
{
  const int now = get_time_msec();
  for (const ActionPress &press : held_presses)
    dainput::send_action_terminated_event(press.action, eastl::min(now - press.t0Msec, 0x7FFF));
  held_presses.clear();
  queued_presses.clear();
}

using namespace console;
static bool dainput_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME_EX("dainput", "activate_action", 2, 3,
    "sends the triggered event of a digital action, and its terminated event hold_msec later", "<action_name> [hold_msec]")
  {
    uiinput::call_dainput([name = eastl::string(argv[1]), holdMsec = argc > 2 ? to_int(argv[2]) : 0] {
      dainput::action_handle_t action = dainput::get_action_handle(name.c_str());
      if (action == dainput::BAD_ACTION_HANDLE)
        console::print_d("dainput: unknown action <%s>", name.c_str());
      else if ((action & dainput::TYPEGRP__MASK) != dainput::TYPEGRP_DIGITAL)
        console::print_d("dainput: <%s> is not a digital action", name.c_str());
      else
        queued_presses.push_back(ActionPress{action, holdMsec, 0});
    });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dainput_console_handler);
