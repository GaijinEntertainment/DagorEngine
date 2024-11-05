/*
 * Squirrel API extensions
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * Manipulate with Squirrel objects directlywithout unnecessary use of
 * stack and refcount
 */

#ifndef _SQDIRECT_H_
#define _SQDIRECT_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQRESULT sq_direct_get(HSQUIRRELVM v, const HSQOBJECT *obj, const HSQOBJECT *slot,
                                    HSQOBJECT *out, bool raw);

#define   sq_direct_tointeger   sq_objtointeger
#define   sq_direct_tofloat     sq_objtofloat

SQUIRREL_API SQBool sq_direct_tobool(const HSQOBJECT *o);

SQUIRREL_API SQBool sq_direct_cmp(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b, SQInteger *res);
SQUIRREL_API bool sq_direct_is_equal(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b);

SQUIRREL_API bool sq_fast_equal_by_value_deep(const HSQOBJECT *a, const HSQOBJECT *b, int depth);

SQUIRREL_API SQRESULT sq_direct_getuserdata(const HSQOBJECT *obj, SQUserPointer *p, SQUserPointer *typetag=NULL);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _SQDIRECT_H_ */
