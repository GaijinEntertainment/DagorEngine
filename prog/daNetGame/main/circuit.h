// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/string_view.h>

class DataBlock;

namespace circuit
{
const char *init_name_early(); // Reentrant. Returns circuit name if succeed
void init();

const DataBlock *get_conf();
eastl::string_view get_name(); // string_view is zero-terminated

bool is_submission_env();
bool is_staging_env();
} // namespace circuit
