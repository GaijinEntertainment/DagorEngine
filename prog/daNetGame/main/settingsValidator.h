// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

class DataBlock;

namespace settings_validator
{
void ensure_no_duplicates(const DataBlock *block, const eastl::string &prefix, bool recursive);
}