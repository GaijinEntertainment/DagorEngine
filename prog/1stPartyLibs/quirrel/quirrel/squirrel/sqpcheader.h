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
#include <assert.h>

#ifdef QUIRREL_HOST_HEADER
#define _SQ_QHH_STR(x) #x
#define _SQ_QHH_STR2(x) _SQ_QHH_STR(x)
#include _SQ_QHH_STR2(QUIRREL_HOST_HEADER)
#undef _SQ_QHH_STR
#undef _SQ_QHH_STR2
#endif

#include <new>
//squirrel stuff
#include <squirrel.h>
#include "sqobject.h"
#include "sqstate.h"

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#include <charconv>
#define SQ_USE_STD_FROM_CHARS 1
#endif

#ifndef SQ_RUNTIME_TYPE_CHECK
#define SQ_RUNTIME_TYPE_CHECK 1
#endif

#ifndef SQ_WATCHDOG_ENABLED
#define SQ_WATCHDOG_ENABLED 0
#endif

#endif //_SQPCHEADER_H_
