//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace levelprofiler
{

void init();
void teardown();
// Selects the texture in the Texture pool tab and shows its mip 0. An empty name clears the selection
// until the next draw, which selects the first filtered texture. No effect before init(), with a null
// name, or in a rel build (the stub).
void select_texture(const char *name);

} // namespace levelprofiler