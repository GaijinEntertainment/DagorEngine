// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include "consolePrivate.h"

using namespace console_private;

namespace console
{
// set console driver
void set_visual_driver(IVisualConsoleDriver *driver, const char *script_filename)
{
  if (conDriver)
    conDriver->shutdown();

  conDriver = driver;

  if (conDriver)
  {
    conDriver->init(script_filename);
    console::get_visual_driver()->set_cur_height(conHeight);
  }
}

void set_visual_driver_raw(IVisualConsoleDriver *driver) { conDriver = driver; }

// get console driver
IVisualConsoleDriver *get_visual_driver() { return conDriver; }
} // namespace console
