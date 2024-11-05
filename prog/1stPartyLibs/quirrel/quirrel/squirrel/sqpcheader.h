/*  see copyright notice in squirrel.h */
#ifndef _SQPCHEADER_H_
#define _SQPCHEADER_H_

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__

#include <assert.h>

#else //__EMSCRIPTEN__

#include <util/dag_globDef.h>

#ifdef assert
#undef assert
#endif
#define assert G_ASSERT

#endif //__EMSCRIPTEN__

#include <new>
//squirrel stuff
#include <squirrel.h>
#include "sqobject.h"
#include "sqstate.h"

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#include <charconv>
#define SQ_USE_STD_FROM_CHARS 1
#endif

#endif //_SQPCHEADER_H_
