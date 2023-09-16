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

#endif //_SQPCHEADER_H_
