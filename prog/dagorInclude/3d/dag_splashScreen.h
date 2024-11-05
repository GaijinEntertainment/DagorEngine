//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BaseTexture;
typedef BaseTexture Texture;


void show_splash_screen(Texture *tex, bool independent_render = true);
void release_splash_screen_resources();
