// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

typedef struct SQVM *HSQUIRRELVM;

bool parse_json_directly_to_quirrel(HSQUIRRELVM vm, const char *start, const char *end, eastl::string &error_string);
bool parse_json_zstd_stream_directly_to_quirrel(HSQUIRRELVM vm, void *zstd_stream_data, size_t zstd_stream_size,
  eastl::string &error_string);
eastl::string directly_convert_quirrel_val_to_json_string(HSQUIRRELVM vm, int stack_pos, bool pretty);
void directly_convert_quirrel_val_to_zstd_json(HSQUIRRELVM vm, int stack_pos,
  void (*write_func)(const void *data, size_t size, void *user_data), void *user_data);
