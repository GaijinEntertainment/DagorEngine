//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_relocatableFixedVector.h>

namespace dynamic_shadow_render
{

static constexpr int ESTIMATED_MAX_VOLUMES_TO_UPDATE_PER_FRAME = 64;
using VolumesVector = dag::RelocatableFixedVector<uint16_t, ESTIMATED_MAX_VOLUMES_TO_UPDATE_PER_FRAME, true>;

} // namespace dynamic_shadow_render