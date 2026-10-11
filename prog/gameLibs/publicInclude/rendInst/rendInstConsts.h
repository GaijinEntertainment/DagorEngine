//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace rendinst
{
#if _TARGET_PC | _TARGET_C2 | _TARGET_SCARLETT // PC also covers all exporters
static constexpr int RI_MAX_LODS = 8;
#else
// assets are exported with up to 8 lods, extra ones are dropped on load (see RenderableInstanceLodsResource::LOAD_MAX_LODS)
static constexpr int RI_MAX_LODS = 4;
#endif
static constexpr int RI_SEED_COLLISION_IGNORE = 0xBAD;
} // namespace rendinst
