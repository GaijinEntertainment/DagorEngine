#include <stdio.h>
#include <stdarg.h>

#define _DEBUG_DUMP

#include "sqplus.h"

#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>


SQ_THREAD_LOCAL(HSQUIRRELVM) SquirrelVM::_VM = NULL;
SQ_THREAD_LOCAL(SquirrelObject *) SquirrelVM::_root = NULL;
SQ_THREAD_LOCAL(SquirrelObject *) SquirrelVM::_nativeClassesTable = NULL;
HSQUIRRELVM * SquirrelVM::ptr_to_hsqvm = NULL;


SquirrelError::SquirrelError() : DagorException(1, NULL)
{
  const SQChar *s;
  sq_getlasterror(SquirrelVM::_VM);
  sq_getstring(SquirrelVM::_VM,-1,&s);
  if(s) {
    desc = s;
  }
  else {
    desc = _SC("unknown error");
  }

  excDesc=desc;
}


void SquirrelVM::Init(unsigned flags)
{
  ptr_to_hsqvm = &_VM;
  _VM = sq_open(1280);
  sq_setprintfunc(_VM,SquirrelVM::PrintFunc,SquirrelVM::PrintFunc);
  sq_pushroottable(_VM);
  if ((flags & SquirrelVM::SF_IGNORE_DEFAULT_LIBRARIES) == 0)
  {
    sqstd_register_mathlib(_VM);
    sqstd_register_stringlib(_VM);
  }
  sqstd_seterrorhandlers(_VM);
  _root = new SquirrelObject();
  _root->AttachToStackObject(-1);

  _nativeClassesTable = new SquirrelObject(CreateTable());

  sq_pop(_VM,1);
  (void)(flags);
  //TODO error handler, compiler error handler
}

SQBool SquirrelVM::Update()
{
  //update remote debugger
  return SQTrue;
}

void SquirrelVM::Cleanup()
{
  //cleans the root table
  sq_pushnull(_VM);
  sq_setroottable(_VM);
}

void SquirrelVM::Shutdown(bool sh_dbg)
{
  if (_VM) {
    Cleanup();
#if 0
    sq_release(_VM,&_root->_o);
    sq_resetobject(&_root->_o);
#endif
    delete _root;
    _root = NULL;

    delete _nativeClassesTable;
    _nativeClassesTable = NULL;
    (void)(sh_dbg);

    HSQUIRRELVM v = _VM;
    _VM = NULL;
    sq_close(v);
  } // if
}

void SquirrelVM::PrintFunc(HSQUIRRELVM /*v*/,const SQChar* s,...)
{
  static SQChar temp[2048];
  va_list vl;
  va_start(vl, s);
  vsnprintf( temp, sizeof(temp)/sizeof(temp[0]), s, vl);
  puts(temp);
  va_end(vl);
}

bool SquirrelVM::CompileBuffer(const SQChar* s,SquirrelObject& outObject,
  HSQOBJECT *bindings, const SQChar* sourceName)
{
  if(SQ_SUCCEEDED(sq_compile(_VM,s,strlen(s),sourceName,SQTrue,bindings))) {
    outObject.AttachToStackObject(-1);
    sq_pop(_VM,1);
    return true;
  }
  return false;
}

bool SquirrelVM::RunScript(const SquirrelObject &o,SquirrelObject& outObject,
  SquirrelObject *_this)
{
  sq_pushobject(_VM,o._o);
  if(_this) {
    sq_pushobject(_VM,_this->_o);
  }
  else {
    sq_pushroottable(_VM);
  }

  if(SQ_SUCCEEDED(sq_call(_VM,1,SQTrue,SQ_CALL_RAISE_ERROR))) {
    outObject.AttachToStackObject(-1);
    sq_pop(_VM,2);
    return true;
  }
  sq_pop(_VM,1);
  return false;
}

SquirrelObject SquirrelVM::CreateInstance(SquirrelObject &oclass)
{
  SquirrelObject ret;
  int oldtop = sq_gettop(_VM);
  sq_pushobject(_VM,oclass._o);
  if(SQ_FAILED(sq_createinstance(_VM,-1))) {
    sq_settop(_VM,oldtop);
    DAGOR_THROW ( SquirrelError());
  }
  ret.AttachToStackObject(-1);
  sq_pop(_VM,2);
  return ret;
}

