// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// winnt.h FIELD_OFFSET derefs null, and DIJOFS_* expand it at the use site: same constants, without the UB
#if defined(__clang__)
#undef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((LONG) __builtin_offsetof(type, field))
#endif
