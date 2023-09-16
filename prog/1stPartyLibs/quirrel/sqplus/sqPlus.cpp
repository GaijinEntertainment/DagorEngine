#include "sqplus.h"
#include <stdio.h>
#include <sqstdaux.h>

// for SQInstance decl & _instance macros
#include <util/dag_globDef.h>
#define assert G_ASSERT
#include <new>
#include <../squirrel/sqstate.h>
#include <../squirrel/sqvm.h> // for stack_get function
#include <../squirrel/sqtable.h>
#include <../squirrel/sqclass.h>


namespace SqPlus {

SQUserPointer sqplus_rawgetinstanceup(HSQUIRRELVM v,SQInteger idx)
{
  return _instance(stack_get(v,idx))->_userpointer;
}

static int getVarInfo(StackHandler & sa,VarRefPtr & vr) {
  HSQOBJECT htable = sa.GetObjectHandle(1);
  SquirrelObject table(htable);
  const SQChar * el = sa.GetString(2);
  ScriptStringVar256 varNameTag;
  getVarNameTag(varNameTag,sizeof(varNameTag),el);
  SQUserPointer data=0;
  if (!table.RawGetUserData(varNameTag,&data)) {
    sq_pushnull(sa.GetVMPtr());
    return sq_throwobject(sa.GetVMPtr());
  } // if
  vr = (VarRefPtr)data;
  return SQ_OK;
} // getVarInfo

static int getInstanceVarInfo(StackHandler & sa,VarRefPtr & vr,SQUserPointer & data) {
  HSQOBJECT ho = sa.GetObjectHandle(1);
  SquirrelObject instance(ho);
  const SQChar * el = sa.GetString(2);
  ScriptStringVar256 varNameTag;
  getVarNameTag(varNameTag,sizeof(varNameTag),el);
  SQUserPointer ivrData=0;
  if (!instance.RawGetUserData(varNameTag,&ivrData)) {
//    throw SquirrelError("getInstanceVarInfo: Could not retrieve UserData");

    sq_pushnull(sa.GetVMPtr());
    return sq_throwobject(sa.GetVMPtr());
  } // if
  vr = (VarRefPtr)ivrData;
  unsigned char * up;
  if (!(vr->access & (VAR_ACCESS_STATIC|VAR_ACCESS_CONSTANT))) {
    up = (unsigned char *)instance.GetInstanceUP(0);
    up += (size_t)vr->offsetOrAddrOrConst;
  } else {
    up = (unsigned char *)vr->offsetOrAddrOrConst; // Address
  } // if
  data = up;
  return SQ_OK;
} // getInstanceVarInfo


static int setVar(StackHandler & sa,VarRef * vr,void * data) {
  if (vr->access & (VAR_ACCESS_READ_ONLY|VAR_ACCESS_CONSTANT)) {
    return sqstd_throwerrorf(sa.GetVMPtr(), _SC("setVar(): Cannot write to constant: %s"), sa.GetString(2));
  }

  switch (vr->type) { //-V556
  case TypeInfo<int32_t>::TypeID: { //-V556
    int32_t * val = (int32_t *)data; // Address
    if (val) {
      *val = sa.GetInt(3);
      return sa.Return(SQInteger(*val));
    }
    break;
  } // case
  case TypeInfo<int64_t>::TypeID: { //-V556
    int64_t * val = (int64_t *)data; // Address
    if (val) {
      *val = sa.GetInt(3);
      return sa.Return(SQInteger(*val));
    }
    break;
  } // case
  case TypeInfo<SQFloat>::TypeID: { //-V556
    SQFloat * val = (SQFloat *)data; // Address
    if (val) {
      *val = sa.GetFloat(3);
      return sa.Return(*val);
    }
    break;
  } // case
  case TypeInfo<bool>::TypeID: { //-V556
    bool * val = (bool *)data; // Address
    if (val) {
      *val = sa.GetBool(3) ? true : false;
      return sa.Return(*val);
    }
    break;
  } // case
  case VAR_TYPE_INSTANCE: { //-V556
    HSQUIRRELVM v = sa.GetVMPtr();
    // vr->copyFunc is the LHS variable type: the RHS var's type is ClassType<>::type() (both point to ClassType<>::copy()).
    // src will be null if the LHS and RHS types don't match.
    SQUserPointer src = sa.GetInstanceUp(3, vr->varType); // Effectively performs: ClassType<>::type() == ClassType<>getCopyFunc().
    if (!src)
      return sq_throwerror(v, _SC("INSTANCE type assignment mismatch"));
    vr->copyFunc(data,src);
    return 0;
  }
  case TypeInfo<SQUserPointer>::TypeID: { //-V556
    return sqstd_throwerrorf(sa.GetVMPtr(), _SC("setVar(): Cannot write to an SQUserPointer: %s"), sa.GetString(2));
  } // case
  case TypeInfo<ScriptStringVarBase>::TypeID: { //-V556
    ScriptStringVarBase * val = (ScriptStringVarBase *)data; // Address
    if (val) {
      const SQChar * strVal = sa.GetString(3);
      if (strVal) {
        *val = strVal;
        return sa.Return(val->s);
      }
    }
    break;
  } // case
  default: break;
  } // switch
  return sq_throwerror(sa.GetVMPtr(), _SC("setVar(): internal error"));
} // setVar


static int getVar(StackHandler & sa,VarRef * vr,void * data) {
  switch (vr->type) { //-V556
  case TypeInfo<int32_t>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      int32_t *val = (int32_t *)data; // Address
      if (val)
        return sa.Return(SQInteger(*val));
    } else {
      int32_t * val = (int32_t *)&data; // Constant value
      return sa.Return(SQInteger(*val));
    }
    break;
  } // case
  case TypeInfo<int64_t>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      int64_t *val = (int64_t *)data; // Address
      if (val)
        return sa.Return(SQInteger(*val));
    } else {
      int64_t * val = (int64_t *)&data; // Constant value
      return sa.Return(SQInteger(*val));
    }
    break;
  } // case
  case TypeInfo<SQFloat>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      SQFloat * val = (SQFloat *)data; // Address
      if (val)
        return sa.Return(*val);
    } else {
      SQFloat * val = (SQFloat *)&data; // Constant value
      return sa.Return(*val);
    }
    break;
  } // case
  case TypeInfo<bool>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      bool * val = (bool *)data; // Address
      if (val)
        return sa.Return(*val);
    } else {
      bool * val = (bool *)&data; // Constant value
      return sa.Return(*val);
    }
    break;
  } // case
  case VAR_TYPE_INSTANCE: //-V556
    if (!CreateNativeClassInstance(sa.GetVMPtr(),vr->typeName,data,0)) { // data = address. Allocates memory.
      return sqstd_throwerrorf(sa.GetVMPtr(), _SC("getVar(): Could not create instance: %s"), vr->typeName);
    }
    return 1;
  case TypeInfo<SQUserPointer>::TypeID: { //-V556
    return sa.Return(data); // The address of member variable, not the variable itself.
  } // case
  case TypeInfo<ScriptStringVarBase>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      ScriptStringVarBase * val = (ScriptStringVarBase *)data; // Address
      if (val)
        return sa.Return(val->s);
    } else {
      return sq_throwerror(sa.GetVMPtr(), _SC("getVar(): Invalid type+access: 'ScriptStringVarBase' with VAR_ACCESS_CONSTANT (use VAR_ACCESS_READ_ONLY instead)"));
    }
    break;
  } // case
  case TypeInfo<const SQChar *>::TypeID: { //-V556
    if (!(vr->access & VAR_ACCESS_CONSTANT)) {
      return sq_throwerror(sa.GetVMPtr(), _SC("getVar(): Invalid type+access: 'const SQChar *' without VAR_ACCESS_CONSTANT"));
    } else {
      return sa.Return((const SQChar *)data); // Address
    } // if
    break;
  } // case
  default: break;
  } // switch

  return sq_throwerror(sa.GetVMPtr(), _SC("getVar(): internal error"));
} // getVar

