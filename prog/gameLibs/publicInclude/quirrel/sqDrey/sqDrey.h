//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>

namespace sqdrey
{
void init(const char *path_to_static_analyzer_, const char *drey_args_, const char *path_to_sqconfig_);
void before_reload_scripts();
void after_reload_scripts(bool output_to_logerr);
String get_drey_result();
int get_drey_call_count();
} // namespace sqdrey
