//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

typedef void *TSgWParam;
typedef void *TSgLParam;


enum
{
  CLIENT_WINDOW_TYPE_NONE = -1,
};


enum HeaderPos
{
  HEADER_NONE,
  HEADER_TOP,
  HEADER_LEFT,
};


enum WindowSizeLock
{
  WSL_NONE = 0, // 0b000,
  WSL_WIDTH,    //= 0b001,
  WSL_HEIGHT,   //= 0b010,
  WSL_BOTH,     //= 0b011,
};


enum WindowAlign
{
  WA_LEFT = 2,   // 0b010,
  WA_TOP = 0,    // 0b000,
  WA_RIGHT = 3,  // 0b011,
  WA_BOTTOM = 1, // 0b001,

  WA_IS_VERTICAL = 6, // 0b110,
  WA_DEFAULT_POS = 5, // 0b101,

  WA_NONE = 7, // 0b111,
};

enum WindowSizeInit
{
  WSI_NORMAL,
  WSI_MINIMIZED,
  WSI_MAXIMIZED,
};
