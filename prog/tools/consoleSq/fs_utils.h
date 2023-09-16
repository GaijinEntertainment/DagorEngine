#pragma once
#include <EASTL/string.h>

#ifndef _MAX_DIR
#ifdef PATH_MAX
#define _MAX_DIR PATH_MAX
#else
#define _MAX_DIR 4096
#endif
#endif

bool is_file_case_valid(const char *file_name);
bool change_dir(const char *dir);
eastl::string resolve_absolute_file_name(const char *file_name);
