/*
Copyright (c) 2003-2017 Alberto Demichelis
Copyright (c) 2016-2023 by Gaijin Games KFT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef _SQUIRREL_H_
#define _SQUIRREL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SQUIRREL_API
#define SQUIRREL_API extern
#endif

#define SQTrue  (1)
#define SQFalse (0)

struct SQVM;
struct SQTable;
struct SQArray;
struct SQString;
struct SQClosure;
struct SQGenerator;
struct SQNativeClosure;
struct SQUserData;
struct SQFunctionProto;
struct SQRefCounted;
struct SQClass;
struct SQInstance;
struct SQDelegable;
struct SQOuter;

class OutputStream;
class Arena;

class KeyValueFile;

namespace SQCompilation
{
  struct SqASTData;
}

#include "sqconfig.h"
#include <stdio.h>

#define SQUIRREL_VERSION_NUMBER_MAJOR 4
#define SQUIRREL_VERSION_NUMBER_MINOR 8
#define SQUIRREL_VERSION_NUMBER_PATCH 0

#define SQUIRREL_VERSION    _SC("4.8.0")
#define SQUIRREL_COPYRIGHT  _SC("Copyright (C) 2003-2016 Alberto Demichelis; 2016-2024 Gaijin Games KFT")

#define SQ_VMSTATE_IDLE         0
#define SQ_VMSTATE_RUNNING      1
#define SQ_VMSTATE_SUSPENDED    2

#define SQUIRREL_EOB 0
#define SQ_BYTECODE_STREAM_TAG  0xFAFA

#define SQOBJECT_REF_COUNTED    0x08000000
#define SQOBJECT_NUMERIC        0x04000000
#define SQOBJECT_DELEGABLE      0x02000000
#define SQOBJECT_CANBEFALSE     0x01000000

#define SQOBJ_FLAG_IMMUTABLE    0x01

#define SQ_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) ((type)&_RT_MASK)

#define _RT_NULL            0x00000001
#define _RT_INTEGER         0x00000002
#define _RT_FLOAT           0x00000004
#define _RT_BOOL            0x00000008
#define _RT_STRING          0x00000010
#define _RT_TABLE           0x00000020
#define _RT_ARRAY           0x00000040
#define _RT_USERDATA        0x00000080
#define _RT_CLOSURE         0x00000100
#define _RT_NATIVECLOSURE   0x00000200
#define _RT_GENERATOR       0x00000400
#define _RT_USERPOINTER     0x00000800
#define _RT_THREAD          0x00001000
#define _RT_FUNCPROTO       0x00002000
#define _RT_CLASS           0x00004000
#define _RT_INSTANCE        0x00008000
#define _RT_WEAKREF         0x00010000
#define _RT_OUTER           0x00020000

typedef enum tagSQObjectType{
    OT_NULL =           (_RT_NULL|SQOBJECT_CANBEFALSE),
    OT_INTEGER =        (_RT_INTEGER|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
    OT_FLOAT =          (_RT_FLOAT|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
    OT_BOOL =           (_RT_BOOL|SQOBJECT_CANBEFALSE),
    OT_STRING =         (_RT_STRING|SQOBJECT_REF_COUNTED),
    OT_TABLE =          (_RT_TABLE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
    OT_ARRAY =          (_RT_ARRAY|SQOBJECT_REF_COUNTED),
    OT_USERDATA =       (_RT_USERDATA|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
    OT_CLOSURE =        (_RT_CLOSURE|SQOBJECT_REF_COUNTED),
    OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE|SQOBJECT_REF_COUNTED),
    OT_GENERATOR =      (_RT_GENERATOR|SQOBJECT_REF_COUNTED),
    OT_USERPOINTER =    _RT_USERPOINTER,
    OT_THREAD =         (_RT_THREAD|SQOBJECT_REF_COUNTED) ,
    OT_FUNCPROTO =      (_RT_FUNCPROTO|SQOBJECT_REF_COUNTED), //internal usage only
    OT_CLASS =          (_RT_CLASS|SQOBJECT_REF_COUNTED),
    OT_INSTANCE =       (_RT_INSTANCE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
    OT_WEAKREF =        (_RT_WEAKREF|SQOBJECT_REF_COUNTED),
    OT_OUTER =          (_RT_OUTER|SQOBJECT_REF_COUNTED) //internal usage only
}SQObjectType;

typedef uint8_t SQObjectFlags;

#define ISREFCOUNTED(t) ((t)&SQOBJECT_REF_COUNTED)


typedef union tagSQObjectValue
{
    struct SQTable *pTable;
    struct SQArray *pArray;
    struct SQClosure *pClosure;
    struct SQOuter *pOuter;
    struct SQGenerator *pGenerator;
    struct SQNativeClosure *pNativeClosure;
    struct SQString *pString;
    struct SQUserData *pUserData;
    SQInteger nInteger;
    SQFloat fFloat;
    SQUserPointer pUserPointer;
    struct SQFunctionProto *pFunctionProto;
    struct SQRefCounted *pRefCounted;
    struct SQDelegable *pDelegable;
    struct SQVM *pThread;
    struct SQClass *pClass;
    struct SQInstance *pInstance;
    struct SQWeakRef *pWeakRef;
    SQRawObjectVal raw;
}SQObjectValue;


typedef struct tagSQObject
{
    SQObjectType _type;
    SQObjectFlags _flags;
    SQObjectValue _unVal;
}SQObject;

typedef struct  tagSQMemberHandle{
    SQBool _static;
    SQInteger _index;
}SQMemberHandle;

typedef struct tagSQStackInfos{
    const SQChar* funcname;
    const SQChar* source;
    SQInteger line;
}SQStackInfos;

typedef enum tagSQMessageSeverity{
    SEV_HINT,
    SEV_WARNING,
    SEV_ERROR,
}SQMessageSeverity;

typedef struct SQVM* HSQUIRRELVM;
typedef SQObject HSQOBJECT;
typedef SQMemberHandle HSQMEMBERHANDLE;
typedef SQInteger (*SQFUNCTION)(HSQUIRRELVM);
typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size);
typedef void (*SQCOMPILERERROR)(HSQUIRRELVM,SQMessageSeverity /*severity*/,const SQChar * /*desc*/,const SQChar * /*source*/,SQInteger /*line*/,SQInteger /*column*/, const SQChar * /*extra info*/);
typedef void (*SQPRINTFUNCTION)(HSQUIRRELVM,const SQChar * ,...);
typedef void (*SQDEBUGHOOK)(HSQUIRRELVM /*v*/, SQInteger /*type*/, const SQChar * /*sourcename*/, SQInteger /*line*/, const SQChar * /*funcname*/);
typedef void (*SQCOMPILELINEHOOK)(HSQUIRRELVM /*v*/, const SQChar * /*sourcename*/, SQInteger /*line*/);
typedef SQInteger (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,SQInteger);
typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);
typedef SQInteger (*SQGETTHREAD)();
typedef void (*SQSQCALLHOOK)(HSQUIRRELVM);

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);

