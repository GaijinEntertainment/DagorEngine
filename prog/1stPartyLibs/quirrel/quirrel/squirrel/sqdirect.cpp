#include "sqpcheader.h"
#include "sqvm.h"
#include "squserdata.h"
#include "sqdirect.h"
#include "sqarray.h"
#include "sqtable.h"


SQRESULT sq_direct_get(HSQUIRRELVM v, const HSQOBJECT *obj, const HSQOBJECT *slot,
                      HSQOBJECT *out, bool raw)
{
    const SQObjectPtr &selfPtr = static_cast<const SQObjectPtr &>(*obj);
    const SQObjectPtr &slotPtr = static_cast<const SQObjectPtr &>(*slot);
    SQObjectPtr outPtr;

    SQUnsignedInteger getFlags = raw ?
      (GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW) : GET_FLAG_DO_NOT_RAISE_ERROR;

    bool res = v->Get(selfPtr, slotPtr, outPtr, getFlags, DONT_FALL_BACK);
    *out = outPtr;
    return res ? SQ_OK : SQ_ERROR;
}


SQBool sq_direct_tobool(const HSQOBJECT *o)
{
    const SQObjectPtr &objPtr = static_cast<const SQObjectPtr &>(*o);
    return SQVM::IsFalse(objPtr) ? SQFalse : SQTrue;
}


SQBool sq_direct_cmp(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b, SQInteger *res)
{
    const SQObjectPtr &aPtr = static_cast<const SQObjectPtr &>(*a);
    const SQObjectPtr &bPtr = static_cast<const SQObjectPtr &>(*b);

    return v->ObjCmp(aPtr, bPtr, *res);
}


bool sq_direct_is_equal(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b)
{
    const SQObjectPtr &aPtr = static_cast<const SQObjectPtr &>(*a);
    const SQObjectPtr &bPtr = static_cast<const SQObjectPtr &>(*b);

    bool res = false;
    bool status = v->IsEqual(aPtr, bPtr, res);
    (void)status;
    assert(status); // cannot fail
    return res;
}


#define MAX_FAST_COMPARE_SIZE 1024

static bool fastEqualByValue(const SQObjectPtr &a, const SQObjectPtr &b, int depth)
{
    if (sq_type(a) != sq_type(b))
    {
      if (!sq_isnumeric(a) || !sq_isnumeric(b))
          return false;
      return tofloat(a) == tofloat(b);
    }

    // same type
    if (_rawval(a) == _rawval(b))
        return true;

    if (depth <= 0)
        return false;

    if (sq_isarray(a))
    {
        auto aa = _array(a);
        auto ab = _array(b);
        if (aa->Size() != ab->Size())
            return false;

        if (aa->Size() > MAX_FAST_COMPARE_SIZE)
            return false;

        for (SQInteger i = 0; i < aa->Size(); i++)
            if (!fastEqualByValue(aa->_values[i], ab->_values[i], depth - 1))
                return false;

        return true;
    }
    else if (sq_istable(a))
    {
        auto ta = _table(a);
        auto tb = _table(b);
        if (ta->CountUsed() != tb->CountUsed())
            return false;

        if (ta->CountUsed() > MAX_FAST_COMPARE_SIZE)
            return false;

        SQObjectPtr iter;
        while (true)
        {
            SQObjectPtr key, va, vb;
            iter._unVal.nInteger = ta->Next(true, iter, key, va);
            iter._type = OT_INTEGER;
            if (_integer(iter) < 0)
                break;

            if (!tb->Get(key, vb))
                return false;
            if (!fastEqualByValue(va, vb, depth - 1))
                return false;
        }

        return true;
    }

    return false;
}


bool sq_fast_equal_by_value_deep(const HSQOBJECT *a, const HSQOBJECT *b, int depth)
{
    return fastEqualByValue(
        static_cast<const SQObjectPtr&>(*a),
        static_cast<const SQObjectPtr&>(*b),
        depth);
}


SQRESULT sq_direct_getuserdata(const HSQOBJECT *obj, SQUserPointer *p, SQUserPointer *typetag)
{
    if (obj->_type != OT_USERDATA)
      return SQ_ERROR;

    (*p) = _userdataval(*obj);
    if(typetag) *typetag = _userdata(*obj)->_typetag;
    return SQ_OK;
}