// === Instance Vars ===

SQInteger setInstanceVarFunc(HSQUIRRELVM v) {
  StackHandler sa(v);
  if (sa.GetType(1) == OT_INSTANCE) {
    VarRefPtr vr;
    void * data;
    int res = getInstanceVarInfo(sa,vr,data);
    if (res != SQ_OK) return res;
    return setVar(sa,vr,data);
  }
  return sq_throwerror(v, _SC("setInstanceVarFunc(): not an instance"));
}

SQInteger getInstanceVarFunc(HSQUIRRELVM v) {
  StackHandler sa(v);
  if (sa.GetType(1) == OT_INSTANCE) {
    VarRefPtr vr;
    void * data;
    int res = getInstanceVarInfo(sa,vr,data);
    if (res != SQ_OK) return res;
    return getVar(sa,vr,data);
  }
  return sq_throwerror(v, _SC("getInstanceVarFunc(): not an instance"));
}

// === Classes ===

SQBool CreateClass(HSQUIRRELVM v,SquirrelObject & newClass,SQUserPointer classType,const SQChar * name,const SQChar * baseName) {
  int n = 0;
  int oldtop = sq_gettop(v);
  sq_pushroottable(v);
  sq_pushstring(v,name,-1);
  if (baseName) {
    sq_pushstring(v,baseName,-1);
    if (SQ_FAILED(sq_get(v,-3))) { // Make sure the base exists if specified by baseName.
      sq_settop(v,oldtop);
      return SQFalse;
    } // if
  } // if
  if (SQ_FAILED(sq_newclass(v,baseName ? 1 : 0))) { // Will inherit from base class on stack from sq_get() above.
    sq_settop(v,oldtop);
    return SQFalse;
  } // if
  newClass.AttachToStackObject(-1);
  sq_settypetag(v,-1,classType);
  sq_newslot(v,-3,SQFalse);
  sq_pop(v,1);
  return SQTrue;
} // CreateClass


SQBool CreateNativeClassInstance(HSQUIRRELVM v,const SQChar *classname,SQUserPointer ud,SQRELEASEHOOK hook)
{
  int oldtop = sq_gettop(v);
  sq_pushroottable(v);
  sq_pushstring(v,classname,-1);
  if(SQ_FAILED(sq_rawget(v,-2))){ // Get the class (created with sq_newclass()).
    sq_settop(v,oldtop);
    return SQFalse;
  }
  //sq_pushroottable(v);
  if(SQ_FAILED(sq_createinstance(v,-1))) {
    sq_settop(v,oldtop);
    return SQFalse;
  }
  sq_remove(v,-3); //removes the root table
  sq_remove(v,-2); //removes the class
  if(SQ_FAILED(sq_setinstanceup(v,-1,ud))) {
    sq_settop(v,oldtop);
    return SQFalse;
  }
  sq_setreleasehook(v,-1,hook);
  return SQTrue;
}

}; // namespace SqPlus

// sqPlus
