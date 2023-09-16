//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <forceFeedback/forceFeedback.h>

typedef force_feedback::rumble::EventParams RumbleEventParams;

MAKE_TYPE_FACTORY(RumbleEventParams, RumbleEventParams);

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(force_feedback::rumble::EventParams::Band, RumbleBand);
DAS_BASE_BIND_ENUM_98(force_feedback::rumble::EventParams::Band, RumbleBand, LO, HI)
