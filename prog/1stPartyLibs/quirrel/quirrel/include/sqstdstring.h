/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_STRING_H_
#define _SQSTD_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int SQRexBool;
typedef struct SQRex SQRex;
typedef struct SQAllocContextT * SQAllocContext;

typedef struct {
    const char *begin;
    SQInteger len;
} SQRexMatch;

SQUIRREL_API SQRex *sqstd_rex_compile(SQAllocContext ctx, const char *pattern,const char **error);
SQUIRREL_API void sqstd_rex_free(SQRex *exp);
SQUIRREL_API SQBool sqstd_rex_match(SQRex* exp,const char* text);
SQUIRREL_API SQBool sqstd_rex_search(SQRex* exp,const char* text, const char** out_begin, const char** out_end);
SQUIRREL_API SQBool sqstd_rex_searchrange(SQRex* exp,const char* text_begin,const char* text_end,const char** out_begin, const char** out_end);
SQUIRREL_API SQInteger sqstd_rex_getsubexpcount(SQRex* exp);
SQUIRREL_API SQBool sqstd_rex_getsubexp(SQRex* exp, SQInteger n, SQRexMatch *subexp);

SQUIRREL_API SQRESULT sqstd_format(HSQUIRRELVM v,SQInteger nformatstringidx,SQInteger *outlen,char **output);

SQUIRREL_API void sqstd_pushstringf(HSQUIRRELVM v,const char *s,...);

SQUIRREL_API SQRESULT sqstd_register_stringlib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTD_STRING_H_*/
