//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>
#include "daNetTypes.h"

struct DaNetStatistics
{
  DaNetTime connectionStartTime;
  uintptr_t bytesSent;
  uintptr_t bytesReceived;
  uintptr_t bytesDropped;
  uintptr_t packetsSent;
  uintptr_t packetsReceived;
};

inline void StatisticsToString(DaNetStatistics *, char *, int) {}
