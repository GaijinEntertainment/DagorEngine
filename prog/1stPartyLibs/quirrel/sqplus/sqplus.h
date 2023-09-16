// SqPlus.h
// Created by John Schultz 9/05/05, major update 10/05/05.
// Template function call design from LuaPlusCD by Joshua C. Jensen,
// inspired by luabind which was inspired by boost::python.
// Const argument, const member functions, and Mac OS-X changes by Simon Michelmore.
// DECLARE_INSTANCE_TYPE_NAME changes by Ben (Project5) from http://www.squirrel-lang.org/forums/.
// Added Kamaitati's changes 5/28/06.
// Free for any use.

#ifndef _SQ_PLUS_H_
#define _SQ_PLUS_H_
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if 1
#define SQ_CALL_RAISE_ERROR SQTrue
#else
#define SQ_CALL_RAISE_ERROR SQFalse
#endif

#include "squirrel.h"

#include "squirrelObject.h"
#include "squirrelVM.h"
#include "dag_sqAux.h"

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wnull-dereference"
#endif


namespace SqPlus {

SQUserPointer sqplus_rawgetinstanceup(HSQUIRRELVM v,SQInteger idx);

// === Class Inheritance Support ===
// Inheritance in Squirrel allows one class to inherit a base class's functions and variables.
// Variables are merged: if Derived has a var name 'val' and Base has a var of the same name,
// the resulting var 'val' will take Derived's initialization value.
// Functions are not merged, and can be called via Squirrel scoping rules.

// === Constant argument and constant member function support ===
// Define SQPLUS_CONST_OPT before including SqPlus.h for constant argument + constant member function support.
#define SQPLUS_CONST_OPT

// === Uncomment to skip sq_argassert() ===
//#define SQ_SKIP_ARG_ASSERT

template<class T> struct TypeWrapper {};
struct SquirrelNull {};
struct SQAnything { void * anything; }; // Needed for binding pointers to variables (cannot dereference void *).
typedef SQAnything * SQAnythingPtr;
typedef SQChar * SQCharPtr;

// === Push helpers (implemented via templates-functors to force ADL)
template<typename T> struct functor;
template<typename T> inline void Push(HSQUIRRELVM v,T value) { functor<T>::push(v, value); }

inline void Push(HSQUIRRELVM v,bool value)           { sq_pushbool(v,value); }
inline void Push(HSQUIRRELVM v,signed char value)    { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,unsigned char value)  { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,short value)          { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,unsigned short value) { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,SQInteger value)      { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,SQUnsignedInteger value) { sq_pushinteger(v,value); }
#ifdef _SQ64
inline void Push(HSQUIRRELVM v,SQInt32 value)            { sq_pushinteger(v,value); }
inline void Push(HSQUIRRELVM v,unsigned int value)   { sq_pushinteger(v,value); }
#endif
inline void Push(HSQUIRRELVM v,double value)         { sq_pushfloat(v,(SQFloat)value); }
inline void Push(HSQUIRRELVM v,float value)          { sq_pushfloat(v,(SQFloat)value); }
inline void Push(HSQUIRRELVM v,const SQChar * value) { sq_pushstring(v,value,-1); }
inline void Push(HSQUIRRELVM v,      SQChar * value) { sq_pushstring(v,value,-1); }
inline void Push(HSQUIRRELVM v,const SquirrelNull &) { sq_pushnull(v); }
inline void Push(HSQUIRRELVM v,SQFUNCTION value)     { sq_pushuserpointer(v,(void*)value); }
inline void Push(HSQUIRRELVM v,SQAnythingPtr value)  { sq_pushuserpointer(v,(void*)value); }
inline void Push(HSQUIRRELVM v,SquirrelObject & so)  { sq_pushobject(v,so.GetObjectHandle()); }

// === Do not use directly: use one of the predefined sizes below ===

struct ScriptStringVarBase {
  const unsigned char MaxLength; // Real length is MaxLength+1.
  SQChar s[1];
  ScriptStringVarBase(int _MaxLength) : MaxLength(_MaxLength) {}
  operator SQChar * () { return &s[0]; }
  operator void * () { return (void *)&s[0]; }
  const SQChar * operator = (const SQChar * _s) {
    return safeStringCopy(s,_s,MaxLength);
  }
  // Special safe string copy where MaxLength is 1 less than true buffer length.
  // strncpy() pads out nulls for the full length of the buffer specified by MaxLength.
  static inline SQChar * safeStringCopy(SQChar * d,const SQChar * s,int MaxLength) {
    int i=0;
    while (s[i]) {
      d[i] = s[i];
      i++;
      if (i == MaxLength) break;
    } // while
    d[i] = 0; // Null terminate.
    return d;
  } // safeStringCopy
private:
  ScriptStringVarBase(const ScriptStringVarBase &);
  ScriptStringVarBase& operator = (const ScriptStringVarBase &);
};

// === Do not use directly: use one of the predefined sizes below ===

template<int MAXLENGTH> // MAXLENGTH is max printable characters (trailing NULL is accounted for in ScriptStringVarBase::s[1]).
struct ScriptStringVar : ScriptStringVarBase {
  SQChar ps[MAXLENGTH];
  ScriptStringVar() : ScriptStringVarBase(MAXLENGTH) {
    s[0] = 0;
  }
  ScriptStringVar(const SQChar * _s) : ScriptStringVarBase(MAXLENGTH) {
    *this = _s;
  }
  const SQChar * operator = (const SQChar * _s) {
    return safeStringCopy(s,_s,MaxLength);
  }
  const SQChar * operator = (const ScriptStringVar & _s) {
    return safeStringCopy(s,_s.s,MaxLength);
  }
  bool operator == (const ScriptStringVar & _s) {
    return strcmp(s,_s.s) == 0;
  }
  bool compareCaseInsensitive(const ScriptStringVar & _s) {
    return stricmp(s,_s.s) == 0;
  }
};

// === Fixed size strings for scripting ===

typedef ScriptStringVar<256> ScriptStringVar256;

// === Script Variable Types ===

enum ScriptVarType {VAR_TYPE_NONE=-1,VAR_TYPE_INT32=0,VAR_TYPE_INT64,VAR_TYPE_FLOAT,VAR_TYPE_BOOL,VAR_TYPE_CONST_STRING,VAR_TYPE_STRING,VAR_TYPE_USER_POINTER,VAR_TYPE_INSTANCE};

template <typename T>
struct TypeInfo {
  const SQChar * typeName;
  enum {TypeID=VAR_TYPE_NONE,Size=0};
};

// === Common Variable Types ===

template<>
struct TypeInfo<int32_t> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("int32")) {}
  enum {TypeID=VAR_TYPE_INT32,Size=sizeof(int32_t)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<int64_t> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("int64")) {}
  enum {TypeID=VAR_TYPE_INT64,Size=sizeof(int64_t)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<SQFloat> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("float")) {}
  enum {TypeID=VAR_TYPE_FLOAT,Size=sizeof(SQFloat)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<bool> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("bool")) {}
  enum {TypeID=VAR_TYPE_BOOL,Size=sizeof(bool)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<SQUserPointer> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("SQUserPointer")) {}
  enum {TypeID=VAR_TYPE_USER_POINTER,Size=sizeof(SQUserPointer)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<SQAnything> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("SQUserPointer")) {}
  enum {TypeID=VAR_TYPE_USER_POINTER,Size=sizeof(SQUserPointer)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<const SQChar *> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("const SQChar *")) {}
  enum {TypeID=VAR_TYPE_CONST_STRING,Size=sizeof(const SQChar *)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

template<>
struct TypeInfo<ScriptStringVarBase> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("ScriptStringVarBase")) {}
  enum {TypeID=VAR_TYPE_STRING,Size=sizeof(ScriptStringVarBase)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

// === Fixed String Variants ===

template<>
struct TypeInfo<ScriptStringVar256> {
  const SQChar * typeName;
  TypeInfo() : typeName(_SC("ScriptStringVar256")) {}
  enum {TypeID=VAR_TYPE_STRING,Size=sizeof(ScriptStringVar256)};
  operator ScriptVarType() { return ScriptVarType(TypeID); }
};

enum VarAccessType {VAR_ACCESS_READ_WRITE=0,VAR_ACCESS_READ_ONLY=1<<0,VAR_ACCESS_CONSTANT=1<<1,VAR_ACCESS_STATIC=1<<2};

// See VarRef and ClassType<> below: for instance assignment.
typedef void (*CopyVarFunc)(void * dst,void * src);

// === Variable references for script access ===

struct VarRef {
  // In this case 'offsetOrAddrOrConst' is simpler than using an anonymous union.
  void * offsetOrAddrOrConst; // Instance member variable offset from 'this' pointer base (as size_t), or address if static variable (void *), or constant value.
  SQUserPointer instanceType; // Unique ID for the containing class instance (for instance vars only). When the var is an instance, its type is encoded in varType.
  SQUserPointer varType;      // Unique ID for variable instance.
  CopyVarFunc copyFunc;       // Function pointer to copy variables (for instance variables only).
  const SQChar * typeName;    // Type name string (to create instances by name).
  ScriptVarType type;         // Variable type (from enum above).
  short size;                 // Currently for debugging only (size of item when pointer to item is dereferenced). Could be used for variable max string buffer length.
  short access;               // VarAccessType.

  VarRef()
    : offsetOrAddrOrConst(0), type(VAR_TYPE_NONE), instanceType((SQUserPointer)-1), varType((SQUserPointer)-1),
    copyFunc(0), size(0), access(VAR_ACCESS_READ_WRITE)
  {
  }

  VarRef(void * _offsetOrAddrOrConst, ScriptVarType _type, SQUserPointer _instanceType, SQUserPointer _varType,
    CopyVarFunc _copyFunc, int _size,VarAccessType _access,const SQChar * _typeName)
    :offsetOrAddrOrConst(_offsetOrAddrOrConst), type(_type), instanceType(_instanceType), varType(_varType),
    copyFunc(_copyFunc), size(_size), access(_access), typeName(_typeName)
  {
  }
};

typedef VarRef * VarRefPtr;

// Internal use only.
inline void getVarNameTag(SQChar * buff,SQInteger maxSize,const SQChar * scriptName) {
//  assert(maxSize > 3);
#if 1
  SQChar * d = buff;
  d[0] = '_';
  d[1] = 'v';
  d = &d[2];
  maxSize -= (2+1); // +1 = space for null.
  int pos=0;
  while (scriptName[pos] && pos < maxSize) {
    d[pos] = scriptName[pos];
    pos++;
  } // while
  d[pos] = 0; // null terminate.
#else
  scsprintf(buff,maxSize,_SC("_v%s"),scriptName);
#endif
} // getVarNameTag

// Internal use only.
SQInteger setInstanceVarFunc(HSQUIRRELVM v);
SQInteger getInstanceVarFunc(HSQUIRRELVM v);

// === BEGIN Helpers ===

inline VarRefPtr createVarRef(SquirrelObject & so,const SQChar * scriptVarName) {
  SQUserPointer sqp = 0;
  ScriptStringVar256 scriptVarTagName; getVarNameTag(scriptVarTagName,sizeof(scriptVarTagName),scriptVarName);
  if (!so.GetUserData(scriptVarTagName, &sqp)) {
    so.NewUserData(scriptVarTagName,sizeof(VarRef));
    if (!so.GetUserData(scriptVarTagName,&sqp)) DAGOR_THROW ( SquirrelError(_SC("Could not create UserData.")));
  } // if
  return reinterpret_cast<VarRefPtr>(sqp);
} // createVarRef


inline void createInstanceSetGetHandlers(SquirrelObject & so) {
  if (!so.Exists(_SC("_set"))) {
    SquirrelVM::CreateFunction(so,setInstanceVarFunc,_SC("_set"),_SC("sn|b|s|x")); // String var name = number(int or float) or bool or string or instance.
    SquirrelVM::CreateFunction(so,getInstanceVarFunc,_SC("_get"),_SC("s"));      // String var name.
  } // if
} // createInstanceSetGetHandlers

// === END Helpers ===

// === Class Type Helper class: returns a unique number for each class type ===

template<typename T>
struct ClassType {
  static SQUserPointer type(void)       { return (SQUserPointer)&uniqueId; }
  static CopyVarFunc getCopyFunc(void)  { return (CopyVarFunc)&copy; }
  static void copy(T * dst,T * src)     { *dst = *src; }
  static int uniqueId;
};

template <class T> int ClassType<T>::uniqueId;


// === Bind a constant by value: SQInteger, SQFloat, SQBool, or CONST CHAR * (for tables only (not classes)) ===

// Cast Variable helper
#define SQ_CV_TYPE(T, T2, var_)\
  struct CV\
  {\
    union\
    {\
      T var;\
      T2 var2;\
      void* pVoid;\
    };\
  } cv;\
  if (sizeof(T) < sizeof(void*))\
    cv.pVoid = NULL;\
  cv.var = var_



template<typename T>
void BindConstant(T constant,const SQChar * scriptVarName) {
  HSQUIRRELVM vm = SquirrelVM::GetVMPtr();
  sq_pushconsttable(vm);
  sq_pushstring(vm, scriptVarName, -1);
  Push(vm, constant);
  if (SQ_FAILED(sq_rawset(vm, -3)))
    DAGOR_THROW ( SquirrelError(_SC("Failed to bind constant")));
  sq_pop(vm, 1);
} // BindConstant

// === Register a class instance member variable or constant. var argument provides type and offset ( effectively &((ClassType *)0)->var ) ===

// classType is the type of the member variable's containing class.
template<typename T>
void RegisterInstanceVariable(SquirrelObject & so,SQUserPointer classType,T * var,const SQChar * scriptVarName,VarAccessType access=VAR_ACCESS_READ_WRITE) {
  VarRef * pvr = createVarRef(so,scriptVarName);
  void * offsetOrAddrOrConst = (void *)var; // var must be passed in as &obj->var, where obj = 0 (the address is the offset), or as static/global address.
  *pvr = VarRef(offsetOrAddrOrConst,TypeInfo<T>(),classType,ClassType<T>::type(),ClassType<T>::getCopyFunc(),sizeof(*var),access,TypeInfo<T>().typeName);
  createInstanceSetGetHandlers(so);
} // RegisterInstanceVariable


//////////////////////////////////////////////////////////////////////////
/////////// BEGIN Generalized Class/Struct Instance Support //////////////
//////////////////////////////////////////////////////////////////////////

SQBool CreateNativeClassInstance(HSQUIRRELVM v,const SQChar * classname,SQUserPointer ud,SQRELEASEHOOK hook);


template<typename T>
inline int ReturnNativeCopy(HSQUIRRELVM v, const T & classToCopy)
{
  int oldtop = sq_gettop(v);
  SquirrelObject ctbl = SquirrelVM::GetNativeClassesTable();
  sq_pushobject(v, ctbl.GetObjectHandle());     // +1 to stack
  sq_pushinteger(v, SQInteger(ClassType<T>::type()));
  if (SQ_FAILED(sq_get(v, -2)))
  {
    sq_settop(v, oldtop);
    return 0;
  }

  sq_pushroottable(v); // Push the 'this'.
  if (SQ_FAILED(sq_call(v, 1, SQTrue, SQ_CALL_RAISE_ERROR)))
  {
    sq_settop(v, oldtop);
    return 0;
  }
  sq_remove(v, -2); //roottable
  sq_remove(v, -2); //class table
  //G_ASSERT(oldtop == sq_gettop(v)-1);

  SQUserPointer up=0;
  if (SQ_FAILED(sq_getinstanceup(v,-1,&up,ClassType<T>::type())))
  {
    sq_settop(v, oldtop);
    return 0;
  }
  //G_ASSERT(up);
  T * newClass = (T *)up;
  *newClass = classToCopy;

  return 1;
}


template<typename T>
inline int ReturnCopy(HSQUIRRELVM v, const T & classToCopy)
{
  return ReturnNativeCopy<T>(v, classToCopy);
}

template<typename T>
inline SQBool CreateCopyInstance(HSQUIRRELVM v, const T & classToCopy)
{
  return ReturnNativeCopy<T>(v, classToCopy);
}


// Get an instance of type T from the stack at idx (for function calls).
template<typename T,bool ExceptionOnError>
T * GetInstance(HSQUIRRELVM v,SQInteger idx) {
#if DAGOR_DBGLEVEL > 0
  SQUserPointer up=0;
  SQRESULT result=sq_getinstanceup(v,idx,&up,ClassType<T>::type());
  if (ExceptionOnError) { // This code block should be compiled out when ExceptionOnError is false. In any case, the compiler should not generate a test condition (include or exclude the enclosed code block).
    if (SQ_FAILED(result) || !up) DAGOR_THROW ( SquirrelError(_SC("GetInstance: Invalid argument type")));
  } // if
  if (SQ_FAILED(result)) return NULL;
  return (T *)up;
#else
  if (ExceptionOnError)
    return (T*)sqplus_rawgetinstanceup(v,idx);
  else
  {
    SQUserPointer up = NULL;
    return SQ_FAILED(sq_getinstanceup(v,idx,&up,ClassType<T>::type())) ? NULL : (T*)up;
  }
#endif
} // GetInstance

// NAME and macro changes from Ben's (Project5) forum post. 2/26/06 jcs

#define SQPLUS_DECLARE_TYPE_INFO(TYPE, NAME) \
  inline const SQChar * GetTypeName(const TYPE & /*n*/) { return _SC(#NAME); } \
  template<> \
  struct TypeInfo<TYPE> { \
    const SQChar * typeName; \
    TypeInfo() : typeName(_SC(#NAME)) {} \
    enum {TypeID=VAR_TYPE_INSTANCE,Size=sizeof(TYPE)}; \
    operator ScriptVarType() { return ScriptVarType(TypeID); } \
  };

#define SQPLUS_DECLARE_TYPE_OPERATIONS(TYPE, NAME) \
  inline bool Match(TypeWrapper<TYPE &>,HSQUIRRELVM v,int idx) { \
    return GetInstance<TYPE,false>(v,idx) != NULL; } \
  inline bool Match(TypeWrapper<TYPE *>,HSQUIRRELVM v,int idx) { \
    return (sq_gettype(v,idx)==OT_NULL) || (GetInstance<TYPE,false>(v,idx) != NULL); } \
  inline TYPE & Get(TypeWrapper<TYPE &>,HSQUIRRELVM v,int idx) { \
    return *GetInstance<TYPE,true>(v,idx); } \
  inline TYPE * Get(TypeWrapper<TYPE *>,HSQUIRRELVM v,int idx) { \
    if (sq_gettype(v,idx)==OT_NULL) return NULL;\
    return  GetInstance<TYPE,true>(v,idx); } \
  inline void PushNative(HSQUIRRELVM v,TYPE * value) { \
    if (!value) sq_pushnull(v); \
    else { \
      if (!CreateNativeClassInstance(v,GetTypeName(*value),value,0)) \
         DAGOR_THROW ( SquirrelError(_SC("Push(): could not create INSTANCE of " #NAME))); \
    } } \
  inline void PushNative(HSQUIRRELVM v,TYPE & value) { \
    if (!CreateCopyInstance(v, value)) \
      DAGOR_THROW ( SquirrelError(_SC("Push(): could not create INSTANCE copy of " #NAME))); }

#define SQPLUS_DECLARE_TYPE_PUSH_NATIVE(TYPE, NAME) \
  template<> struct functor<TYPE*> { static inline void push(HSQUIRRELVM v,TYPE * value) { PushNative(v, value); } }; \
  template<> struct functor<TYPE>  { static inline void push(HSQUIRRELVM v,TYPE & value) { PushNative(v, value); } }; \
  template<> struct functor<TYPE&> { static inline void push(HSQUIRRELVM v,TYPE & value) { PushNative(v, value); } };

#define SQPLUS_SPECIALIZE_SQUIRREL_OBJECT_TYPE_GET(TYPE, NAME) \
  template<> inline TYPE& SquirrelObject::Get(void) { \
    sq_pushobject(SquirrelVM::_VM,GetObjectHandle()); \
    TYPE& val = SqPlus::Get(SqPlus::TypeWrapper<TYPE&>(),SquirrelVM::_VM,-1); \
    sq_poptop(SquirrelVM::_VM); \
    return val; \
  } \
  template<> inline TYPE* SquirrelObject::Get(void) { \
    sq_pushobject(SquirrelVM::_VM,GetObjectHandle()); \
    TYPE* val = SqPlus::Get(SqPlus::TypeWrapper<TYPE*>(),SquirrelVM::_VM,-1); \
    sq_poptop(SquirrelVM::_VM); \
    return val; \
  }


#define DECLARE_INSTANCE_TYPE_NAME_(TYPE,NAME) \
namespace SqPlus { \
  SQPLUS_DECLARE_TYPE_INFO(TYPE,NAME) \
  SQPLUS_DECLARE_TYPE_OPERATIONS(TYPE,NAME) \
  SQPLUS_DECLARE_TYPE_PUSH_NATIVE(TYPE,NAME) \
} \
SQPLUS_SPECIALIZE_SQUIRREL_OBJECT_TYPE_GET(TYPE,NAME)


#ifndef SQPLUS_CONST_OPT
#define DECLARE_INSTANCE_TYPE(TYPE) DECLARE_INSTANCE_TYPE_NAME_(TYPE,TYPE)
#define DECLARE_INSTANCE_TYPE_NAME(TYPE,NAME) DECLARE_INSTANCE_TYPE_NAME_(TYPE,NAME)
#else
#define SQPLUS_DECLARE_INSTANCE_TYPE_CONST
#include "sqPlusConst.h"
#endif

//////////////////////////////////////////////////////////////////////////
//////////// END Generalized Class/Struct Instance Support ///////////////
//////////////////////////////////////////////////////////////////////////

#ifndef SQ_SKIP_ARG_ASSERT
  #define sq_argassert(arg,_index_) if (!Match(TypeWrapper<P##arg>(),v,_index_)) return sq_throwerror(v,_SC("Incorrect function argument"))
#else
  #define sq_argassert(arg,_index_)
#endif

// === Return value variants ===

template<class RT>
struct ReturnSpecialization {

  // === Standard Function calls ===

  static int Call(RT (*func)(),HSQUIRRELVM v,int /*index*/) {
    RT ret = func();
    Push(v,ret);
    return 1;
  }

  template<typename P1>
  static int Call(RT (*func)(P1),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2>
  static int Call(RT (*func)(P1,P2),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3>
  static int Call(RT (*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3,typename P4>
  static int Call(RT (*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5>
  static int Call(RT (*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
  static int Call(RT (*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
  static int Call(RT (*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6)
    );
    Push(v,ret);
    return 1;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
  static int Call(RT (*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    sq_argassert(8,index + 7);
    RT ret = func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6),
      Get(TypeWrapper<P8>(),v,index + 7)
    );
    Push(v,ret);
    return 1;
  }
  // === Member Function calls ===

  template <typename Callee>
  static int Call(Callee & callee,RT (Callee::*func)(),HSQUIRRELVM v,int /*index*/) {
    RT ret = (callee.*func)();
    Push(v,ret);
    return 1;
  }

  template <typename Callee,typename P1>
  static int Call(Callee & callee,RT (Callee::*func)(P1),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6)
    );
    Push(v,ret);
    return 1;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
  static int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    sq_argassert(8,index + 7);
    RT ret = (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6),
      Get(TypeWrapper<P8>(),v,index + 7)
    );
    Push(v,ret);
    return 1;
  }

#ifdef SQPLUS_CONST_OPT
#define SQPLUS_CALL_CONST_MFUNC_RET0
#include "sqPlusConst.h"
#endif
};

// === No return value variants ===

template<>
struct ReturnSpecialization<void> {

  // === Standard function calls ===

  static int Call(void (*func)(),HSQUIRRELVM v,int /*index*/) {
    (void)v;
    func();
    return 0;
  }

  template<typename P1>
  static int Call(void (*func)(P1),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    func(
      Get(TypeWrapper<P1>(),v,index + 0)
    );
    return 0;
  }

  template<typename P1,typename P2>
  static int Call(void (*func)(P1,P2),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3>
  static int Call(void (*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3,typename P4>
  static int Call(void (*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5>
  static int Call(void (*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
  static int Call(void (*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
  static int Call(void (*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6)
    );
    return 0;
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
  static int Call(void (*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    sq_argassert(8,index + 7);
    func(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6),
      Get(TypeWrapper<P8>(),v,index + 7)
    );
    return 0;
  }
  // === Member function calls ===

  template<typename Callee>
  static int Call(Callee & callee,void (Callee::*func)(),HSQUIRRELVM,int /*index*/) {
    (callee.*func)();
    return 0;
  }

  template<typename Callee,typename P1>
  static int Call(Callee & callee,void (Callee::*func)(P1),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6)
    );
    return 0;
  }

  template<typename Callee,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
  static int Call(Callee & callee,void (Callee::*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
    sq_argassert(1,index + 0);
    sq_argassert(2,index + 1);
    sq_argassert(3,index + 2);
    sq_argassert(4,index + 3);
    sq_argassert(5,index + 4);
    sq_argassert(6,index + 5);
    sq_argassert(7,index + 6);
    sq_argassert(8,index + 7);
    (callee.*func)(
      Get(TypeWrapper<P1>(),v,index + 0),
      Get(TypeWrapper<P2>(),v,index + 1),
      Get(TypeWrapper<P3>(),v,index + 2),
      Get(TypeWrapper<P4>(),v,index + 3),
      Get(TypeWrapper<P5>(),v,index + 4),
      Get(TypeWrapper<P6>(),v,index + 5),
      Get(TypeWrapper<P7>(),v,index + 6),
      Get(TypeWrapper<P8>(),v,index + 7)
    );
    return 0;
  }

#ifdef SQPLUS_CONST_OPT
#define SQPLUS_CALL_CONST_MFUNC_NORET
#include "sqPlusConst.h"
#endif

};

// === STANDARD Function return value specialized call handlers ===

template<typename RT>
int Call(RT (*func)(),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1>
int Call(RT (*func)(P1),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2>
int Call(RT (*func)(P1,P2),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3>
int Call(RT (*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3,typename P4>
int Call(RT (*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3,typename P4,typename P5>
int Call(RT (*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
int Call(RT (*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
int Call(RT (*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}

template<typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
int Call(RT (*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(func,v,index);
}
// === MEMBER Function return value specialized call handlers ===

template<typename Callee,typename RT>
int Call(Callee & callee, RT (Callee::*func)(),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1>
int Call(Callee & callee,RT (Callee::*func)(P1),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2>
int Call(Callee & callee,RT (Callee::*func)(P1,P2),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}

template<typename Callee,typename RT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
int Call(Callee & callee,RT (Callee::*func)(P1,P2,P3,P4,P5,P6,P7,P8),HSQUIRRELVM v,int index) {
  return ReturnSpecialization<RT>::Call(callee,func,v,index);
}
#ifdef SQPLUS_CONST_OPT
#define SQPLUS_CALL_CONST_MFUNC_RET1
#include "sqPlusConst.h"
#endif

// === Direct Call Standard Function handler ===

template<typename Func>
struct DirectCallFunction {
  static inline SQInteger Dispatch(HSQUIRRELVM v) {
    StackHandler sa(v);
    int paramCount = sa.GetParamCount();
    Func * func = (Func *)sa.GetUserData(paramCount);
    return Call(*func,v,2);
  } // Dispatch
};

// === Direct Call Member Function handler ===

template<typename Callee,typename Func>
class DirectCallMemberFunction {
public:
  static inline SQInteger Dispatch(HSQUIRRELVM v) {
    StackHandler sa(v);
    int paramCount = sa.GetParamCount();
    unsigned char * ud = (unsigned char *)sa.GetUserData(paramCount);
    return Call(**(Callee**)ud,*(Func*)(ud + sizeof(Callee*)),v,2);
  } // Dispatch
};

// === Direct Call Instance Member Function handler ===


template<typename Callee,typename Func>
class DirectCallInstanceMemberFunction {
public:
  static inline SQInteger Dispatch(HSQUIRRELVM v) {
    StackHandler sa(v);
    Callee * instance = (Callee *)sa.GetInstanceUp(1,0);
    int paramCount = sa.GetParamCount();
    Func * func = (Func *)sa.GetUserData(paramCount);
    return Call<Callee>(*instance,*func,v,2);
  } // Dispatch
};


// === Standard function call ===

template<typename Func>
inline void sq_pushdirectclosure(HSQUIRRELVM v,Func func,SQUnsignedInteger nupvalues) {
  SQUserPointer up = sq_newuserdata(v,sizeof(func)); // Also pushed on stack.
  memcpy(up,&func,sizeof(func));
  sq_newclosure(v,DirectCallFunction<Func>::Dispatch,nupvalues+1);
} // sq_pushdirectclosure

// === Fixed Class pointer call (always calls with object pointer that was registered) ===

template<typename Callee,typename Func>
inline void sq_pushdirectclosure(HSQUIRRELVM v,const Callee & callee,Func func,SQUnsignedInteger nupvalues) {
  unsigned char * up = (unsigned char *)sq_newuserdata(v,sizeof(Callee*)+sizeof(func));  // Also pushed on stack.
  const SQUserPointer pCallee = (SQUserPointer)&callee;
  memcpy(up,&pCallee,sizeof(Callee*));
  memcpy(up + sizeof(Callee*),&func,sizeof(func));
  sq_newclosure(v,DirectCallMemberFunction<Callee,Func>::Dispatch,nupvalues+1);
} // sq_pushdirectclosure

// === Class Instance call: class pointer retrieved from script class instance ===

template<typename Callee,typename Func>
inline void sq_pushdirectinstanceclosure(HSQUIRRELVM v,const Callee & /*callee*/,Func func,SQUnsignedInteger nupvalues) {
  unsigned char * up = (unsigned char *)sq_newuserdata(v,sizeof(func));  // Also pushed on stack.
  memcpy(up,&func,sizeof(func));
  sq_newclosure(v,DirectCallInstanceMemberFunction<Callee,Func>::Dispatch,nupvalues+1);
} // sq_pushdirectinstanceclosure


// === Register a STANDARD function (table or class on stack) ===

template<typename Func>
inline void Register(HSQUIRRELVM v,Func func,const SQChar * name) {
  sq_pushstring(v,name,-1);
  sq_pushdirectclosure(v,func,0);
#if DAGOR_DBGLEVEL > 0
  sq_setnativeclosurename(v,-1,name); // For debugging only.
#endif
  sq_newslot(v,-3,SQFalse); // Stack is restored after this call (same state as before Register() call).
} // Register

// === Register a MEMBER function (table or class on stack) ===

template<typename Callee,typename Func>
inline void Register(HSQUIRRELVM v,Callee & callee,Func func,const SQChar * name) {
  sq_pushstring(v,name,-1);
  sq_pushdirectclosure(v,callee,func,0);
#if DAGOR_DBGLEVEL > 0
  sq_setnativeclosurename(v,-1,name); // For debugging only.
#endif
  sq_newslot(v,-3,SQFalse); // Stack is restored after this call (same state as before Register() call).
} // Register

// === Register a STANDARD global function (root table) ===

template<typename Func>
inline void RegisterGlobal(HSQUIRRELVM v,Func func,const SQChar * name) {
  sq_pushroottable(v);
  Register(v,func,name);
  sq_poptop(v); // Remove root table.
} // RegisterGlobal

template<typename Func>
inline void RegisterGlobal(Func func,const SQChar * name) {
  RegisterGlobal(SquirrelVM::GetVMPtr(),func,name);
} // RegisterGlobal

// === Register a MEMBER global function (root table) ===

template<typename Callee,typename Func>
inline void RegisterGlobal(HSQUIRRELVM v,Callee & callee,Func func,const SQChar * name) {
  sq_pushroottable(v);
  Register(v,callee,func,name);
  sq_poptop(v); // Remove root table.
} // RegisterGlobal

template<typename Callee,typename Func>
inline void RegisterGlobal(Callee & callee,Func func,const SQChar * name) {
  RegisterGlobal(SquirrelVM::GetVMPtr(),callee,func,name);
} // RegisterGlobal

// === Register a STANDARD function (hso is table or class) ===

template<typename Func>
inline void Register(HSQUIRRELVM v,HSQOBJECT hso,Func func,const SQChar * name) {
  sq_pushobject(v,hso);
  Register(v,func,name);
  sq_poptop(v); // Remove hso.
} // Register

// === Register a MEMBER function (hso is table or class) ===
// === Fixed Class pointer call (always calls with object pointer that was registered) ===

template<typename Callee,typename Func>
inline void Register(HSQUIRRELVM v,HSQOBJECT hso,Callee & callee,Func func,const SQChar * name) {
  sq_pushobject(v,hso);
  Register(v,callee,func,name);
  sq_poptop(v); // Remove hso.
} // Register

// === Register an INSTANCE MEMBER function ===
// === Class Instance call: class pointer retrieved from script class instance ===

template<typename Callee,typename Func>
inline void RegisterInstance(HSQUIRRELVM v,HSQOBJECT hclass,Callee & callee,Func func,const SQChar * name) {
  sq_pushobject(v,hclass);
  sq_pushstring(v,name,-1);
  sq_pushdirectinstanceclosure(v,callee,func,0);
  sq_newslot(v,-3,SQFalse);
  sq_poptop(v); // Remove hclass.
} // RegisterInstance


// === Call Squirrel Functions from C/C++ ===
// No type checking is performed for Squirrel functions as Squirrel types are dynamic:
// Incoming types are passed unchanged to Squirrel functions. The parameter count is checked: an exception is thrown if mismatched.
// Return values must match the RT template argument type, else an exception can be thrown on return.

template<typename RT>
struct SquirrelFunction {
  HSQUIRRELVM v;
  SquirrelObject object; // Table or class.
  SquirrelObject func;
  SquirrelFunction() : v(0) {}
  SquirrelFunction(HSQUIRRELVM _v,const SquirrelObject & _object,const SquirrelObject & _func) : v(_v), object(_object), func(_func) {}
  SquirrelFunction(const SquirrelObject & _object,const SquirrelObject & _func) : v(SquirrelVM::GetVMPtr()), object(_object), func(_func) {}
  SquirrelFunction(const SquirrelObject & _object,const SQChar * name) {
    v      = SquirrelVM::GetVMPtr();
    object = _object;
    func   = object.GetValue(name);
  }
  SquirrelFunction(const SQChar * name) {
    v      = SquirrelVM::GetVMPtr();
    object = SquirrelVM::GetRootTable();
    func   = object.GetValue(name);
  }

  // Release references and reset internal objects to null.
  void reset(void) {
    func.Reset();
    object.Reset();
  } // Reset

  // return true if func field is not null and function can be called
  bool isValid() { return !func.IsNull(); }

// If handler was overriden by users, then we have already handled the error
// on lower levels
#define SQPLUS_CHECK_FNCALL(res) \
  if (!SQ_SUCCEEDED(res)) \
  { \
    sq_pushnull(v); /* compensate for return value which is not returned in case of sq_call() fail (for GetRet()) */ \
    if (!dag_sq_is_errhandler_overriden(v)) \
      DAGOR_THROW ( SquirrelError(_SC("SquirrelFunction<> call failed"))); \
  }

  RT operator()(void) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    SQPLUS_CHECK_FNCALL(sq_call(v,1,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1>
  RT operator()(P1 p1) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    SQPLUS_CHECK_FNCALL(sq_call(v,2,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2>
  RT operator()(P1 p1,P2 p2) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    SQPLUS_CHECK_FNCALL(sq_call(v,3,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3>
  RT operator()(P1 p1,P2 p2,P3 p3) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    SQPLUS_CHECK_FNCALL(sq_call(v,4,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3,typename P4>
  RT operator()(P1 p1,P2 p2,P3 p3,P4 p4) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    Push(v,p4);
    SQPLUS_CHECK_FNCALL(sq_call(v,5,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5>
  RT operator()(P1 p1,P2 p2,P3 p3,P4 p4,P5 p5) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    Push(v,p4);
    Push(v,p5);
    SQPLUS_CHECK_FNCALL(sq_call(v,6,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
  RT operator()(P1 p1,P2 p2,P3 p3,P4 p4,P5 p5,P6 p6) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    Push(v,p4);
    Push(v,p5);
    Push(v,p6);
    SQPLUS_CHECK_FNCALL(sq_call(v,7,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
  RT operator()(P1 p1,P2 p2,P3 p3,P4 p4,P5 p5,P6 p6,P7 p7) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    Push(v,p4);
    Push(v,p5);
    Push(v,p6);
    Push(v,p7);
    SQPLUS_CHECK_FNCALL(sq_call(v,8,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }

  template<typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
  RT operator()(P1 p1,P2 p2,P3 p3,P4 p4,P5 p5,P6 p6,P7 p7, P8 p8) {
    sq_pushobject(v,func.GetObjectHandle());
    sq_pushobject(v,object.GetObjectHandle());
    Push(v,p1);
    Push(v,p2);
    Push(v,p3);
    Push(v,p4);
    Push(v,p5);
    Push(v,p6);
    Push(v,p7);
    Push(v,p8);
    SQPLUS_CHECK_FNCALL(sq_call(v,9,SQTrue,SQ_CALL_RAISE_ERROR));
    return GetRet(TypeWrapper<RT>(),v,-1);
  }
};

// === Class/Struct registration ===

// Allow implementing custom deleter
template<class T> inline void DestroyObject(T *obj) {
  delete obj;
}

#define SQ_DELETE_CLASS(CLASSTYPE) if (up) { CLASSTYPE * self = (CLASSTYPE *)up; SqPlus::DestroyObject(self);} return 0
#define SQ_DECLARE_RELEASE(CLASSTYPE) \
  static SQInteger release(SQUserPointer up,SQInteger /*size*/) { \
    SQ_DELETE_CLASS(CLASSTYPE); \
  }

template<typename T>
struct ReleaseClassPtrPtr {
  static SQInteger release(SQUserPointer up,SQInteger /*size*/) {
    if (up) {
      T ** self = (T **)up;
      SqPlus::DestroyObject(*self);
    } // if
    return 0;
  } // release
};

template<typename T>
struct ReleaseClassPtr {
  static SQInteger release(SQUserPointer up,SQInteger /*size*/) {
    if (up) {
      T * self = (T *)up;
      SqPlus::DestroyObject(self);
    } // if
    return 0;
  } // release
};

SQBool CreateClass(HSQUIRRELVM v,SquirrelObject & newClass,SQUserPointer classType,const SQChar * name,const SQChar * baseName=0);


// Call PostConstruct() at the end of custom constructors.
template<typename T>
inline int PostConstruct(HSQUIRRELVM v,T * newClass,SQRELEASEHOOK hook) {
  sq_setinstanceup(v,1,newClass);
  sq_setreleasehook(v,1,hook);
  return 1;
} // PostConstruct



template<typename T>
struct ConstructReleaseClass {
  static SQInteger construct(HSQUIRRELVM v) {
    return PostConstruct<T>(v,new T(),release);
  } // construct
  SQ_DECLARE_RELEASE(T)
};


template<typename T>
inline SquirrelObject RegisterClassType(HSQUIRRELVM v,const SQChar * scriptClassName,const SQChar * baseScriptClassName=0) {
  int top = sq_gettop(v);
  SquirrelObject newClass;
  if (CreateClass(v,newClass,(SQUserPointer)ClassType<T>::type(),scriptClassName,baseScriptClassName)) {
    SquirrelVM::CreateFunction(newClass,&ConstructReleaseClass<T>::construct,_SC("constructor"));
    SquirrelObject ctbl = SquirrelVM::GetNativeClassesTable();
    ctbl.SetValue(SQInteger(ClassType<T>::type()), newClass);
  } // if
  sq_settop(v,top);
  return newClass;
} // RegisterClassType

template<typename T>
inline SquirrelObject RegisterClassTypeNoConstructor(HSQUIRRELVM v,const SQChar * scriptClassName,const SQChar * baseScriptClassName=0) {
  int top = sq_gettop(v);
  SquirrelObject newClass;
  if (CreateClass(v,newClass,(SQUserPointer)ClassType<T>::type(),scriptClassName,baseScriptClassName)) {
  }
  sq_settop(v,top);
  return newClass;
} // RegisterClassTypeNoConstructor

// === Define and register a C++ class and its members for use with Squirrel ===
// Constructors+destructors are automatically created. Custom constructors must use the
// standard SQFUNCTION signature if variable argument types are required (overloads).
// See testSqPlus2.cpp for examples.

// <NOTE> Do not use SQClassDefBase<> directly, use SQClassDef<> or SQClassDefNoConstructor<>, below.
template<typename TClassType>
struct SQClassDefBase {
  HSQUIRRELVM v;
  const SQChar * name;
  SquirrelObject newClass;

  SQClassDefBase(HSQUIRRELVM _v,const SQChar * _name) : v(_v), name(_name) {}
  SQClassDefBase(const SQChar * _name) : v(SquirrelVM::GetVMPtr()), name(_name) {}

  // Register a member function.
  template<typename Func>
  SQClassDefBase & func(Func pfunc,const SQChar * name) {
    RegisterInstance(v,newClass.GetObjectHandle(),*(TClassType *)0,pfunc,name);
    return *this;
  } // func

  // === BEGIN static-member+global function registration ===

  // === This version is for static member functions only, such as custom constructors where 'this' is not yet valid ===
  // typeMask: "*" means don't check parameters, typeMask=0 means function takes no arguments (and is type checked for that case).
  // All the other Squirrel type-masks are passed normally.

  template<typename Func>
  SQClassDefBase & staticFuncVarArgs(Func pfunc,const SQChar * name,
    const SQChar * typeMask=_SC("*"), SQInteger nparamscheck=SQ_MATCHTYPEMASKSTRING)
  {
    SquirrelVM::PushObject(newClass);
    SquirrelVM::CreateFunction(pfunc, name, typeMask, nparamscheck);
    SquirrelVM::Pop(1);
    return *this;
  } // staticFuncVarArgs

  // Register a standard global function (effectively embedding a global function in TClassType's script namespace: does not need or use a 'this' pointer).
  template<typename Func>
  SQClassDefBase & staticFunc(Func pfunc,const SQChar * name) {
    Register(v,newClass.GetObjectHandle(),pfunc,name);
    return *this;
  } // staticFunc

  // Register a function to a pre-allocated class/struct member function: will use callee's 'this' (effectively embedding a global function in TClassType's script namespace).
  template<typename Callee,typename Func>
  SQClassDefBase & staticFunc(Callee & callee,Func pfunc,const SQChar * name) {
    Register(v,newClass.GetObjectHandle(),callee,pfunc,name);
    return *this;
  } // staticFunc

  // === END static+global function registration ===

  // Register a member variable.
  template<typename VarType>
  SQClassDefBase & var(VarType TClassType::* pvar,const SQChar * name,VarAccessType access=VAR_ACCESS_READ_WRITE) {
    SQ_CV_TYPE(VarType TClassType::*, VarType*, pvar);
    RegisterInstanceVariable(newClass,ClassType<TClassType>::type(),cv.var2,name,access);
    return *this;
  } // var

};

template<typename TClassType>
struct SQClassDef : public SQClassDefBase<TClassType> {
  typedef SQClassDefBase<TClassType> BASE;
  SQClassDef(HSQUIRRELVM _v,const SQChar * _name) : SQClassDefBase<TClassType>(_v,_name) {
    BASE::newClass = RegisterClassType<TClassType>(BASE::v,BASE::name);
  }
  SQClassDef(const SQChar * _name) : SQClassDefBase<TClassType>(_name) {
    BASE::newClass = RegisterClassType<TClassType>(BASE::v,BASE::name);
  }
};

template<typename TClassType>
struct SQClassDefNoConstructor : public SQClassDefBase<TClassType> {
  typedef SQClassDefBase<TClassType> BASE;
  SQClassDefNoConstructor(HSQUIRRELVM _v,const SQChar * _name) : SQClassDefBase<TClassType>(_v,_name) {
    BASE::newClass = RegisterClassTypeNoConstructor<TClassType>(BASE::v,BASE::name);
  }
  SQClassDefNoConstructor(const SQChar * _name) : SQClassDefBase<TClassType>(_name) {
    BASE::newClass = RegisterClassTypeNoConstructor<TClassType>(BASE::v,BASE::name);
  }
};

// === BEGIN Function Call Handlers ===

#define SQPLUS_CHECK_GET(res) if (!SQ_SUCCEEDED(res)) DAGOR_THROW ( SquirrelError(_SC("sq_get*() failed (type error)")))

inline bool Match(TypeWrapper<bool>,HSQUIRRELVM v,int idx)           { return sq_gettype(v,idx) & SQOBJECT_CANBEFALSE; }
inline bool Match(TypeWrapper<signed char>,HSQUIRRELVM v,int idx)    { return sq_gettype(v,idx) == OT_INTEGER; }
inline bool Match(TypeWrapper<unsigned char>,HSQUIRRELVM v, int idx) { return sq_gettype(v,idx) == OT_INTEGER; }
inline bool Match(TypeWrapper<short>,HSQUIRRELVM v,int idx)          { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<unsigned short>,HSQUIRRELVM v,int idx) { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<SQInt32>,HSQUIRRELVM v,int idx)        { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<SQInteger>,HSQUIRRELVM v,int idx)      { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<unsigned int>,HSQUIRRELVM v,int idx)   { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<unsigned long>,HSQUIRRELVM v,int idx)  { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<float>,HSQUIRRELVM v,int idx)          { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<double>,HSQUIRRELVM v,int idx)         { return sq_gettype(v,idx) & SQOBJECT_NUMERIC; }
inline bool Match(TypeWrapper<HSQUIRRELVM>,HSQUIRRELVM /*v*/,int /*idx*/)    { return true; } // See Get() for HSQUIRRELVM below (v is always present).
inline bool Match(TypeWrapper<SQAnythingPtr>,HSQUIRRELVM v,int idx)  { return sq_gettype(v,idx) == OT_USERPOINTER; }
inline bool Match(TypeWrapper<SquirrelObject>,HSQUIRRELVM /*v*/,int /*idx*/) { return true; } // See sq_getstackobj(): always returns true.
inline bool Match(TypeWrapper<const SQChar *>,HSQUIRRELVM v,int idx) { return sq_gettype(v,idx) == OT_STRING; }
inline bool Match(TypeWrapper<      SQChar *>,HSQUIRRELVM v,int idx) { return sq_gettype(v,idx) == OT_STRING; }

inline void           Get(TypeWrapper<void>,HSQUIRRELVM /*v*/,int)            {}
inline bool           Get(TypeWrapper<bool>,HSQUIRRELVM v,int idx)            { SQBool b; sq_tobool(v,idx,&b); return b != 0; }
inline char           Get(TypeWrapper<signed char>,HSQUIRRELVM v,int idx)     { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<char>(i); }
inline unsigned char  Get(TypeWrapper<unsigned char>,HSQUIRRELVM v,int idx)   { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<unsigned char>(i); }
inline short          Get(TypeWrapper<short>,HSQUIRRELVM v,int idx)           { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<short>(i); }
inline unsigned short Get(TypeWrapper<unsigned short>,HSQUIRRELVM v,int idx)  { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<unsigned short>(i); }
inline int            Get(TypeWrapper<SQInt32>,HSQUIRRELVM v,int idx)         { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return i; }
inline SQInteger      Get(TypeWrapper<SQInteger>,HSQUIRRELVM v,int idx)       { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return i; }
inline unsigned int   Get(TypeWrapper<unsigned int>,HSQUIRRELVM v,int idx)    { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<unsigned int>(i); }
inline unsigned long  Get(TypeWrapper<unsigned long>,HSQUIRRELVM v,int idx)   { SQInteger i; SQPLUS_CHECK_GET(sq_getinteger(v,idx,&i)); return static_cast<unsigned long>(i); }
inline float          Get(TypeWrapper<float>,HSQUIRRELVM v,int idx)           { SQFloat f; SQPLUS_CHECK_GET(sq_getfloat(v,idx,&f)); return f; }
inline double         Get(TypeWrapper<double>,HSQUIRRELVM v,int idx)          { SQFloat f; SQPLUS_CHECK_GET(sq_getfloat(v,idx,&f)); return static_cast<double>(f); }
inline SquirrelNull   Get(TypeWrapper<SquirrelNull>,HSQUIRRELVM v,int idx)    { (void)v, (void)idx; return SquirrelNull();  }
inline SQAnythingPtr  Get(TypeWrapper<SQAnythingPtr>,HSQUIRRELVM v,int idx)   { SQUserPointer p; SQPLUS_CHECK_GET(sq_getuserpointer(v,idx,&p)); return (SQAnythingPtr)p; }
inline HSQUIRRELVM    Get(TypeWrapper<HSQUIRRELVM>,HSQUIRRELVM v,int /*idx*/) { sq_poptop(v); return v; } // sq_poptop(v): remove UserData from stack so GetParamCount() matches normal behavior.
inline SquirrelObject Get(TypeWrapper<SquirrelObject>,HSQUIRRELVM v,int idx)  { HSQOBJECT o; SQPLUS_CHECK_GET(sq_getstackobj(v,idx,&o)); return SquirrelObject(o); }
inline const SQChar * Get(TypeWrapper<const SQChar *>,HSQUIRRELVM v,int idx)  { const SQChar * s; SQPLUS_CHECK_GET(sq_getstring(v,idx,&s)); return s; }
inline       SQChar * Get(TypeWrapper<      SQChar *>,HSQUIRRELVM v,int idx)  { const SQChar * s; SQPLUS_CHECK_GET(sq_getstring(v,idx,&s)); return (SQChar*)s; }


// GetRet() restores the stack for SquirrelFunction<>() calls.
template<typename RT>
inline RT GetRet(TypeWrapper<RT>,HSQUIRRELVM v,int idx) { RT ret = Get(TypeWrapper<RT>(),v,idx); sq_pop(v,2); return ret; } // sq_pop(v,2): restore stack after function call.

// Specialization to support void return type.
inline void GetRet(TypeWrapper<void>,HSQUIRRELVM v,int /*idx*/) { sq_pop(v,2); }

// Intentionally implemented with invalid signature (returns void) to forbid returning raw string pointers from SQVM (which is UB)
inline void GetRet(TypeWrapper<const SQChar *>,HSQUIRRELVM,int /*idx*/) {}
inline void GetRet(TypeWrapper<SQChar *>,HSQUIRRELVM,int /*idx*/) {}

}; // namespace SqPlus


// Get any bound type from this SquirrelObject. Note that Squirrel's handling of references and pointers still holds here.
template<typename _ty>
inline _ty SquirrelObject::Get(void) {
  sq_pushobject(SquirrelVM::_VM,GetObjectHandle());
  _ty val = SqPlus::Get(SqPlus::TypeWrapper<_ty>(),SquirrelVM::_VM,-1);
  sq_poptop(SquirrelVM::_VM);
  return val;
}


#if defined(__clang__)
# pragma clang diagnostic pop
#endif
#endif //_SQ_PLUS_H_
