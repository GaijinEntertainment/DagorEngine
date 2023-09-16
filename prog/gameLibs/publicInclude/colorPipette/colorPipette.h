//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace colorpipette
{
typedef void (*ColorPipetteCallback)(void *user_data, bool picked, int r, int g, int b);
bool is_available_on_this_platform();
bool start(ColorPipetteCallback callback, void *user_data);
bool is_active();
void terminate();
} // namespace colorpipette
