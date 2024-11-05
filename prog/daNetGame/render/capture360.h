// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>

class TMatrix;

namespace capture360
{
bool is_360_capturing_in_progress();
void update_capture(ManagedTexView tex);
} // namespace capture360
