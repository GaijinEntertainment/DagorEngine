//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <daWeapons/guidance/guidance.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(GuidanceLockOutput::Result, GuidanceLockState);
DAS_BASE_BIND_ENUM_98(GuidanceLockOutput::Result, GuidanceLockState, RESULT_INVALID, RESULT_STANDBY, RESULT_POWER_ON,
  RESULT_WARMING_UP, RESULT_LOCKING, RESULT_TRACKING, RESULT_LOCK_AFTER_LAUNCH, RESULT_MAX)
