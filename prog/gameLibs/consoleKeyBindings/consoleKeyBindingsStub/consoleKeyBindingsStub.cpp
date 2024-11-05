// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <consoleKeyBindings/consoleKeyBindings.h>

namespace console_keybindings
{
bool bind(const char *, const char *) { return false; }
const char *get_command_by_key_code(int, unsigned) { return nullptr; }
void clear() {}

void set_binds_file_path(const String &) {}
void load_binds_from_file() {}
void save_binds_to_file() {}
} // namespace console_keybindings
