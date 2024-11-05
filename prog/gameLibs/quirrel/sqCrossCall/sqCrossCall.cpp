// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqCrossCall/sqCrossCall.h>
#include <debug/dag_assert.h>
#include <util/dag_string.h>
#include <sqstdblob.h>

static bool repush(HSQUIRRELVM vm_from, SQInteger idx, HSQUIRRELVM vm_to, int depth, String *err_msg = nullptr)
{
  // In case of the same VM instead of pushing object reference make a deep copy
  // if (vm_from == vm_to)
  // {
  //   sq_push(vm_to, idx);
  //   return true;
  // }

  if (!sq_canaccessfromthisthread(vm_from))
  {
    if (err_msg)
      err_msg->printf(0, "Can't access source VM from this thread");
    else
      G_ASSERTF(0, "Can't access source VM from this thread");
    return false;
  }

  if (!sq_canaccessfromthisthread(vm_to))
  {
    if (err_msg)
      err_msg->printf(0, "Can't access destination VM from this thread");
    else
      G_ASSERTF(0, "Can't access destination VM from this thread");
    return false;
  }

  SQInteger prevTopFrom = sq_gettop(vm_from), prevTopTo = sq_gettop(vm_to);
  (void)prevTopFrom;
  (void)prevTopTo;

  SQObjectType objType = sq_gettype(vm_from, idx);
  bool succeeded = true;

  if (depth == 0 && err_msg)
    clear_and_shrink(*err_msg);

  switch (objType)
  {
    case OT_NULL:
    {
      sq_pushnull(vm_to);
      break;
    }
    case OT_INTEGER:
    {
      SQInteger val = 0;
      G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm_from, idx, &val)));
      sq_pushinteger(vm_to, val);
      break;
    }
    case OT_FLOAT:
    {
      SQFloat val = 0;
      G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm_from, idx, &val)));
      sq_pushfloat(vm_to, val);
      break;
    }
    case OT_BOOL:
    {
      SQBool val = SQFalse;
      G_VERIFY(SQ_SUCCEEDED(sq_getbool(vm_from, idx, &val)));
      sq_pushbool(vm_to, val);
      break;
    }
    case OT_STRING:
    {
      const SQChar *val = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm_from, idx, &val)));
      sq_pushstring(vm_to, val, -1);
      break;
    }
    case OT_ARRAY:
    {
      SQInteger len = sq_getsize(vm_from, idx);
      sq_push(vm_from, idx); // array working reference
      SQInteger srcArrIdx = sq_gettop(vm_from);
      sq_newarray(vm_to, len);
      SQInteger dstArrIdx = sq_gettop(vm_to);
      for (SQInteger i = 0; i < len; ++i)
      {
        sq_pushinteger(vm_from, i);
        G_VERIFY(SQ_SUCCEEDED(sq_get_noerr(vm_from, srcArrIdx))); // for array must always succeed
        SQInteger srcValIdx = sq_gettop(vm_from);
        sq_pushinteger(vm_to, i);
        if (!repush(vm_from, srcValIdx, vm_to, depth + 1, err_msg))
          succeeded = false;

        G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm_to, dstArrIdx))); // for array must always succeed
        sq_pop(vm_from, 1);                                  // pop source item value
      }
      sq_pop(vm_from, 1); // array working reference
      break;
    }
    case OT_TABLE:
    case OT_CLASS:    // copy class to table
    case OT_INSTANCE: // copy instance to table
    {
      // special handling of blobs
      SQUserPointer blobData = nullptr;
      if (objType == OT_INSTANCE && SQ_SUCCEEDED(sqstd_getblob(vm_from, idx, &blobData)))
      {
        SQInteger len = sqstd_getblobsize(vm_from, idx);
        G_ASSERT(len >= 0);
        SQUserPointer destPtr = sqstd_createblob(vm_to, len);
        G_ASSERT(destPtr);
        memcpy(destPtr, blobData, len);
        break;
      }

      sq_newtable(vm_to);
      SQInteger dstTableIdx = sq_gettop(vm_to);
      sq_push(vm_from, idx);
      SQInteger srcTableIdx = sq_gettop(vm_from);
      sq_pushnull(vm_from);
      SQInteger iteratorIdx = sq_gettop(vm_from);
      while (SQ_SUCCEEDED(sq_next(vm_from, srcTableIdx)))
      {
        G_ASSERT(sq_gettop(vm_from) == iteratorIdx + 2);
        if (!repush(vm_from, iteratorIdx + 1, vm_to, depth + 1, err_msg))
          succeeded = false;
        if (!repush(vm_from, iteratorIdx + 2, vm_to, depth + 1, err_msg))
          succeeded = false;

        if (!succeeded)
        {
          sq_pop(vm_to, 2);   // don't need to set invalid values
          sq_pop(vm_from, 2); // pops key and val before the next iteration
          continue;
        }

        G_ASSERTF(sq_gettype(vm_to, dstTableIdx) == OT_TABLE, "%X", sq_gettype(vm_to, dstTableIdx));
        if (!SQ_SUCCEEDED(sq_rawset(vm_to, dstTableIdx)))
        {
          G_ASSERT(!"set() failed");
          sq_pop(vm_to, 2);
        }
        sq_pop(vm_from, 2); // pops key and val before the next iteration
      }
      sq_pop(vm_from, 2); // pops the iterator and table

      break;
    }

    default:
      if (err_msg)
        err_msg->printf(0, "Unexpected obj #%d type %X (depth: %d)", idx, objType, depth);
      else
        G_ASSERTF(0, "Unexpected obj #%d type %X (depth: %d)", idx, objType, depth);
      // maintain arguments count and order
      sq_pushnull(vm_to);
      succeeded = false;
      break;
  }

  if (vm_to != vm_from)
  {
    G_ASSERTF(sq_gettop(vm_from) == prevTopFrom, "'From' stack size %d -> %d, idx = %d@%d", prevTopFrom, sq_gettop(vm_from), idx,
      depth);
  }
  G_ASSERTF(sq_gettop(vm_to) == prevTopTo + 1, "'To' stack size %d -> %d, idx = %d@%d", prevTopTo, sq_gettop(vm_to), idx, depth);

  return succeeded;
}


