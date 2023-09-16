//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace gamephys
{
static constexpr const float DEFAULT_PHYSICS_UPDATE_FIXED_DT = 1.f / 48.f;

#if DAGOR_DBGLEVEL > 0
extern float PHYSICS_UPDATE_FIXED_DT;
#else
static constexpr const float PHYSICS_UPDATE_FIXED_DT = DEFAULT_PHYSICS_UPDATE_FIXED_DT;
#endif
} // namespace gamephys

using gamephys::PHYSICS_UPDATE_FIXED_DT;
