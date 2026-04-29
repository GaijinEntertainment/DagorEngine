// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN // TODO: tools Linux porting: SplashScreen

#include <windows.h>

class SplashScreen
{
public:
  SplashScreen();
  ~SplashScreen();

  static void kill();

private:
  static void *hwnd;
  static void *bitmap;

  static int bmpW;
  static int bmpH;

  static unsigned registerSplashClass();
  static LRESULT CALLBACK wndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);
};

#else

class SplashScreen
{
public:
  SplashScreen() {}
  ~SplashScreen() {}

  static void kill() {}
};

#endif