bool sq_cross_push(HSQUIRRELVM vm_from, SQInteger idx, HSQUIRRELVM vm_to, String *err_msg)
{
  return repush(vm_from, idx, vm_to, 0, err_msg);
}


SQInteger sq_cross_call(HSQUIRRELVM vm_from, HSQUIRRELVM vm_to, HSQOBJECT namespace_obj)
{
  int toTop = sq_gettop(vm_to);

  // put namespace on top vm_to find function in it
  sq_pushobject(vm_to, namespace_obj);

  if (sq_gettype(vm_from, 2) == OT_ARRAY)
  {
    // slot for sq_next() iterator
    sq_pushnull(vm_from);

    int pathLen = sq_getsize(vm_from, 2);
    for (int i = 0; i < pathLen; ++i)
    {
      const char *key = nullptr;
      sq_next(vm_from, 2);
      sq_getstring(vm_from, sq_gettop(vm_from), &key);

      sq_pushstring(vm_to, key, -1);
      if (SQ_FAILED(sq_get(vm_to, sq_gettop(vm_to) - 1)))
      {
        // pop iterator for sq_next()
        sq_pop(vm_from, 1);
        sq_settop(vm_to, toTop);
        sq_pushnull(vm_from);
        return 1;
      }

      sq_pop(vm_from, 2);
    }

    // pop iterator for sq_next()
    sq_pop(vm_from, 1);
  }
  else if (sq_gettype(vm_from, 2) == OT_STRING)
  {
    const char *fname = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm_from, 2, &fname)));
    sq_pushstring(vm_to, fname, -1);
    if (SQ_FAILED(sq_get(vm_to, sq_gettop(vm_to) - 1)))
    {
      sq_pushnull(vm_from);
      sq_settop(vm_to, toTop);
      return 1;
    }
  }

  if (sq_gettype(vm_to, sq_gettop(vm_to)) != OT_CLOSURE)
  {
    sq_pushnull(vm_from);
    sq_settop(vm_to, toTop);
    return 1;
  }

  HSQOBJECT env;
  sq_getstackobj(vm_to, sq_gettop(vm_to) - 1, &env);
  sq_pushobject(vm_to, env);

  int nArgs = sq_gettop(vm_from);
  String errMsg;
  for (int iArg = 3; iArg <= nArgs; ++iArg)
  {
    if (!repush(vm_from, iArg, vm_to, 0, &errMsg))
      return sq_throwerror(vm_from, errMsg);
  }

  if (SQ_FAILED(sq_call(vm_to, nArgs - 1, SQTrue, SQTrue)))
    return sq_throwerror(vm_from, "Failed to call function in another VM");
  if (!repush(vm_to, sq_gettop(vm_to), vm_from, 0, &errMsg))
    return sq_throwerror(vm_from, errMsg);
  sq_settop(vm_to, toTop);
  return 1;
}
