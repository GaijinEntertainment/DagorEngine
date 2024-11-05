// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/input/input.h>
#include <startup/dag_inpDevClsDrv.h>
#include "esHidEventRouter.h"

Tab<ecs::IGenHidEventHandler *> ESHidEventRouter::ehStack(inimem);
Tab<unsigned> ESHidEventRouter::ehStackPrio(inimem); // largest-to-smallest sorted array (parallel to ehStack)
ESHidEventRouter ESHidEventRouter::hidEvRouter;
HumanInput::IGenKeyboardClient *ESHidEventRouter::secKbd = nullptr;
HumanInput::IGenPointingClient *ESHidEventRouter::secPnt = nullptr;
HumanInput::IGenJoystickClient *ESHidEventRouter::secJoy = nullptr;

static bool hid_event_handlers_inited = false;
static void hid_event_handlers_init()
{
  hid_event_handlers_inited = true;
  if (global_cls_drv_kbd)
    global_cls_drv_kbd->useDefClient(&ESHidEventRouter::hidEvRouter);
  if (global_cls_drv_pnt)
    global_cls_drv_pnt->useDefClient(&ESHidEventRouter::hidEvRouter);
  if (global_cls_drv_joy)
    global_cls_drv_joy->useDefClient(&ESHidEventRouter::hidEvRouter);
}
void ESHidEventRouter::hid_event_handlers_clear()
{
  if (global_cls_drv_kbd)
    global_cls_drv_kbd->useDefClient(NULL);
  if (global_cls_drv_pnt)
    global_cls_drv_pnt->useDefClient(NULL);
  if (global_cls_drv_joy)
    global_cls_drv_joy->useDefClient(NULL);
  clear_and_shrink(ehStack);
  clear_and_shrink(ehStackPrio);
  hid_event_handlers_inited = false;
}

void register_hid_event_handler(ecs::IGenHidEventHandler *eh, unsigned priority)
{
  if (!hid_event_handlers_inited) // init on demand
    hid_event_handlers_init();

  static uint16_t ord = 0;
  int idx = find_value_idx(ESHidEventRouter::ehStack, eh);
  if (idx >= 0)
  {
    // remove handler from stack if already exists
    erase_items(ESHidEventRouter::ehStack, idx, 1);
    erase_items(ESHidEventRouter::ehStackPrio, idx, 1);
  }

  // modulate priority with ordinal count (last set, higher priority)
  priority = (priority << 16) + ord;
  ord++;

  for (int i = 0; i < ESHidEventRouter::ehStackPrio.size(); i++)
    if (priority > ESHidEventRouter::ehStackPrio[i])
    {
      // insert before first handler with lower priority
      insert_item_at(ESHidEventRouter::ehStack, i, eh);
      insert_item_at(ESHidEventRouter::ehStackPrio, i, priority);
      return;
    }
  // append handler (lowest priority)
  ESHidEventRouter::ehStack.push_back(eh);
  ESHidEventRouter::ehStackPrio.push_back(priority);
}
void unregister_hid_event_handler(ecs::IGenHidEventHandler *eh)
{
  for (int i = 0; i < ESHidEventRouter::ehStack.size(); i++)
    if (ESHidEventRouter::ehStack[i] == eh)
    {
      erase_items(ESHidEventRouter::ehStack, i, 1);
      erase_items(ESHidEventRouter::ehStackPrio, i, 1);
      break;
    }
}

void set_secondary_hid_client_kbd(HumanInput::IGenKeyboardClient *cli)
{
  if (!hid_event_handlers_inited) // init on demand
    hid_event_handlers_init();
  ESHidEventRouter::secKbd = cli;
}
void set_secondary_hid_client_pnt(HumanInput::IGenPointingClient *cli)
{
  if (!hid_event_handlers_inited) // init on demand
    hid_event_handlers_init();
  ESHidEventRouter::secPnt = cli;
}
void set_secondary_hid_client_joy(HumanInput::IGenJoystickClient *cli)
{
  if (!hid_event_handlers_inited) // init on demand
    hid_event_handlers_init();
  ESHidEventRouter::secJoy = cli;
}
