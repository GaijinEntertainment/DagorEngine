#ifndef __DAGOR_PLATFORM_H
#define __DAGOR_PLATFORM_H

#if _TARGET_PC_WIN
#include <windows.h>
#include <mmsystem.h>
#include <dbghelp.h>

#undef MF_POPUP

#elif _TARGET_XBOX
#include <windows.h>

#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID

#endif

#endif
