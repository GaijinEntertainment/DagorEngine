//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
namespace fxwindhelper
{
void load_fx_wind_curve_params(const DataBlock *blk);
float apply_wind_strength_curve(float beaufort);
} // namespace fxwindhelper