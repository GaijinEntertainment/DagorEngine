//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace PhysCounters
{
extern int rayCastObjCreated;
extern int rayCastUpdate;
extern int memoryUsed;
extern int memoryUsedMax;

void resetCounters();
} // namespace PhysCounters
