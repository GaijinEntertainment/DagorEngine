// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <util/dag_globDef.h>


void set_script_param(String &script, const char *param, const char *val);
void set_script_param_int(String &script, const char *param, int val);
void set_script_param_real(String &script, const char *param, real val);

void erase_script_param(String &script, const char *param);

String get_script_param(const String &script, const char *param);
