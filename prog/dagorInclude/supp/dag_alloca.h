//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <malloc.h>
#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
#include <alloca.h>
#elif defined(__GNUC__)
#include <stdlib.h>
#endif
