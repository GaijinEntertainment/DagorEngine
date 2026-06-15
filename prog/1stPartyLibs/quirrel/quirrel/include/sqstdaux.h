/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_AUXLIB_H_
#define _SQSTD_AUXLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API void sqstd_seterrorhandlers(HSQUIRRELVM v);
SQUIRREL_API void sqstd_printcallstack(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_formatcallstackstring(HSQUIRRELVM v);
SQUIRREL_API void sqstd_aux_error_to_string(HSQUIRRELVM v, SQInteger idx);

// Push `trace` (the captured async-fault trace array) rendered as a string
// (SQ_ERROR when it is not an array / has no frames).
SQUIRREL_API SQRESULT sqstd_formaterrortracestring(HSQUIRRELVM v, HSQOBJECT trace);

// Push the live callstack if it has frames, else `trace` (the async fault's
// captured trace array) rendered as a string. SQ_ERROR if neither is available:
// `trace` is null/not-an-array and there is no live callstack.
SQUIRREL_API SQRESULT sqstd_formaterrorcontextstring(HSQUIRRELVM v, HSQOBJECT trace);

SQUIRREL_API SQRESULT sqstd_throwerrorf(HSQUIRRELVM v,const char *err,...);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* _SQSTD_AUXLIB_H_ */
