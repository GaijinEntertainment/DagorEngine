// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

#ifndef _MAX_DIR
#ifdef PATH_MAX
#define _MAX_DIR PATH_MAX
#else
#define _MAX_DIR 4096
#endif
#endif

bool change_dir(const char *dir);
eastl::string resolve_absolute_file_name(const char *file_name);
