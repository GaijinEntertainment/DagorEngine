// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
using namespace console;

PULL_CONSOLE_PROC(def_app_console_handler)
void console_init() { console::init(); }
void console_close() { console::shutdown(); }