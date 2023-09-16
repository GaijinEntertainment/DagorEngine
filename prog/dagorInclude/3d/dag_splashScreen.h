//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class BaseTexture;
typedef BaseTexture Texture;


void show_splash_screen(Texture *tex, bool independent_render = true);
void release_splash_screen_resources();
