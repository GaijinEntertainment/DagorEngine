/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <float.h>
#include <assert.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqclass.h>
#include <squirrel/sqarray.h>

// FNV-1a hash parameters
#define SQ_M_HASH_MULTIPLIER 1099511628211LU
#define SQ_M_HASH_INIT 14695981039346656037LU

static SQInteger math_recursive_hash_impl(HSQUIRRELVM vm, HSQOBJECT &obj, SQUnsignedInteger prev_hash,
  SQUnsignedInteger &out_hash, SQInteger depth)
{
  if (depth <= 0) {
    out_hash = prev_hash;
    return SQ_OK;
  }

  SQObjectType objType = sq_type(obj);
  prev_hash = (objType ^ prev_hash) * SQ_M_HASH_MULTIPLIER;

  switch (objType) {
    case OT_NULL:
    case OT_BOOL:
    case OT_INTEGER:
      out_hash = (SQUnsignedInteger(_integer(obj)) ^ prev_hash) * SQ_M_HASH_MULTIPLIER;
      break;
    case OT_FLOAT: {
      #ifdef SQUSEDOUBLE
        uint64_t fbits = 0;
        memcpy(&fbits, &_float(obj), sizeof(SQFloat));
        out_hash = (SQUnsignedInteger(fbits ^ (fbits >> 42u)) ^ prev_hash) * SQ_M_HASH_MULTIPLIER;
      #else
        uint32_t fbits = 0;
        memcpy(&fbits, &_float(obj), sizeof(SQFloat));
        out_hash = (SQUnsignedInteger(fbits ^ (fbits >> 21u)) ^ prev_hash) * SQ_M_HASH_MULTIPLIER;
      #endif
      break;
    }
    case OT_STRING: {
      SQString *str = _string(obj);
      out_hash = (prev_hash ^ str->_len) * SQ_M_HASH_MULTIPLIER;
      out_hash = (out_hash ^ str->_hash) * SQ_M_HASH_MULTIPLIER;
      break;
    }
    case OT_ARRAY: {
      SQArray *arr = _array(obj);
      SQUnsignedInteger h = prev_hash;
      SQInteger arrSize = arr->Size();
      for (SQInteger i = 0; i < arrSize; ++i) {
        SQUnsignedInteger resultHash = 0;
        if (SQ_FAILED(math_recursive_hash_impl(vm, arr->_values[i], h, resultHash, depth - 1)))
          return SQ_ERROR;
        h = resultHash;
      }
      out_hash = h;
      break;
    }
    case OT_TABLE:
    case OT_CLASS:
    case OT_INSTANCE: {
      out_hash = prev_hash;
      sq_pushobject(vm, obj);
      sq_pushnull(vm);
      while (SQ_SUCCEEDED(sq_next(vm, -2))) {
        SQUnsignedInteger resultHash = 0;
        HSQOBJECT key = stack_get(vm, -2);
        if (SQ_FAILED(math_recursive_hash_impl(vm, key, out_hash, resultHash, depth - 1)))
          return SQ_ERROR;
        out_hash = resultHash;
        HSQOBJECT val = stack_get(vm, -1);
        if (SQ_FAILED(math_recursive_hash_impl(vm, val, out_hash, resultHash, depth - 1)))
          return SQ_ERROR;
        out_hash = resultHash;
        sq_pop(vm, 2);
      }
      sq_pop(vm, 2);
      break;
    }

    default:
      out_hash = prev_hash;
      return sq_throwerror(vm, "unsupported type for hashing");
  }

  out_hash ^= out_hash >> 15;
  return SQ_OK;
}

SQInteger sq_math_hash(HSQUIRRELVM v)
{
  HSQOBJECT obj;
  sq_getstackobj(v, 2, &obj);
  SQUnsignedInteger resultHash = 0;
  if (SQ_FAILED(math_recursive_hash_impl(v, obj, SQ_M_HASH_INIT, resultHash, 1)))
    return SQ_ERROR;
  sq_pushinteger(v, SQInteger(resultHash) & (~SQUnsignedInteger(0) >> 1));
  return 1;
}

SQInteger sq_math_deep_hash(HSQUIRRELVM v)
{
  SQInteger argDepth = 200; // not a very deep by default to avoid stack overflows in case of circular references
  if (sq_gettop(v) >= 3) {
    if (SQ_FAILED(sq_getinteger(v, 3, &argDepth)))
      return sq_throwerror(v, "invalid param");
    if (argDepth < 1)
      return sq_throwerror(v, "hashing depth must be >= 1");
  }
  HSQOBJECT obj;
  sq_getstackobj(v, 2, &obj);
  SQUnsignedInteger resultHash = 0;
  if (SQ_FAILED(math_recursive_hash_impl(v, obj, SQ_M_HASH_INIT, resultHash, argDepth)))
    return SQ_ERROR;
  sq_pushinteger(v, SQInteger(resultHash) & (~SQUnsignedInteger(0) >> 1));
  return 1;
}
