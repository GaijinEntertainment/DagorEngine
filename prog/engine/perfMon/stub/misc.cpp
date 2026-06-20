// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "perfMon/dag_graphStat.h"
#include "perfMon/dag_sleepPrecise.h"
#include <dxgidebug.h>


const GUID DXGI_DEBUG_ALL{};
ID3dDrawStatistic Stat3D::g_draw_stat;
void sleep_precise_usec(int, PreciseSleepContext &) {}
