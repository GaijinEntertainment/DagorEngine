// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;

void load_render_libs_allowed_blk(const char *lib_excluded_blk_fn);
void set_render_libs_allowed_blk(const DataBlock &lib_excluded_blk);
bool is_render_lib_allowed(const char *lib_name);
