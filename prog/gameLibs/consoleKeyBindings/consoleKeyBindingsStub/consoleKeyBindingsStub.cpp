// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <consoleKeyBindings/consoleKeyBindings.h>
#include <util/dag_console.h>

namespace console_keybindings
{
bool bind(const char *, const char *) { return false; }
const char *get_command_by_key_code(int, unsigned) { return nullptr; }
void clear() {}

void set_binds_file_path(const char *) {}
void load_binds_from_file() {}
void save_binds_to_file() {}
} // namespace console_keybindings


static bool consoleKeybindings_console_handler(const char *argv[], int argc)
{
  int found = console::collector_cmp(argv[0], argc, "consoleKeybindings.bind", 3, 3, "", "<shortcut> \"<command>\"");
  if (found == 0)
    found = console::collector_cmp(argv[0], argc, "console.bind", 3, 3, "", "<shortcut> \"<command>\"");
  return found != 0;
}

REGISTER_CONSOLE_HANDLER(consoleKeybindings_console_handler);
