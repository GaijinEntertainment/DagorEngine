/*
 * Squirrel API extensions
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 */
#ifndef _SQEXT_H_
#define _SQEXT_H_

#include "squirrel.h"


SQUIRREL_API SQRESULT sq_ext_getfuncinfo(HSQOBJECT obj, SQFunctionInfo *fi);
SQUIRREL_API SQRESULT sq_ext_get_array_floats(HSQOBJECT obj, int start, int count, float * dest);
SQUIRREL_API int sq_ext_get_array_int(HSQOBJECT obj, int index, int def = 0);
SQUIRREL_API float sq_ext_get_array_float(HSQOBJECT obj, int index, float def = 0.f);


#endif /*_SQEXT_H_*/
