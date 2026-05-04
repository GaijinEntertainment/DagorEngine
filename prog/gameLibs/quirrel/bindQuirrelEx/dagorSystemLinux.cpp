// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <osApiWrappers/dag_linuxGUI.h>

SQInteger linux_get_primary_screen_info(HSQUIRRELVM vm)
{
  if (!linux_GUI::init())
    return -1;
  int width = 0;
  int height = 0;
  linux_GUI::get_display_size(width, height, true /*for_primary_output*/);
  Sqrat::Table res(vm);
  res.SetValue("pixelsWidth", width);
  res.SetValue("pixelsHeight", height);
  sq_pushobject(vm, res);
  return 1;
}