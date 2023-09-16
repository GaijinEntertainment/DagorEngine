#ifndef _SQUIRREL_VM_H_
#define _SQUIRREL_VM_H_


#include <debug/dag_except.h>

struct SquirrelError : public DagorException
{
  SquirrelError();
  SquirrelError(const SQChar* s) : DagorException(1, s), desc(s) {}

  const SQChar *desc;
};


struct SquirrelVMSys {
  HSQUIRRELVM _VM;
  SquirrelObject * _root;
  SquirrelObject *_nativeClassesTable;
};

class SquirrelVM
{
  friend class SquirrelObject;
  friend struct SquirrelError;
public:
  enum
  {
    // now unused  SF_ENABLE_DEBUGGER = 1,
    // now unused  SF_WAIT_FOR_CONNECTIONS = 2,
    // now unused  SF_DEBUGGER_AUTO_UPDATE = 4,
    SF_IGNORE_DEFAULT_LIBRARIES = 8,
  };

  static void Init(unsigned flags = 0);
  static SQBool IsInitialized(){return _VM == NULL?SQFalse:SQTrue;}
  static void Shutdown(bool shutdown_debugger = false);
  static void Cleanup();
  static SQBool Update(); //debugger and maybe GC later
  static bool CompileScript(const SQChar *s, SquirrelObject& outObject);
  static bool CompileBuffer(const SQChar *s, SquirrelObject& outObject,
    HSQOBJECT *bindings=NULL, const SQChar *sourceName=_SC("console_buffer"));
  static bool RunScript(const SquirrelObject &o, SquirrelObject& outObject,
    SquirrelObject *_this = NULL);
  static void PrintFunc(HSQUIRRELVM v,const SQChar* s,...);
  static SquirrelObject CreateTable();
  static SquirrelObject CreateArray(int size);
  static SquirrelObject CreateInstance(SquirrelObject &oclass); // oclass is an existing class. Create an 'instance' (OT_INSTANCE) of oclass.
  static SquirrelObject CreateFunction(SQFUNCTION func);
  static SquirrelObject CreateUserData(int size);

  static const SquirrelObject &GetRootTable();
  static SquirrelObject &GetNativeClassesTable();
  static HSQUIRRELVM GetVMPtr() { return _VM; }
  static HSQUIRRELVM GetVMPtrFromAnyThread() { return ptr_to_hsqvm ? *ptr_to_hsqvm : NULL; } // only for debug purpose

  static void GetVMSys(SquirrelVMSys & vmSys) {
    vmSys._VM   = _VM;
    vmSys._root = _root;
    vmSys._nativeClassesTable = _nativeClassesTable;
  } // GetVMSys

  static void SetVMSys(const SquirrelVMSys & vmSys) {
    _VM   = vmSys._VM;
    _root = vmSys._root;
    _nativeClassesTable = vmSys._nativeClassesTable;
  } // SetVMSys

  static void PushObject(SquirrelObject & so) {
    sq_pushobject(_VM,so._o);
  } // PushObject
  static void Pop(SQInteger nelemstopop) {
    sq_pop(_VM,nelemstopop);
  } // Pop
  static void PushRootTable(void);
  // Create/bind a function on the table currently on the stack.
  static SquirrelObject CreateFunction(SQFUNCTION func,const SQChar * scriptFuncName,const SQChar * typeMask=0, SQInteger numParams = SQ_MATCHTYPEMASKSTRING);
  // Create/bind a function on the table so. typeMask: standard Squirrel types plus: no typemask means no args, "*" means any type of args.
  static SquirrelObject CreateFunction(SquirrelObject & so,SQFUNCTION func,const SQChar * scriptFuncName,const SQChar * typeMask=0);
  // Create/bind a function to the root table. typeMask: standard Squirrel types plus: no typemask means no args, "*" means any type of args.
  static SquirrelObject CreateFunctionGlobal(SQFUNCTION func,const SQChar * scriptFuncName,const SQChar * typeMask=0);

private:

#if defined(_TARGET_APPLE) || !defined(_TARGET_STATIC_LIB) || DAGOR_DBGLEVEL <= 0
  #define SQ_THREAD_LOCAL(T) T
#else
#ifdef _MSC_VER
  #define SQ_THREAD_LOCAL(T) __declspec(thread) T
#else
  #define SQ_THREAD_LOCAL(T) __thread T
#endif
#endif

  static HSQUIRRELVM * ptr_to_hsqvm;
  static SQ_THREAD_LOCAL(HSQUIRRELVM) _VM;
  static SQ_THREAD_LOCAL(SquirrelObject *) _root;
  static SQ_THREAD_LOCAL(SquirrelObject *) _nativeClassesTable;
};

#endif //_SQUIRREL_VM_H_

