// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/wind/fluidWind.h>

class Point3;

FluidWind *get_fluid_wind();
void init_fluid_wind();
void close_fluid_wind();
void update_fluid_wind(float dt, const Point3 &origin);