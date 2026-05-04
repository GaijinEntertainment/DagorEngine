// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepClient/sepClientStats.h>

namespace sepclientstats
{
sepclient::SepClientStatsCallbackPtr spawn_callback(eastl::string_view default_project_id);
}