typedef struct tagSQRegFunction{
    const SQChar *name;
    SQFUNCTION f;
    SQInteger nparamscheck;
    const SQChar *typemask;
}SQRegFunction;

typedef struct tagSQFunctionInfo {
    SQUserPointer funcid;
    const SQChar *name;
    const SQChar *source;
    SQInteger line;
}SQFunctionInfo;

#define BIT(n) (1ULL << (n))

enum CompilationOptions : SQUnsignedInteger {
  CO_CLOSURE_HOISTING_OPT = BIT(1)
};

#undef BIT

typedef struct tagSQCompilerMessage {
  int intId;
  const char* textId;
  int line;
  int column;
  int columnsWidth;
  const char* message;
  const char* fileName;
  bool isError;
} SQCompilerMessage;

typedef void (*SQ_COMPILER_DIAG_CB)(HSQUIRRELVM v, const SQCompilerMessage *msg);


/*vm*/
SQUIRREL_API HSQUIRRELVM sq_open(SQInteger initialstacksize);
SQUIRREL_API HSQUIRRELVM sq_newthread(HSQUIRRELVM friendvm, SQInteger initialstacksize);
SQUIRREL_API void sq_seterrorhandler(HSQUIRRELVM v);
SQUIRREL_API HSQOBJECT sq_geterrorhandler(HSQUIRRELVM v);
SQUIRREL_API void sq_close(HSQUIRRELVM v);
SQUIRREL_API void sq_setforeignptr(HSQUIRRELVM v,SQUserPointer p);
SQUIRREL_API SQUserPointer sq_getforeignptr(HSQUIRRELVM v);
SQUIRREL_API void sq_setsharedforeignptr(HSQUIRRELVM v,SQUserPointer p);
SQUIRREL_API SQUserPointer sq_getsharedforeignptr(HSQUIRRELVM v);
SQUIRREL_API void sq_setvmreleasehook(HSQUIRRELVM v,SQRELEASEHOOK hook);
SQUIRREL_API SQRELEASEHOOK sq_getvmreleasehook(HSQUIRRELVM v);
SQUIRREL_API void sq_setsharedreleasehook(HSQUIRRELVM v,SQRELEASEHOOK hook);
SQUIRREL_API SQRELEASEHOOK sq_getsharedreleasehook(HSQUIRRELVM v);
SQUIRREL_API void sq_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errfunc);
SQUIRREL_API SQPRINTFUNCTION sq_getprintfunc(HSQUIRRELVM v);
SQUIRREL_API SQPRINTFUNCTION sq_geterrorfunc(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_suspendvm(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_wakeupvm(HSQUIRRELVM v,SQBool resumedret,SQBool retval,SQBool invoke_err_handler,SQBool throwerror);
SQUIRREL_API SQInteger sq_getvmstate(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_registerbaselib(HSQUIRRELVM v);

/*compiler*/
SQUIRREL_API SQRESULT sq_compile(HSQUIRRELVM v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror, const HSQOBJECT *bindings = nullptr);

SQUIRREL_API SQRESULT sq_parsetobinaryast(HSQUIRRELVM v, const SQChar *s, SQInteger size, const SQChar *sourcename, OutputStream *ostream, SQBool raiseerror);
SQUIRREL_API SQRESULT sq_translatebinaryasttobytecode(HSQUIRRELVM v, const uint8_t *buffer, size_t size, const HSQOBJECT *bindings, SQBool raiseerror);

SQUIRREL_API SQCompilation::SqASTData *sq_parsetoast(HSQUIRRELVM v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool preserveComments, SQBool raiseerror);
SQUIRREL_API SQRESULT sq_translateasttobytecode(HSQUIRRELVM v, SQCompilation::SqASTData *astData, const HSQOBJECT *bindings, const SQChar *s, SQInteger size, SQBool raiseerror, SQBool debugInfo);
SQUIRREL_API void sq_analyzeast(HSQUIRRELVM v, SQCompilation::SqASTData *astData, const HSQOBJECT *bindings, const SQChar *s, SQInteger size);
SQUIRREL_API void sq_checktrailingspaces(HSQUIRRELVM v, const SQChar *sourceName, const SQChar *s, SQInteger size);


SQUIRREL_API void sq_dumpast(HSQUIRRELVM v, SQCompilation::SqASTData *astData, OutputStream *s);
SQUIRREL_API void sq_dumpbytecode(HSQUIRRELVM v, HSQOBJECT obj, OutputStream *s, int instruction_index = -1);

SQUIRREL_API void sq_releaseASTData(HSQUIRRELVM v, SQCompilation::SqASTData *astData);

SQUIRREL_API void sq_setcompilationoption(HSQUIRRELVM v, enum CompilationOptions co, bool value);
SQUIRREL_API bool sq_checkcompilationoption(HSQUIRRELVM v, enum CompilationOptions co);
SQUIRREL_API void sq_enabledebuginfo(HSQUIRRELVM v, SQBool enable);
SQUIRREL_API void sq_enablevartrace(HSQUIRRELVM v, SQBool enable);
SQUIRREL_API SQBool sq_isvartracesupported();
SQUIRREL_API void sq_lineinfo_in_expressions(HSQUIRRELVM v, SQBool enable);
SQUIRREL_API void sq_notifyallexceptions(HSQUIRRELVM v, SQBool enable);
SQUIRREL_API void sq_setcompilererrorhandler(HSQUIRRELVM v,SQCOMPILERERROR f);
SQUIRREL_API void sq_setcompilerdiaghandler(HSQUIRRELVM v, SQ_COMPILER_DIAG_CB f);
SQUIRREL_API SQCOMPILERERROR sq_getcompilererrorhandler(HSQUIRRELVM v);

/*stack operations*/
SQUIRREL_API void sq_push(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API void sq_pop(HSQUIRRELVM v,SQInteger nelemstopop);
SQUIRREL_API void sq_poptop(HSQUIRRELVM v);
SQUIRREL_API void sq_remove(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQInteger sq_gettop(HSQUIRRELVM v);
SQUIRREL_API void sq_settop(HSQUIRRELVM v,SQInteger newtop);
SQUIRREL_API SQRESULT sq_reservestack(HSQUIRRELVM v,SQInteger nsize);
SQUIRREL_API SQInteger sq_cmp(HSQUIRRELVM v);
SQUIRREL_API bool sq_cmpraw(HSQUIRRELVM v, HSQOBJECT &lhs, HSQOBJECT &rhs, SQInteger &res);
SQUIRREL_API void sq_move(HSQUIRRELVM dest,HSQUIRRELVM src,SQInteger idx);

/*object creation handling*/
SQUIRREL_API SQUserPointer sq_newuserdata(HSQUIRRELVM v,SQUnsignedInteger size);
SQUIRREL_API void sq_newtable(HSQUIRRELVM v);
SQUIRREL_API void sq_newtableex(HSQUIRRELVM v,SQInteger initialcapacity);
SQUIRREL_API void sq_newarray(HSQUIRRELVM v,SQInteger size);
SQUIRREL_API void sq_newclosure(HSQUIRRELVM v,SQFUNCTION func,SQUnsignedInteger nfreevars);
SQUIRREL_API SQRESULT sq_setparamscheck(HSQUIRRELVM v,SQInteger nparamscheck,const SQChar *typemask);
SQUIRREL_API SQRESULT sq_bindenv(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API void sq_pushstring(HSQUIRRELVM v,const SQChar *s,SQInteger len);
SQUIRREL_API void sq_pushfloat(HSQUIRRELVM v,SQFloat f);
SQUIRREL_API void sq_pushinteger(HSQUIRRELVM v,SQInteger n);
SQUIRREL_API void sq_pushbool(HSQUIRRELVM v,SQBool b);
SQUIRREL_API void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p);
SQUIRREL_API void sq_pushnull(HSQUIRRELVM v);
SQUIRREL_API void sq_pushthread(HSQUIRRELVM v, HSQUIRRELVM thread);
SQUIRREL_API SQObjectType sq_gettype(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_typeof(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQInteger sq_getsize(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQHash sq_gethash(HSQUIRRELVM v, SQInteger idx);
SQUIRREL_API SQRESULT sq_getbase(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQBool sq_instanceof(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_tostring(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API void sq_tobool(HSQUIRRELVM v, SQInteger idx, SQBool *b);
SQUIRREL_API SQRESULT sq_getstringandsize(HSQUIRRELVM v,SQInteger idx,const SQChar **c,SQInteger *size);
SQUIRREL_API SQRESULT sq_getstring(HSQUIRRELVM v,SQInteger idx,const SQChar **c);
SQUIRREL_API SQRESULT sq_getinteger(HSQUIRRELVM v,SQInteger idx,SQInteger *i);
SQUIRREL_API SQRESULT sq_getfloat(HSQUIRRELVM v,SQInteger idx,SQFloat *f);
SQUIRREL_API SQRESULT sq_getbool(HSQUIRRELVM v,SQInteger idx,SQBool *b);
SQUIRREL_API SQRESULT sq_getthread(HSQUIRRELVM v,SQInteger idx,HSQUIRRELVM *thread);
SQUIRREL_API SQRESULT sq_getuserpointer(HSQUIRRELVM v,SQInteger idx,SQUserPointer *p);
SQUIRREL_API SQRESULT sq_getuserdata(HSQUIRRELVM v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag);
SQUIRREL_API SQRESULT sq_settypetag(HSQUIRRELVM v,SQInteger idx,SQUserPointer typetag);
SQUIRREL_API SQRESULT sq_gettypetag(HSQUIRRELVM v,SQInteger idx,SQUserPointer *typetag);
SQUIRREL_API void sq_setreleasehook(HSQUIRRELVM v,SQInteger idx,SQRELEASEHOOK hook);
SQUIRREL_API SQRELEASEHOOK sq_getreleasehook(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQChar *sq_getscratchpad(HSQUIRRELVM v,SQInteger minsize);
SQUIRREL_API SQRESULT sq_getfunctioninfo(HSQUIRRELVM v,SQInteger level,SQFunctionInfo *fi);
SQUIRREL_API SQRESULT sq_getclosureinfo(HSQUIRRELVM v,SQInteger idx,SQInteger *nparams,SQInteger *nfreevars);
SQUIRREL_API SQRESULT sq_getclosurename(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_setnativeclosurename(HSQUIRRELVM v,SQInteger idx,const SQChar *name);
SQUIRREL_API SQRESULT sq_setnativeclosuredocstring(HSQUIRRELVM v,SQInteger idx,const SQChar *docstring);
SQUIRREL_API SQRESULT sq_setinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer p);
SQUIRREL_API SQRESULT sq_getinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p,SQUserPointer typetag);
SQUIRREL_API SQRESULT sq_setclassudsize(HSQUIRRELVM v, SQInteger idx, SQInteger udsize);
SQUIRREL_API SQRESULT sq_newclass(HSQUIRRELVM v,SQBool hasbase);
SQUIRREL_API SQRESULT sq_createinstance(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_getclass(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API void sq_weakref(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_getdefaultdelegate(HSQUIRRELVM v,SQObjectType t);
SQUIRREL_API SQRESULT sq_getmemberhandle(HSQUIRRELVM v,SQInteger idx,HSQMEMBERHANDLE *handle);
SQUIRREL_API SQRESULT sq_getbyhandle(HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle);
SQUIRREL_API SQRESULT sq_setbyhandle(HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle);

/*object manipulation*/
SQUIRREL_API void sq_pushroottable(HSQUIRRELVM v);
SQUIRREL_API void sq_pushregistrytable(HSQUIRRELVM v);
SQUIRREL_API void sq_pushconsttable(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_setroottable(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_setconsttable(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx, SQBool bstatic); //-V1071
SQUIRREL_API SQRESULT sq_deleteslot(HSQUIRRELVM v,SQInteger idx,SQBool pushval); //-V1071
SQUIRREL_API SQRESULT sq_set(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_get(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_get_noerr(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_rawget(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_rawget_noerr(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_rawset(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_rawdeleteslot(HSQUIRRELVM v,SQInteger idx,SQBool pushval);
SQUIRREL_API SQRESULT sq_newmember(HSQUIRRELVM v,SQInteger idx,SQBool bstatic);
SQUIRREL_API SQRESULT sq_arrayappend(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_arraypop(HSQUIRRELVM v,SQInteger idx,SQBool pushval);
SQUIRREL_API SQRESULT sq_arrayresize(HSQUIRRELVM v,SQInteger idx,SQInteger newsize);
SQUIRREL_API SQRESULT sq_arrayreverse(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_arrayremove(HSQUIRRELVM v,SQInteger idx,SQInteger itemidx);
SQUIRREL_API SQRESULT sq_arrayinsert(HSQUIRRELVM v,SQInteger idx,SQInteger destpos);
SQUIRREL_API SQRESULT sq_setdelegate(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_getdelegate(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_clone(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_setfreevariable(HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval);
SQUIRREL_API SQRESULT sq_next(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_getweakrefval(HSQUIRRELVM v,SQInteger idx);
SQUIRREL_API SQRESULT sq_clear(HSQUIRRELVM v,SQInteger idx,SQBool freemem = SQTrue);
SQUIRREL_API SQRESULT sq_freeze(HSQUIRRELVM v, SQInteger idx);
SQUIRREL_API SQRESULT sq_freeze_inplace(HSQUIRRELVM v, SQInteger idx);

/*calls*/
SQUIRREL_API SQRESULT sq_call(HSQUIRRELVM v,SQInteger params,SQBool retval,SQBool invoke_err_handler);
SQUIRREL_API SQRESULT sq_resume(HSQUIRRELVM v,SQBool retval,SQBool invoke_err_handler);
SQUIRREL_API const SQChar *sq_getlocal(HSQUIRRELVM v,SQUnsignedInteger level,SQUnsignedInteger idx);
SQUIRREL_API SQRESULT sq_getcallee(HSQUIRRELVM v);
SQUIRREL_API const SQChar *sq_getfreevariable(HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval);
SQUIRREL_API void sq_throwparamtypeerror(HSQUIRRELVM v, SQInteger nparam, SQInteger typemask, SQInteger type);
SQUIRREL_API SQRESULT sq_throwerror(HSQUIRRELVM v,const SQChar *err);
SQUIRREL_API SQRESULT sq_throwobject(HSQUIRRELVM v);
SQUIRREL_API void sq_reseterror(HSQUIRRELVM v);
SQUIRREL_API void sq_getlasterror(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_tailcall(HSQUIRRELVM v, SQInteger nparams);

/*raw object handling*/
SQUIRREL_API SQRESULT sq_getstackobj(HSQUIRRELVM v,SQInteger idx,HSQOBJECT *po);//-V1071
SQUIRREL_API void sq_pushobject(HSQUIRRELVM v,HSQOBJECT obj);
SQUIRREL_API void sq_addref(HSQUIRRELVM v,HSQOBJECT *po);
SQUIRREL_API SQBool sq_release(HSQUIRRELVM v,HSQOBJECT *po);
SQUIRREL_API SQUnsignedInteger sq_getrefcount(HSQUIRRELVM v,HSQOBJECT *po);
SQUIRREL_API void sq_resetobject(HSQOBJECT *po);
SQUIRREL_API const SQChar *sq_objtostring(const HSQOBJECT *o);
SQUIRREL_API SQBool sq_objtobool(const HSQOBJECT *o);
SQUIRREL_API SQInteger sq_objtointeger(const HSQOBJECT *o);
SQUIRREL_API SQFloat sq_objtofloat(const HSQOBJECT *o);
SQUIRREL_API SQUserPointer sq_objtouserpointer(const HSQOBJECT *o);
SQUIRREL_API SQRESULT sq_getobjtypetag(const HSQOBJECT *o,SQUserPointer * typetag);
SQUIRREL_API SQUnsignedInteger sq_getvmrefcount(HSQUIRRELVM v, const HSQOBJECT *po);
SQUIRREL_API const SQChar* sq_objtypestr(SQObjectType tp);

SQUIRREL_API SQBool sq_tracevar(HSQUIRRELVM v, const HSQOBJECT * container, const HSQOBJECT * key, SQChar * buf, int buf_size);

/*GC*/
SQUIRREL_API SQInteger sq_collectgarbage(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sq_resurrectunreachable(HSQUIRRELVM v);

/*serialization*/
SQUIRREL_API SQRESULT sq_writeclosure(HSQUIRRELVM vm,SQWRITEFUNC writef,SQUserPointer up);
SQUIRREL_API SQRESULT sq_readclosure(HSQUIRRELVM vm,SQREADFUNC readf,SQUserPointer up);

SQUIRREL_API SQRESULT sq_limitthreadaccess(HSQUIRRELVM vm, int64_t tid);
SQUIRREL_API bool sq_canaccessfromthisthread(HSQUIRRELVM vm);

typedef struct SQAllocContextT * SQAllocContext;
SQUIRREL_API SQAllocContext sq_getallocctx(HSQUIRRELVM v);

/*mem allocation*/
SQUIRREL_API void *sq_malloc(SQAllocContext ctx, SQUnsignedInteger size);
SQUIRREL_API void *sq_realloc(SQAllocContext ctx, void* p,SQUnsignedInteger oldsize,SQUnsignedInteger newsize);
SQUIRREL_API void sq_free(SQAllocContext ctx, void *p,SQUnsignedInteger size);

/*debug*/
SQUIRREL_API SQRESULT sq_stackinfos(HSQUIRRELVM v,SQInteger level,SQStackInfos *si);
SQUIRREL_API void sq_setdebughook(HSQUIRRELVM v);
SQUIRREL_API void sq_setnativedebughook(HSQUIRRELVM v,SQDEBUGHOOK hook);
SQUIRREL_API SQGETTHREAD sq_set_thread_id_function(HSQUIRRELVM v, SQGETTHREAD func);
SQUIRREL_API SQSQCALLHOOK sq_set_sq_call_hook(HSQUIRRELVM v, SQSQCALLHOOK hook);
SQUIRREL_API SQCOMPILELINEHOOK sq_set_compile_line_hook(HSQUIRRELVM v, SQCOMPILELINEHOOK hook);
SQUIRREL_API void sq_forbidglobalconstrewrite(HSQUIRRELVM v, SQBool on);

/*static analysis*/
SQUIRREL_API void sq_resetanalyzerconfig();
SQUIRREL_API bool sq_loadanalyzerconfig(const char *configFileName);
SQUIRREL_API bool sq_loadanalyzerconfigblk(const KeyValueFile &config);

SQUIRREL_API bool sq_setdiagnosticstatebyname(const char *diagId, bool val);
SQUIRREL_API bool sq_setdiagnosticstatebyid(int32_t id, bool val);
SQUIRREL_API void sq_invertwarningsstate();
SQUIRREL_API void sq_printwarningslist(FILE *ostream);
SQUIRREL_API void sq_enablesyntaxwarnings(bool on);
SQUIRREL_API void sq_checkglobalnames(HSQUIRRELVM v);
SQUIRREL_API void sq_mergeglobalnames(const HSQOBJECT *bindings);


/*UTILITY MACRO*/
#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
#define sq_istable(o) ((o)._type==OT_TABLE)
#define sq_isarray(o) ((o)._type==OT_ARRAY)
#define sq_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define sq_isclosure(o) ((o)._type==OT_CLOSURE)
#define sq_isgenerator(o) ((o)._type==OT_GENERATOR)
#define sq_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define sq_isstring(o) ((o)._type==OT_STRING)
#define sq_isinteger(o) ((o)._type==OT_INTEGER)
#define sq_isfloat(o) ((o)._type==OT_FLOAT)
#define sq_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define sq_isuserdata(o) ((o)._type==OT_USERDATA)
#define sq_isthread(o) ((o)._type==OT_THREAD)
#define sq_isnull(o) ((o)._type==OT_NULL)
#define sq_isclass(o) ((o)._type==OT_CLASS)
#define sq_isinstance(o) ((o)._type==OT_INSTANCE)
#define sq_isbool(o) ((o)._type==OT_BOOL)
#define sq_isweakref(o) ((o)._type==OT_WEAKREF)
#define sq_type(o) ((o)._type)
#define sq_objflags(o) ((o)._flags)


#define SQ_OK (0)
#define SQ_ERROR (-1)

#define SQ_FAILED(res) ((res)<0)
#define SQ_SUCCEEDED(res) ((res)>=0)

#if defined(__GNUC__) || defined(__clang__)
# define SQ_UNUSED_ARG(x) x __attribute__((__unused__))
#else
# define SQ_UNUSED_ARG(x)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQUIRREL_H_*/
