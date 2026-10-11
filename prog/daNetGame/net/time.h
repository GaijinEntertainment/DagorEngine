// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/net/time.h>

double advance_time(float dt, float &out_rt_dt);

void reset_time_mgr(ITimeManager *new_mgr = nullptr);

bool is_default_time();
