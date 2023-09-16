//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

//! show splash screen before on app start, before d3d and EasyAntiCheat init
void win_show_splash_screen(const wchar_t *splash_image_path);

//! hides splash screen
void win_hide_splash_screen();
