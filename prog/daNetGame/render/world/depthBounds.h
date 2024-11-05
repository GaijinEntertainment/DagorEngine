// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

bool pull_depth_bounds_enabled();
bool depth_bounds_enabled();
float far_plane_depth(int texfmt);
void api_set_depth_bounds(float zmin, float zmax);
