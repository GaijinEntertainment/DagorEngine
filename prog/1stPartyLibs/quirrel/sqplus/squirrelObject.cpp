#include "sqplus.h"

#include <sqstdblob.h>


SquirrelObject::SquirrelObject(void)
{
  sq_resetobject(&_o);
}

SquirrelObject::~SquirrelObject()
{
  Reset();
}

SquirrelObject::SquirrelObject(const SquirrelObject &o)
{
  _o = o._o;
  sq_addref(SquirrelVM::_VM,&_o);
}

SquirrelObject::SquirrelObject(HSQOBJECT o)
{
  _o = o;
  sq_addref(SquirrelVM::_VM,&_o);
}

void SquirrelObject::Reset()
{
  if(SquirrelVM::_VM)
  {
    HSQOBJECT c=_o;
    sq_resetobject(&_o);
    sq_release(SquirrelVM::_VM, &c);
  }
  else
    sq_resetobject(&_o);
}

SquirrelObject SquirrelObject::Clone()
{
  SquirrelObject ret;
  if(GetType() == OT_TABLE || GetType() == OT_ARRAY)
  {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_clone(SquirrelVM::_VM,-1);
    ret.AttachToStackObject(-1);
    sq_pop(SquirrelVM::_VM,2);
  }
  return ret;
}

SquirrelObject & SquirrelObject::operator =(const SquirrelObject &o)
{
  HSQOBJECT t;
  t = o._o;
  sq_addref(SquirrelVM::_VM,&t);
  sq_release(SquirrelVM::_VM,&_o);
  _o = t;
  return *this;
}

SquirrelObject & SquirrelObject::operator =(int n)
{
  sq_pushinteger(SquirrelVM::_VM,n);
  AttachToStackObject(-1);
  sq_pop(SquirrelVM::_VM,1);
  return *this;
}

