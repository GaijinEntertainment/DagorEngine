// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace rendinst
{
struct RiExtraPool;
}

void ri_extra_max_height_on_ri_resource_added(rendinst::RiExtraPool &pool);

void ri_extra_max_height_on_ri_added_or_moved(rendinst::RiExtraPool &pool, float ri_height);

float ri_extra_max_height_get_max_height(int variableId);

void ri_extra_max_height_on_terminate();
