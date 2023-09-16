#pragma once

#if defined(__MACOSX__) || (defined(__APPLE_IOS__) && !defined(__arm__))
  #include "config_macosx.h"
#elif defined(__APPLE_IOS__) && defined(__arm__)
  #include "config_macosx.h"
#elif defined(_TARGET_TVOS)
  #include "config_macosx.h"
#elif defined(ANDROID)
  #include "config_android.h"
#elif defined(__LINUX__) || defined(__linux__)
  #include "config_linux.h"
#elif defined(_TARGET_C3)

#elif defined(WIN32) || defined(_WIN64)
  #include "config_win32.h"
#elif defined(_TARGET_C1) || defined(_TARGET_C2)

#else
  #error "unsupported platform"
#endif

