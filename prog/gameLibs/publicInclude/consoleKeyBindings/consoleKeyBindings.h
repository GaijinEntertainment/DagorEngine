//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>

namespace console_keybindings
{
struct ConsoleKeybinding
{
  int keyCode;
  unsigned modifiers;
  SimpleString command;
  SimpleString keyCombo; // technically can be calcualted from keyCode and modifiers but it's easier to save on bind
};

bool bind(const char *key_comb, const char *command);
const char *get_command_by_key_code(int key_code, unsigned modifiers);
void clear();

void set_binds_file_path(const String &path);
void load_binds_from_file();
void save_binds_to_file();
} // namespace console_keybindings
