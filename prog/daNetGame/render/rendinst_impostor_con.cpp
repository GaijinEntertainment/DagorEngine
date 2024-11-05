// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <rendInst/rendInstGen.h>
#include <shaders/dag_rendInstRes.h>


static bool rendinst_impostor_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("rendinst_impostor", "use_parallax", 1, 2)
  {
    if (argc >= 2)
      rendinst_impostor_set_parallax_mode(console::to_int(argv[1]) == 1);
    else
      rendinst_impostor_set_parallax_mode(!rendinst_impostor_is_parallax_allowed());
    console::print_d("impostor_parallax_mode set to %s", rendinst_impostor_is_parallax_allowed() ? "on" : "off");
  }
  CONSOLE_CHECK_NAME("rendinst_impostor", "tri_view", 1, 2)
  {
    if (argc >= 2)
      rendinst_impostor_set_view_mode(console::to_int(argv[1]) == 1);
    else
      rendinst_impostor_set_view_mode(!rendinst_impostor_is_tri_view_allowed());
    console::print_d("impostor_parallax_mode set to %s", rendinst_impostor_is_tri_view_allowed() ? "on" : "off");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(rendinst_impostor_console_handler);
