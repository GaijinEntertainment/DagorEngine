// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <scene/dag_physMat.h>
#include <EASTL/string.h>

class DataBlock;

namespace physmat
{
void init();
eastl::string get_config_filename();
}; // namespace physmat
