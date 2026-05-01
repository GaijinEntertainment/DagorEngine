// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "smooth_noise.h"

namespace smooth_noise
{
// PCG2D-based: no LUT to populate, but keep init/close as no-ops so existing
// callers that pair them with their lifetime do not need touching.
void init() {}
void close() {}

} // namespace smooth_noise
