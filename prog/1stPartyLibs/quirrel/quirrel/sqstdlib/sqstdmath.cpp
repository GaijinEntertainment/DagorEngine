/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <math.h>
#include <stdlib.h>
#include <sqstdmath.h>
#include <float.h>

#define SINGLE_ARG_FUNC(_funcname) static SQInteger math_##_funcname(HSQUIRRELVM v){ \
    SQFloat f; \
    sq_getfloat(v,2,&f); \
    sq_pushfloat(v,(SQFloat)_funcname(f)); \
    return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static SQInteger math_##_funcname(HSQUIRRELVM v){ \
    SQFloat p1,p2; \
    sq_getfloat(v,2,&p1); \
    sq_getfloat(v,3,&p2); \
    sq_pushfloat(v,(SQFloat)_funcname(p1,p2)); \
    return 1; \
}

static SQInteger math_srand(HSQUIRRELVM v)
{
    SQInteger i;
    if(SQ_FAILED(sq_getinteger(v,2,&i)))
        return sq_throwerror(v,_SC("invalid param"));
    srand((unsigned int)i);
    return 0;
}

static SQInteger math_rand(HSQUIRRELVM v)
{
    sq_pushinteger(v,rand());
    return 1;
}

static SQInteger math_abs(HSQUIRRELVM v)
{
    SQInteger n;
    sq_getinteger(v,2,&n);
    sq_pushinteger(v,(SQInteger)abs((SQInteger)n));
    return 1;
}

SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(fabs)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
SINGLE_ARG_FUNC(asin)
SINGLE_ARG_FUNC(acos)
SINGLE_ARG_FUNC(log)
SINGLE_ARG_FUNC(log10)
SINGLE_ARG_FUNC(tan)
SINGLE_ARG_FUNC(atan)
TWO_ARGS_FUNC(atan2)
TWO_ARGS_FUNC(pow)
SINGLE_ARG_FUNC(floor)
SINGLE_ARG_FUNC(ceil)
SINGLE_ARG_FUNC(round)
SINGLE_ARG_FUNC(exp)

template<SQInteger CmpRes>
static SQInteger math_min_max(HSQUIRRELVM v)
{
  SQInteger nArgs = sq_gettop(v);
  SQObject objRes;
  sq_getstackobj(v, 2, &objRes);

  for (SQInteger i = 3; i <= nArgs; ++i) {
    SQObject cur;
    sq_getstackobj(v, i, &cur);
    if (!(sq_type(cur) & SQOBJECT_NUMERIC)) {
      sq_throwparamtypeerror(v, i, _RT_FLOAT | _RT_INTEGER, sq_type(cur));
      return SQ_ERROR;
    }

    SQInteger cres = 0;
    if (!sq_cmpraw(v, cur, objRes, cres))
      return sq_throwerror(v, _SC("Internal error, comparison failed"));

    if (cres == CmpRes)
      objRes = cur;
  }

  sq_pushobject(v, objRes);
  return 1;
}

static SQInteger math_min(HSQUIRRELVM v)
{
  return math_min_max<-1>(v);
}

static SQInteger math_max(HSQUIRRELVM v)
{
  return math_min_max<+1>(v);
}

static SQInteger math_clamp(HSQUIRRELVM v)
{
  SQObject x, lo, hi;
  SQInteger cres = 0;

  sq_getstackobj(v, 2, &x);
  sq_getstackobj(v, 3, &lo);
  sq_getstackobj(v, 4, &hi);

  const SQChar *cmpFailedErrText = _SC("Internal error, comparison failed");

  if (!sq_cmpraw(v, lo, hi, cres))
    return sq_throwerror(v, cmpFailedErrText);

  if (cres > 0)
    return sq_throwerror(v, _SC("Invalid clamp range: min>max"));

  if (!sq_cmpraw(v, x, lo, cres))
    return sq_throwerror(v, cmpFailedErrText);

  if (cres < 0) {
    sq_pushobject(v, lo);
    return 1;
  }

  if (!sq_cmpraw(v, x, hi, cres))
    return sq_throwerror(v, cmpFailedErrText);

  if (cres > 0) {
    sq_pushobject(v, hi);
    return 1;
  }

  sq_pushobject(v, x);
  return 1;
}


#define _DECL_FUNC(name,nparams,tycheck) {_SC(#name),math_##name,nparams,tycheck}
static const SQRegFunction mathlib_funcs[] = {
    _DECL_FUNC(sqrt,2,_SC(".n")),
    _DECL_FUNC(sin,2,_SC(".n")),
    _DECL_FUNC(cos,2,_SC(".n")),
    _DECL_FUNC(asin,2,_SC(".n")),
    _DECL_FUNC(acos,2,_SC(".n")),
    _DECL_FUNC(log,2,_SC(".n")),
    _DECL_FUNC(log10,2,_SC(".n")),
    _DECL_FUNC(tan,2,_SC(".n")),
    _DECL_FUNC(atan,2,_SC(".n")),
    _DECL_FUNC(atan2,3,_SC(".nn")),
    _DECL_FUNC(pow,3,_SC(".nn")),
    _DECL_FUNC(floor,2,_SC(".n")),
    _DECL_FUNC(ceil,2,_SC(".n")),
    _DECL_FUNC(round,2,_SC(".n")),
    _DECL_FUNC(exp,2,_SC(".n")),
    _DECL_FUNC(srand,2,_SC(".n")),
    _DECL_FUNC(rand,1,NULL),
    _DECL_FUNC(fabs,2,_SC(".n")),
    _DECL_FUNC(abs,2,_SC(".n")),
    _DECL_FUNC(min,-3,_SC(".nnnnnnnn")),
    _DECL_FUNC(max,-3,_SC(".nnnnnnnn")),
    _DECL_FUNC(clamp,4,_SC(".nnn")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

SQRESULT sqstd_register_mathlib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(mathlib_funcs[i].name!=0) {
        sq_pushstring(v,mathlib_funcs[i].name,-1);
        sq_newclosure(v,mathlib_funcs[i].f,0);
        sq_setparamscheck(v,mathlib_funcs[i].nparamscheck,mathlib_funcs[i].typemask);
        sq_setnativeclosurename(v,-1,mathlib_funcs[i].name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    sq_pushstring(v,_SC("RAND_MAX"),-1);
    sq_pushinteger(v,RAND_MAX);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,_SC("PI"),-1);
    sq_pushfloat(v,(SQFloat)M_PI);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,_SC("FLT_MAX"),-1);
    sq_pushfloat(v,(SQFloat)FLT_MAX);
    sq_newslot(v,-3,SQFalse);
    return SQ_OK;
}
