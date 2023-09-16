#ifndef __LIBZIP_CONFIG_H__
#define __LIBZIP_CONFIG_H__

#if _WIN32
#include "config.win32.h"
#elif __APPLE__
#include "config.macosx.h"
#elif __linux__
#include "config.linux.h"
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_C3

#else
#error You must create proper config.h for your platform
#endif

#endif
