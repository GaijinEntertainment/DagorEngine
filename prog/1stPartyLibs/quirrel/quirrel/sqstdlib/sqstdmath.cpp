/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <math.h>
#include <stdlib.h>
#include <sqstdmath.h>
#include <float.h>
#include <assert.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <chrono>
#include "sqstdhash.h"

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

#define SQ_MAX_INT_RANDOM 0x7FFFFFFF


static void math_randomize_seed(HSQUIRRELVM v) {
    SQUnsignedInteger32 s = SQUnsignedInteger32(size_t(v->_sharedstate)) * 123 + SQUnsignedInteger32(size_t(&v));
    s ^= SQUnsignedInteger32(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    v->_sharedstate->rand_seed += s ^ (s >> 10);
}

static SQInteger math_srand(HSQUIRRELVM v)
{
    SQInteger i;
    if(SQ_FAILED(sq_getinteger(v,2,&i)))
        return sq_throwerror(v,"invalid param");
    v->_sharedstate->rand_seed = SQUnsignedInteger32(i);
    return 0;
}

// Constants 1664525, 1013904223 are based on the multiplier and increment suggested by Numerical Recipes
// and used by the PCG family of random number generators.
// This LCG has a full period of 2^32 and a good spectral test value of M_8 = 0.73201

static SQInteger math_rand(HSQUIRRELVM v)
{
    SQUnsignedInteger32 seed = v->_sharedstate->rand_seed = 1664525 * v->_sharedstate->rand_seed + 1013904223;
    sq_pushinteger(v, (seed ^ (seed >> 16u)) & SQ_MAX_INT_RANDOM);
    return 1;
}

static SQInteger math_abs(HSQUIRRELVM v)
{
    SQObject arg;
    sq_getstackobj(v, 2, &arg);
    if (sq_type(arg) == OT_FLOAT)
        sq_pushfloat(v, fabs(_float(arg)));
    else
        sq_pushinteger(v, abs(_integer(arg)));
    return 1;
}

static SQInteger math_acos(HSQUIRRELVM v) {
    SQFloat f;
    sq_getfloat(v, 2, &f);
    if (f < SQFloat(-1))
      f = SQFloat(-1);
    if (f > SQFloat(1))
      f = SQFloat(1);
    sq_pushfloat(v, (SQFloat)acos(f));
    return 1;
}

static SQInteger math_asin(HSQUIRRELVM v) {
    SQFloat f;
    sq_getfloat(v, 2, &f);
    if (f < SQFloat(-1))
      f = SQFloat(-1);
    if (f > SQFloat(1))
      f = SQFloat(1);
    sq_pushfloat(v, (SQFloat)asin(f));
    return 1;
}

SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(fabs)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
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
      return sq_throwerror(v, "Internal error, comparison failed");

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

  const char *cmpFailedErrText = "Internal error, comparison failed";

  if (!sq_cmpraw(v, lo, hi, cres))
    return sq_throwerror(v, cmpFailedErrText);

  if (cres > 0)
    return sq_throwerror(v, "Invalid clamp range: min>max");

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

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static const SQRegFunctionFromStr mathlib_funcs[] = {
    { math_sqrt,   "pure sqrt(x: number): number",   "Returns the square root of x" },
    { math_sin,    "pure sin(x: number): number",    "Returns the sine of x (in radians)" },
    { math_cos,    "pure cos(x: number): number",    "Returns the cosine of x (in radians)" },
    { math_asin,   "pure asin(x: number): number",   "Returns the arcsine of x" },
    { math_acos,   "pure acos(x: number): number",   "Returns the arccosine of x" },
    { math_log,    "pure log(x: number): number",    "Returns the natural logarithm of x" },
    { math_log10,  "pure log10(x: number): number",  "Returns the base-10 logarithm of x" },
    { math_tan,    "pure tan(x: number): number",    "Returns the tangent of x (in radians)" },
    { math_atan,   "pure atan(x: number): number",   "Returns the arctangent of x" },
    { math_atan2,  "pure atan2(y: number, x: number): number", "Returns the arctangent of y/x using the signs to determine the quadrant" },
    { math_pow,    "pure pow(x: number, y: number): number", "Returns x raised to the power y" },
    { math_floor,  "pure floor(x: number): number",  "Rounds x downward to the nearest integer" },
    { math_ceil,   "pure ceil(x: number): number",   "Rounds x upward to the nearest integer" },
    { math_round,  "pure round(x: number): number",  "Rounds x to the nearest integer" },
    { math_exp,    "pure exp(x: number): number",    "Returns e raised to the power of x" },
    { math_srand,  "srand(seed: int)",               "Sets the seed for rand()" },
    { math_rand,   "rand(): int",                    "Returns a random integer in range 0..RAND_MAX" },
    { math_fabs,   "pure fabs(x: number): float",    "Returns the absolute value of float(x)" },
    { math_abs,    "pure abs(x: number): number",    "Returns the absolute value of x" },
    { math_min,    "pure min(x: number, y: number, ...: number): number", "Returns the minimum value from the arguments" },
    { math_max,    "pure max(x: number, y: number, ...: number): number", "Returns the maximum value from the arguments" },
    { math_clamp,  "pure clamp(x: number, min: number, max: number): number", "Clamps x to the range [min, max]" },
    { sq_math_hash, "pure hash(obj): int",           "Returns a hash value for the given object" },
    { sq_math_deep_hash, "pure deep_hash(obj, [depth: int]): int", "Returns a hash value for the given object, hashing nested objects up to the specified depth" },
    { NULL, NULL, NULL }
};

SQRESULT sqstd_register_mathlib(HSQUIRRELVM v)
{
    math_randomize_seed(v);

    SQInteger i = 0;
    while (mathlib_funcs[i].f) {
        sq_new_closure_slot_from_decl_string(v, mathlib_funcs[i].f, 0, mathlib_funcs[i].declstring, mathlib_funcs[i].docstring);
        i++;
    }

    // Constants
    sq_pushstring(v,"RAND_MAX",-1);
    sq_pushinteger(v,SQ_MAX_INT_RANDOM);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,"PI",-1);
    sq_pushfloat(v,(SQFloat)M_PI);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,"FLT_MAX",-1);
    sq_pushfloat(v,(SQFloat)FLT_MAX);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,"FLT_MIN",-1);
    sq_pushfloat(v,(SQFloat)FLT_MIN);
    sq_newslot(v,-3,SQFalse);

    return SQ_OK;
}
