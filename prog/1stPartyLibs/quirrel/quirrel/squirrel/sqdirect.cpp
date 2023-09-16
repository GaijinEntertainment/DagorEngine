#include "sqpcheader.h"
#include "sqvm.h"
#include "squserdata.h"
#include "sqdirect.h"


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

    SQObjectType t1 = sq_type(*a), t2 = sq_type(*b);
    if (t1 == t2 || t1==OT_NULL || t2==OT_NULL || (sq_isnumeric(*a) && sq_isnumeric(*b)))
    {
      SQBool status = v->ObjCmp(aPtr, bPtr, *res);
      assert(status);
      return status;
    }
    return false;
}

SQRESULT sq_direct_getuserdata(const HSQOBJECT *obj, SQUserPointer *p, SQUserPointer *typetag)
{
    if (obj->_type != OT_USERDATA)
      return SQ_ERROR;

    (*p) = _userdataval(*obj);
    if(typetag) *typetag = _userdata(*obj)->_typetag;
    return SQ_OK;
}
