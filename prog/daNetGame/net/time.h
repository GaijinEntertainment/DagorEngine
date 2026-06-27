// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/net/time.h>

int get_sync_millis();
int get_async_millis();

double advance_time(float dt, float &out_rt_dt);

void reset_time_mgr(ITimeManager *new_mgr = nullptr);

bool is_dummy_time();