void SquirrelObject::ArrayAppend(const SquirrelObject &o)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushobject(SquirrelVM::_VM,o._o);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppend(bool b)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushbool(SquirrelVM::_VM,b);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppend(SQInt32 n)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushinteger(SquirrelVM::_VM,n);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppend(SQInteger n)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushinteger(SquirrelVM::_VM,n);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppend(SQFloat f)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushfloat(SquirrelVM::_VM,f);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppend(const SQChar *s)
{
  if(sq_isarray(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    sq_pushstring(SquirrelVM::_VM, s,-1);
    sq_arrayappend(SquirrelVM::_VM,-2);
    sq_pop(SquirrelVM::_VM,1);
  }
}

void SquirrelObject::ArrayAppendStr(const char *str)
{
  ArrayAppend(str);
}

void SquirrelObject::AttachToStackObject(int idx)
{
  HSQOBJECT t;
  sq_getstackobj(SquirrelVM::_VM,idx,&t);
  sq_addref(SquirrelVM::_VM,&t);
  sq_release(SquirrelVM::_VM,&_o);
  _o = t;
}

SQBool SquirrelObject::IsNull() const
{
  return sq_isnull(_o);
}

SQBool SquirrelObject::IsNumeric() const
{
  return sq_isnumeric(_o);
}

int SquirrelObject::Len() const
{
  int ret = 0;
  if(sq_isarray(_o) || sq_istable(_o) || sq_isstring(_o)) {
    sq_pushobject(SquirrelVM::_VM,_o);
    ret = sq_getsize(SquirrelVM::_VM,-1);
    sq_pop(SquirrelVM::_VM,1);
  }
  return ret;
}

#define _SETVALUE_INT_BEGIN \
        SQBool ret = SQFalse; \
        int top = sq_gettop(SquirrelVM::_VM); \
        sq_pushobject(SquirrelVM::_VM,_o); \
        sq_pushinteger(SquirrelVM::_VM,key);

#define _SETVALUE_INT_END \
        if(SQ_SUCCEEDED(sq_rawset(SquirrelVM::_VM,-3))) { \
                ret = SQTrue; \
        } \
        sq_settop(SquirrelVM::_VM,top); \
        return ret;

SQBool SquirrelObject::SetValue(SQInteger key,const SquirrelObject &val)
{
        _SETVALUE_INT_BEGIN
        sq_pushobject(SquirrelVM::_VM,val._o);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(SQInteger key,SQInt32 n)
{
        _SETVALUE_INT_BEGIN
        sq_pushinteger(SquirrelVM::_VM,n);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(SQInteger key,SQInteger n)
{
        _SETVALUE_INT_BEGIN
        sq_pushinteger(SquirrelVM::_VM,n);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(SQInteger key,SQFloat f)
{
        _SETVALUE_INT_BEGIN
        sq_pushfloat(SquirrelVM::_VM,f);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(SQInteger key,const SQChar *s)
{
        _SETVALUE_INT_BEGIN
        sq_pushstring(SquirrelVM::_VM,s,-1);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(SQInteger key,bool b)
{
        _SETVALUE_INT_BEGIN
        sq_pushbool(SquirrelVM::_VM,b);
        _SETVALUE_INT_END
}

SQBool SquirrelObject::SetValue(const SquirrelObject &key,const SquirrelObject &val)
{
        SQBool ret = SQFalse;
        int top = sq_gettop(SquirrelVM::_VM);
        sq_pushobject(SquirrelVM::_VM,_o);
        sq_pushobject(SquirrelVM::_VM,key._o);
        sq_pushobject(SquirrelVM::_VM,val._o);
        if(SQ_SUCCEEDED(sq_rawset(SquirrelVM::_VM,-3))) {
                ret = SQTrue;
        }
        sq_settop(SquirrelVM::_VM,top);
        return ret;
}

#define _SETVALUE_STR_BEGIN \
        SQBool ret = SQFalse; \
        int top = sq_gettop(SquirrelVM::_VM); \
        sq_pushobject(SquirrelVM::_VM,_o); \
        sq_pushstring(SquirrelVM::_VM,key,-1);

#define _SETVALUE_STR_END \
        if(SQ_SUCCEEDED(sq_rawset(SquirrelVM::_VM,-3))) { \
                ret = SQTrue; \
        } \
        sq_settop(SquirrelVM::_VM,top); \
        return ret;

SQBool SquirrelObject::SetValue(const SQChar *key,const SquirrelObject &val)
{
        _SETVALUE_STR_BEGIN
        sq_pushobject(SquirrelVM::_VM,val._o);
        _SETVALUE_STR_END
}

SQBool SquirrelObject::SetValue(const SQChar *key,SQInt32 n)
{
        _SETVALUE_STR_BEGIN
        sq_pushinteger(SquirrelVM::_VM,n);
        _SETVALUE_STR_END
}

SQBool SquirrelObject::SetValue(const SQChar *key,SQInteger n)
{
        _SETVALUE_STR_BEGIN
        sq_pushinteger(SquirrelVM::_VM,n);
        _SETVALUE_STR_END
}

SQBool SquirrelObject::SetValue(const SQChar *key,SQFloat f)
{
        _SETVALUE_STR_BEGIN
        sq_pushfloat(SquirrelVM::_VM,f);
        _SETVALUE_STR_END
}

SQBool SquirrelObject::SetValue(const SQChar *key,const SQChar *s)
{
        _SETVALUE_STR_BEGIN
        sq_pushstring(SquirrelVM::_VM,s,-1);
        _SETVALUE_STR_END
}

SQBool SquirrelObject::SetValue(const SQChar *key,bool b)
{
        _SETVALUE_STR_BEGIN
        sq_pushbool(SquirrelVM::_VM,b);
        _SETVALUE_STR_END
}

// === BEGIN User Pointer, User Data ===

SQBool SquirrelObject::SetUserPointer(const SQChar * key,SQUserPointer up) {
  _SETVALUE_STR_BEGIN
  sq_pushuserpointer(SquirrelVM::_VM,up);
  _SETVALUE_STR_END
} // SquirrelObject::SetUserPointer

SQUserPointer SquirrelObject::GetUserPointer(const SQChar * key) {
  SQUserPointer ret = NULL;
  if (GetSlot(key)) {
    sq_getuserpointer(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::GetUserPointer

SQBool SquirrelObject::SetUserPointer(SQInteger key,SQUserPointer up) {
  _SETVALUE_INT_BEGIN
  sq_pushuserpointer(SquirrelVM::_VM,up);
  _SETVALUE_INT_END
} // SquirrelObject::SetUserPointer

SQUserPointer SquirrelObject::GetUserPointer(SQInteger key) {
  SQUserPointer ret = NULL;
  if (GetSlot(key)) {
    sq_getuserpointer(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::GetUserPointer

// === User Data ===

SQBool SquirrelObject::NewUserData(const SQChar * key,SQInteger size,SQUserPointer * typetag) {
  _SETVALUE_STR_BEGIN
  sq_newuserdata(SquirrelVM::_VM,size);
  if (typetag) {
    sq_settypetag(SquirrelVM::_VM,-1,typetag);
  } // if
  _SETVALUE_STR_END
} // SquirrelObject::NewUserData

SQBool SquirrelObject::GetUserData(const SQChar * key,SQUserPointer * data,SQUserPointer * typetag) {
  SQBool ret = false;
  if (GetSlot(key)) {
    sq_getuserdata(SquirrelVM::_VM,-1,data,typetag);
    sq_pop(SquirrelVM::_VM,1);
    ret = true;
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::GetUserData

SQBool SquirrelObject::RawGetUserData(const SQChar * key,SQUserPointer * data,SQUserPointer * typetag) {
  SQBool ret = false;
  if (RawGetSlot(key)) {
    sq_getuserdata(SquirrelVM::_VM,-1,data,typetag);
    sq_pop(SquirrelVM::_VM,1);
    ret = true;
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::RawGetUserData

SQBool SquirrelObject::NewBlob(SQInteger key,SQInteger size) {
  _SETVALUE_INT_BEGIN
  sqstd_createblob(SquirrelVM::_VM,size);
  _SETVALUE_INT_END
} // SquirrelObject::NewBlob

SQBool SquirrelObject::GetBlob(SQInteger key,SQUserPointer * data,SQInteger* size) {
  SQBool ret = false;
  if (GetSlot(key)) {
    sqstd_getblob(SquirrelVM::_VM,-1,data);
    if (size)
      *size = sqstd_getblobsize(SquirrelVM::_VM,-1);
    sq_pop(SquirrelVM::_VM,1);
    ret = true;
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::GetBlob

SQBool SquirrelObject::NewBlob(const SQChar * key,SQInteger size) {
  _SETVALUE_STR_BEGIN
  sqstd_createblob(SquirrelVM::_VM,size);
  _SETVALUE_STR_END
} // SquirrelObject::NewBlob

SQBool SquirrelObject::GetBlob(const SQChar * key,SQUserPointer * data,SQInteger* size) {
  SQBool ret = false;
  if (GetSlot(key)) {
    sqstd_getblob(SquirrelVM::_VM,-1,data);
    if (size)
      *size = sqstd_getblobsize(SquirrelVM::_VM,-1);
    sq_pop(SquirrelVM::_VM,1);
    ret = true;
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return ret;
} // SquirrelObject::GetBlob

// === END User Pointer ===

// === BEGIN Arrays ===

SQBool SquirrelObject::ArrayResize(SQInteger newSize) {
//  int top = sq_gettop(SquirrelVM::_VM);
  sq_pushobject(SquirrelVM::_VM,GetObjectHandle());
  SQBool res = sq_arrayresize(SquirrelVM::_VM,-1,newSize) == SQ_OK;
  sq_pop(SquirrelVM::_VM,1);
//  sq_settop(SquirrelVM::_VM,top);
  return res;
} // SquirrelObject::ArrayResize

SQBool SquirrelObject::ArrayExtend(SQInteger amount) {
  int newLen = Len()+amount;
  return ArrayResize(newLen);
} // SquirrelObject::ArrayExtend

SQBool SquirrelObject::ArrayReverse(void) {
  sq_pushobject(SquirrelVM::_VM,GetObjectHandle());
  SQBool res = sq_arrayreverse(SquirrelVM::_VM,-1) == SQ_OK;
  sq_pop(SquirrelVM::_VM,1);
  return res;
} // SquirrelObject::ArrayReverse

SquirrelObject SquirrelObject::ArrayPop(SQBool returnPoppedVal) {
  SquirrelObject ret;
  int top = sq_gettop(SquirrelVM::_VM);
  sq_pushobject(SquirrelVM::_VM,GetObjectHandle());
  if (sq_arraypop(SquirrelVM::_VM,-1,returnPoppedVal) == SQ_OK) {
    if (returnPoppedVal) {
      ret.AttachToStackObject(-1);
    } // if
  } // if
  sq_settop(SquirrelVM::_VM,top);
  return ret;
} // SquirrelObject::ArrayPop

// === END Arrays ===

SQObjectType SquirrelObject::GetType()
{
  return _o._type;
}

SQBool SquirrelObject::GetSlot(SQInteger key) const
{
  sq_pushobject(SquirrelVM::_VM,_o);
  sq_pushinteger(SquirrelVM::_VM,key);
  if(SQ_SUCCEEDED(sq_get(SquirrelVM::_VM,-2))) {
    return SQTrue;
  }

  return SQFalse;
}


SquirrelObject SquirrelObject::GetValue(SQInteger key)const
{
  SquirrelObject ret;
  if(GetSlot(key)) {
    ret.AttachToStackObject(-1);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

SQFloat SquirrelObject::GetFloat(SQInteger key) const
{
  SQFloat ret = 0.0f;
  if(GetSlot(key)) {
    sq_getfloat(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

SQInteger SquirrelObject::GetInt(SQInteger key) const
{
  SQInteger ret = 0;
  if(GetSlot(key)) {
    sq_getinteger(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

const SQChar *SquirrelObject::GetString(SQInteger key) const
{
  const SQChar *ret = NULL;
  if(GetSlot(key)) {
    sq_getstring(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

bool SquirrelObject::GetBool(SQInteger key) const
{
  SQBool ret = SQFalse;
  if(GetSlot(key)) {
    sq_tobool(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret?true:false;
}

SQBool SquirrelObject::Exists(const SQChar *key) const
{
  SQBool ret = SQFalse;
  if(GetSlot(key)) {
    ret = SQTrue;
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SQBool SquirrelObject::GetSlot(const SQChar *name) const
{
  sq_pushobject(SquirrelVM::_VM,_o);
  sq_pushstring(SquirrelVM::_VM,name,-1);
  if(SQ_SUCCEEDED(sq_get(SquirrelVM::_VM,-2))) {
    return SQTrue;
  }

  return SQFalse;
}

SQBool SquirrelObject::RawGetSlot(const SQChar *name) const {
  sq_pushobject(SquirrelVM::_VM,_o);
  sq_pushstring(SquirrelVM::_VM,name,-1);
  if(SQ_SUCCEEDED(sq_rawget(SquirrelVM::_VM,-2))) {
    return SQTrue;
  }
  return SQFalse;
}

SquirrelObject SquirrelObject::GetValue(const SQChar *key)const
{
  SquirrelObject ret;
  if(GetSlot(key)) {
    ret.AttachToStackObject(-1);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

SQFloat SquirrelObject::GetFloat(const SQChar *key, SQFloat def) const
{
  SQFloat ret = def;
  if(GetSlot(key)) {
    sq_getfloat(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

SQInteger SquirrelObject::GetInt(const SQChar *key, SQInteger def) const
{
  SQInteger ret = def;
  if(GetSlot(key)) {
    sq_getinteger(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

const SQChar *SquirrelObject::GetString(const SQChar *key, const SQChar *def) const
{
  const SQChar *ret = def;
  if(GetSlot(key)) {
    sq_getstring(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret;
}

bool SquirrelObject::GetBool(const SQChar *key, bool def) const
{
  SQBool ret = def;
  if(GetSlot(key)) {
    sq_tobool(SquirrelVM::_VM,-1,&ret);
    sq_pop(SquirrelVM::_VM,1);
  }
  sq_pop(SquirrelVM::_VM,1);
  return ret?true:false;
}

SQUserPointer SquirrelObject::GetInstanceUP(SQUserPointer tag) const
{
  SQUserPointer up;
  sq_pushobject(SquirrelVM::_VM,_o);
  if (SQ_FAILED(sq_getinstanceup(SquirrelVM::_VM,-1,(SQUserPointer*)&up,tag))) {
    sq_reseterror(SquirrelVM::_VM);
    up = NULL;
  } // if
  sq_pop(SquirrelVM::_VM,1);
  return up;
}

SQBool SquirrelObject::SetInstanceUP(SQUserPointer up)
{
  if(!sq_isinstance(_o)) return SQFalse;
  sq_pushobject(SquirrelVM::_VM,_o);
  sq_setinstanceup(SquirrelVM::_VM,-1,up);
  sq_pop(SquirrelVM::_VM,1);
  return SQTrue;
}

SQBool SquirrelObject::BeginIteration()
{
  if(!sq_istable(_o) && !sq_isarray(_o) && !sq_isclass(_o))
    return SQFalse;
  sq_pushobject(SquirrelVM::_VM,_o);
  sq_pushnull(SquirrelVM::_VM);
  return SQTrue;
}

SQBool SquirrelObject::Next(SquirrelObject &key,SquirrelObject &val)
{
  if(SQ_SUCCEEDED(sq_next(SquirrelVM::_VM,-2))) {
    key.AttachToStackObject(-2);
    val.AttachToStackObject(-1);
    sq_pop(SquirrelVM::_VM,2);
    return SQTrue;
  }
  return SQFalse;
}

SQBool SquirrelObject::GetTypeTag(SQUserPointer * typeTag) {
  if (SQ_SUCCEEDED(sq_getobjtypetag(&_o,typeTag))) {
    return SQTrue;
  } // if
  return SQFalse;
} // SquirrelObject::GetTypeTag

const SQChar * SquirrelObject::GetTypeName(const SQChar * key) {
  // This version will work even if SQ_SUPPORT_INSTANCE_TYPE_INFO is not enabled.
  SqPlus::ScriptStringVar256 varNameTag;
  SqPlus::getVarNameTag(varNameTag,sizeof(varNameTag),key);
  SQUserPointer data=0;
  if (!RawGetUserData(varNameTag,&data)) {
    return NULL;
  } // if
  SqPlus::VarRefPtr vr = (SqPlus::VarRefPtr)data;
  return vr->typeName;
} // SquirrelObject::GetTypeName

const SQChar* SquirrelObject::ToString()
{
  return sq_objtostring(&_o);
}

SQInteger SquirrelObject::ToInteger()
{
  return sq_objtointeger(&_o);
}

SQFloat SquirrelObject::ToFloat()
{
  return sq_objtofloat(&_o);
}

bool SquirrelObject::ToBool()
{
//  //<<FIXME>>
//  return _o._unVal.nInteger?true:false;

  //== copy from SQVM::IsFalse
  if (((_o._type & SQOBJECT_CANBEFALSE) && (_o._type == OT_FLOAT) && (_o._unVal.fFloat == SQFloat(0.0)))
    || (_o._unVal.nInteger==0) ) { //OT_NULL|OT_INTEGER|OT_BOOL
    return false;
  }
  return true;
}

void SquirrelObject::EndIteration()
{
  sq_pop(SquirrelVM::_VM,2);
}

