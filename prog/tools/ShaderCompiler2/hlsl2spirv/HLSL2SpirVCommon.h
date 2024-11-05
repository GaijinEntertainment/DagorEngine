// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <string>

bool fix_vertex_id_for_DXC(std::string &src, CompileResult &output);

eastl::vector<eastl::string_view> scanDisabledSpirvOptimizations(const char *source);