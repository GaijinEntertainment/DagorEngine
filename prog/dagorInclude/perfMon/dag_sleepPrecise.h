//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// Same as sleep_msec() but more accurate (with tradeoff of using more CPU).
// - This API is assumed to be used for short to mediums sleep times (not longer then 10s of milleceonds).
// - Currently not thread-safe.
void sleep_precise_usec(int time_usec);
