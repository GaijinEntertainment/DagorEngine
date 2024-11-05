// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

typedef uint32_t exe_version32_t; // 8 bit per each version number

const char *get_exe_version_str();
exe_version32_t get_exe_version32();
int get_build_number(); // returns -1 in case of self-built executable
