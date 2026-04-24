//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Dagor-side host header for Quirrel (selected via -DQUIRREL_HOST_HEADER).
// Supplies Dagor's assert handler, sqratConfig path, etc.

#ifndef __EMSCRIPTEN__

#define DAGOR_QUIRREL_HOST_PRESENT 1

#include <util/dag_globDef.h>

#ifdef assert
#undef assert //-V1059
#endif
#define assert G_ASSERT //-V1059

#endif // !__EMSCRIPTEN__