SquirrelObject SquirrelVM::CreateTable()
{
  SquirrelObject ret;
  sq_newtable(_VM);
  ret.AttachToStackObject(-1);
  sq_pop(_VM,1);
  return ret;
}

SquirrelObject SquirrelVM::CreateArray(int size)
{
  SquirrelObject ret;
  sq_newarray(_VM,size);
  ret.AttachToStackObject(-1);
  sq_pop(_VM,1);
  return ret;
}

SquirrelObject SquirrelVM::CreateFunction(SQFUNCTION func)
{
  SquirrelObject ret;
  sq_newclosure(_VM,func,0);
  ret.AttachToStackObject(-1);
  sq_pop(_VM,1);
  return ret;
}

SquirrelObject SquirrelVM::CreateUserData(int size) {
  SquirrelObject ret;
  sq_newuserdata(_VM,size);
  ret.AttachToStackObject(-1);
  sq_pop(_VM,1);
  return ret;
}

const SquirrelObject &SquirrelVM::GetRootTable()
{
  return *_root;
}

SquirrelObject &SquirrelVM::GetNativeClassesTable()
{
  return *_nativeClassesTable;
}

void SquirrelVM::PushRootTable(void) {
  sq_pushroottable(_VM);
} // SquirrelVM::PushRootTable

// Creates a function in the table or class currently on the stack.
//void CreateFunction(HSQUIRRELVM v,const SQChar * scriptFuncName,SQFUNCTION func,int numParams=0,const SQChar * typeMask=0) {
SquirrelObject SquirrelVM::CreateFunction(SQFUNCTION func,const SQChar * scriptFuncName,
                                          const SQChar * typeMask, SQInteger numParams) {
  sq_pushstring(_VM,scriptFuncName,-1);
  sq_newclosure(_VM,func,0);
  SquirrelObject ret;
  ret.AttachToStackObject(-1);
  SQChar tm[64];
  SQChar * ptm = tm;
  if (typeMask) {
    if (typeMask[0] == '*') {
      ptm       = 0; // Variable args: don't check parameters.
      numParams = 0; // Disable params count check
    } else {
      if (scsprintf(tm,sizeof(tm),_SC("t|y|x%s"),typeMask) < 0) {
        DAGOR_THROW ( SquirrelError(_SC("CreateFunction: typeMask string too long.")));
      }
    }
  } else { // <TODO> Need to check object type on stack: table, class, instance, etc.
    scsprintf(tm,sizeof(tm),_SC("%s"),_SC("t|y|x")); // table, class, instance.
  }
  tm[sizeof(tm)-1] = 0;
  if (ptm)
    sq_setparamscheck(_VM,numParams,ptm);
#if DAGOR_DBGLEVEL > 0
  sq_setnativeclosurename(_VM,-1,scriptFuncName); // For debugging only.
#endif
  sq_newslot(_VM,-3,SQFalse); // Create slot in table or class (assigning function to slot at scriptNameFunc).
  return ret;
}

SquirrelObject SquirrelVM::CreateFunction(SquirrelObject & so,SQFUNCTION func,const SQChar * scriptFuncName,const SQChar * typeMask) {
  PushObject(so);
  SquirrelObject ret = CreateFunction(func,scriptFuncName,typeMask);
  Pop(1);
  return ret;
} // SquirrelVM::CreateFunction

// Create a Global function on the root table.
//void CreateFunctionGlobal(HSQUIRRELVM v,const SQChar * scriptFuncName,SQFUNCTION func,int numParams=0,const SQChar * typeMask=0) {
SquirrelObject SquirrelVM::CreateFunctionGlobal(SQFUNCTION func,const SQChar * scriptFuncName,const SQChar * typeMask) {
  PushRootTable(); // Push root table.
  //  CreateFunction(scriptFuncName,func,numParams,typeMask);
  SquirrelObject ret = CreateFunction(func,scriptFuncName,typeMask);
  Pop(1);         // Pop root table.
  return ret;
} // SquirrelVM::CreateFunctionGlobal
