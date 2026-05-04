/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "squserdata.h"
#include "sqclass.h"
#include "compiler/sqtypeparser.h"
#include "compiler/compiler.h"
#include "compiler/sqfuncstate.h"
#include "compiler/arena.h"
#include "compiler/ast.h"
#include "ast_tools/ast_indent_render.h"
#include "compiler/static_analyzer/analyzer.h"
#include "compiler/static_analyzer/config.h"

static_assert(sizeof(SQObject) == sizeof(SQObjectPtr),
    "SQObjectPtr must not add data members beyond SQObject");

SQUIRREL_API SQBool sq_tracevar(HSQUIRRELVM v, const HSQOBJECT *container, const HSQOBJECT * key, char * buf, int buf_size)
{
  return v->GetVarTrace(SQObjectPtr(*container), SQObjectPtr(*key), buf, buf_size);
}

static bool sq_aux_gettypedarg(HSQUIRRELVM v,SQInteger idx,SQObjectType type,SQObjectPtr **o)
{
    *o = &stack_get(v,idx);
    if(sq_type(**o) != type){
        SQObjectPtr oval(v->PrintObjVal(**o));
        v->Raise_Error("wrong argument type, expected '%s' got '%s'",IdType2Name(type),_stringval(oval));
        return false;
    }
    return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { if(!sq_aux_gettypedarg(v,idx,type,&o)) return SQ_ERROR; }

#define sq_aux_paramscheck(v,count) \
{ \
    if(sq_gettop(v) < count){ v->Raise_Error("not enough params in the stack"); return SQ_ERROR; }\
}

using namespace SQCompilation;

SQInteger sq_aux_invalidtype(HSQUIRRELVM v,SQObjectType type)
{
    SQUnsignedInteger buf_size = 100 *sizeof(char);
    scsprintf(_ss(v)->GetScratchPad(buf_size), buf_size, "unexpected type %s", IdType2Name(type));
    return sq_throwerror(v, _ss(v)->GetScratchPad(-1));
}

HSQUIRRELVM sq_open(SQInteger initialstacksize)
{
    SQAllocContext allocctx = nullptr;
    sq_vm_init_alloc_context(&allocctx);

    SQSharedState *ss = (SQSharedState *)SQ_MALLOC(allocctx, sizeof(SQSharedState));
    new (ss) SQSharedState(allocctx);
    ss->Init();

    SQVM *v = (SQVM *)SQ_MALLOC(allocctx, sizeof(SQVM));
    new (v) SQVM(ss);
    ss->_root_vm = v;
    sq_vm_assign_to_alloc_context(allocctx, v);
    if(v->Init(NULL, initialstacksize)) {
        return v;
    }
    sq_delete(allocctx, v, SQVM);
    sq_vm_destroy_alloc_context(&allocctx);
    return NULL;
}

HSQUIRRELVM sq_newthread(HSQUIRRELVM friendvm, SQInteger initialstacksize)
{
    SQSharedState *ss;
    SQVM *v;
    ss=_ss(friendvm);

    v= (SQVM *)SQ_MALLOC(ss->_alloc_ctx, sizeof(SQVM));
    new (v) SQVM(ss);

    if(v->Init(friendvm, initialstacksize)) {
        friendvm->Push(SQObjectPtr(v));
        return v;
    } else {
        sq_delete(ss->_alloc_ctx, v, SQVM);
        return NULL;
    }
}

SQInteger sq_getvmstate(HSQUIRRELVM v)
{
    if(v->_suspended)
        return SQ_VMSTATE_SUSPENDED;
    else {
        if(v->_callsstacksize != 0) return SQ_VMSTATE_RUNNING;
        else return SQ_VMSTATE_IDLE;
    }
}

void sq_seterrorhandler(HSQUIRRELVM v)
{
    SQObject o = stack_get(v, -1);
    if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
        v->_errorhandler = o;
        v->Pop();
    }
}

HSQOBJECT sq_geterrorhandler(HSQUIRRELVM v)
{
  return v->_errorhandler;
}


SQGETTHREAD sq_set_thread_id_function(HSQUIRRELVM v, SQGETTHREAD func)
{
    SQGETTHREAD res = v->_get_current_thread_id_func;
    v->_get_current_thread_id_func = func;
    return res;
}

SQSQCALLHOOK sq_set_sq_call_hook(HSQUIRRELVM v, SQSQCALLHOOK hook)
{
    SQSQCALLHOOK res = v->_sq_call_hook;
    v->_sq_call_hook = hook;
    return res;
}

SQWATCHDOGHOOK sq_set_watchdog_hook(HSQUIRRELVM v, SQWATCHDOGHOOK hook)
{
    SQWATCHDOGHOOK res = v->_watchdog_hook;
    v->_watchdog_hook = hook;
    return res;
}

void sq_kick_watchdog(HSQUIRRELVM v)
{
    if (v->_watchdog_hook)
        v->_watchdog_hook(v, true);
}

SQInteger sq_set_watchdog_timeout_msec(HSQUIRRELVM v, SQInteger timeout)
{
    SQInteger prevTimeout = _ss(v)->watchdog_threshold_msec;
    _ss(v)->watchdog_threshold_msec = SQUnsignedInteger32(timeout < 0 ? 0 : timeout);
    sq_kick_watchdog(v);
    return prevTimeout;
}



void sq_forbidglobalconstrewrite(HSQUIRRELVM v, SQBool on)
{
    if (on)
        _ss(v)->defaultLangFeatures |= LF_FORBID_GLOBAL_CONST_REWRITE;
    else
        _ss(v)->defaultLangFeatures &= ~LF_FORBID_GLOBAL_CONST_REWRITE;
}

void sq_setnativedebughook(HSQUIRRELVM v,SQDEBUGHOOK hook)
{
    v->_debughook_native = hook;
    v->_debughook_closure.Null();
    v->_debughook = hook?true:false;
}

void sq_setdebughook(HSQUIRRELVM v)
{
    SQObject o = stack_get(v,-1);
    if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
        v->_debughook_closure = o;
        v->_debughook_native = NULL;
        v->_debughook = !sq_isnull(o);
        v->Pop();
    }
}

SQCOMPILELINEHOOK sq_set_compile_line_hook(HSQUIRRELVM v, SQCOMPILELINEHOOK hook)
{
    SQCOMPILELINEHOOK res = v->_compile_line_hook;
    v->_compile_line_hook = hook;
    return res;
}

void sq_close(HSQUIRRELVM v)
{
    SQSharedState *ss = _ss(v);
    _thread(ss->_root_vm)->Finalize();
    SQAllocContext allocctx = ss->_alloc_ctx;
    sq_delete(allocctx, ss, SQSharedState);
    sq_vm_destroy_alloc_context(&allocctx);
}

SQRESULT sq_compile(HSQUIRRELVM v, const char *s, SQInteger size, const char *sourcename, SQBool raiseerror, const HSQOBJECT *bindings)
{
    SQObjectPtr o;
#ifndef NO_COMPILER
    if (Compile(v, s, size, bindings, sourcename, o, raiseerror ? true : false)) {
        v->Push(SQObjectPtr(SQClosure::Create(_ss(v), _funcproto(o))));
        return SQ_OK;
    }
    return SQ_ERROR;
#else
    return sq_throwerror(v,"this is a no compiler build");
#endif
}

void sq_lineinfo_in_expressions(HSQUIRRELVM v, SQBool enable)
{
    _ss(v)->_lineInfoInExpressions = enable ? true : false;
}

void sq_enablevartrace(HSQUIRRELVM v, SQBool enable)
{
    _ss(v)->_varTraceEnabled = enable ? true : false;
}

SQBool sq_isvartracesupported()
{
#if SQ_VAR_TRACE_ENABLED == 1
    return SQTrue;
#else
    return SQFalse;
#endif
}

void sq_setcompilationoption(HSQUIRRELVM v, enum CompilationOptions co, bool value) {
  if (value)
    _ss(v)->enableCompilationOption(co);
  else
    _ss(v)->disableCompilationOption(co);
}

bool sq_checkcompilationoption(HSQUIRRELVM v, enum CompilationOptions co) {
  return _ss(v)->checkCompilationOption(co);
}

void sq_notifyallexceptions(HSQUIRRELVM v, SQBool enable)
{
    _ss(v)->_notifyallexceptions = enable?true:false;
}

void sq_addref_refcounted(HSQUIRRELVM v,HSQOBJECT *po)
{
    assert(ISREFCOUNTED(sq_type(*po)));
#ifdef NO_GARBAGE_COLLECTOR
    __AddRef(po->_type,po->_unVal);
#else
    _ss(v)->_refs_table.AddRef(*po);
#endif
}

SQUnsignedInteger sq_getrefcount(HSQUIRRELVM v,HSQOBJECT *po)
{
    if(!ISREFCOUNTED(sq_type(*po))) return 0;
#ifdef NO_GARBAGE_COLLECTOR
   return po->_unVal.pRefCounted->_uiRef;
#else
   return _ss(v)->_refs_table.GetRefCount(*po);
#endif
}

SQBool sq_release_refcounted(HSQUIRRELVM v,HSQOBJECT *po)
{
    assert(ISREFCOUNTED(sq_type(*po)));
#ifdef NO_GARBAGE_COLLECTOR
    bool ret = (po->_unVal.pRefCounted->_uiRef <= 1) ? SQTrue : SQFalse;
    __Release(po->_type,po->_unVal);
    return ret; //the ret val doesn't work(and cannot be fixed)
#else
    return _ss(v)->_refs_table.Release(*po);
#endif
}

SQUnsignedInteger sq_getvmrefcount(HSQUIRRELVM SQ_UNUSED_ARG(v), const HSQOBJECT *po)
{
    if (!ISREFCOUNTED(sq_type(*po))) return 0;
    return po->_unVal.pRefCounted->_uiRef;
}

const char *sq_objtostring(const HSQOBJECT *o)
{
    if(sq_type(*o) == OT_STRING) {
        return _stringval(*o);
    }
    return NULL;
}

SQInteger sq_objtointeger(const HSQOBJECT *o)
{
    if(sq_isnumeric(*o)) {
        return tointeger(*o);
    }
    return 0;
}

SQFloat sq_objtofloat(const HSQOBJECT *o)
{
    if(sq_isnumeric(*o)) {
        return tofloat(*o);
    }
    return 0;
}

SQBool sq_objtobool(const HSQOBJECT *o)
{
    if(sq_isbool(*o)) {
        return _integer(*o);
    }
    return SQFalse;
}

SQBool sq_obj_is_true(const HSQOBJECT *o)
{
    const SQObjectPtr &objPtr = static_cast<const SQObjectPtr &>(*o);
    return SQVM::IsFalse(objPtr) ? SQFalse : SQTrue;
}

SQUserPointer sq_objtouserpointer(const HSQOBJECT *o)
{
    if(sq_isuserpointer(*o)) {
        return _userpointer(*o);
    }
    return 0;
}

SQRESULT sq_obj_getuserdata(const HSQOBJECT *obj, SQUserPointer *p, SQUserPointer *typetag)
{
    if (obj->_type != OT_USERDATA)
      return SQ_ERROR;

    (*p) = _userdataval(*obj);
    if(typetag) *typetag = _userdata(*obj)->_typetag;
    return SQ_OK;
}

const char* sq_objtypestr(SQObjectType tp)
{
    const char* s = IdType2Name(tp);
    return s ? s : "<unknown>";
}

void sq_getregistrytableobj(HSQUIRRELVM v, HSQOBJECT *out)
{
    *out = _ss(v)->_registry;
}

SQRESULT sq_obj_get(HSQUIRRELVM v, const HSQOBJECT *obj, const HSQOBJECT *slot,
                      HSQOBJECT *out, bool raw)
{
    v->ValidateThreadAccess();
    const SQObjectPtr &selfPtr = static_cast<const SQObjectPtr &>(*obj);
    const SQObjectPtr &slotPtr = static_cast<const SQObjectPtr &>(*slot);
    SQObjectPtr outPtr;

    SQUnsignedInteger getFlags = raw ?
      (GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW) : GET_FLAG_DO_NOT_RAISE_ERROR;

    bool res = v->Get(selfPtr, slotPtr, outPtr, getFlags);
    if (res) {
        *out = outPtr;
        v->Push(outPtr);  // keep result alive on VM stack; caller must sq_poptop()
    }
    return res ? SQ_OK : SQ_ERROR;
}

SQBool sq_obj_cmp(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b, SQInteger *res)
{
    const SQObjectPtr &aPtr = static_cast<const SQObjectPtr &>(*a);
    const SQObjectPtr &bPtr = static_cast<const SQObjectPtr &>(*b);

    return v->ObjCmp(aPtr, bPtr, *res);
}

bool sq_obj_is_equal(HSQUIRRELVM v, const HSQOBJECT *a, const HSQOBJECT *b)
{
    return v->IsEqual(*a, *b);
}

SQInteger sq_obj_getsize(const HSQOBJECT *obj)
{
    switch (obj->_type) {
    case OT_TABLE:    return _table(*obj)->CountUsed();
    case OT_ARRAY:    return _array(*obj)->Size();
    case OT_STRING:   return _string(*obj)->_len;
    case OT_USERDATA: return _userdata(*obj)->_size;
    case OT_INSTANCE: return _instance(*obj)->_class->_udsize;
    case OT_CLASS:    return _class(*obj)->_udsize;
    default:          return SQ_ERROR;
    }
}

static SQRESULT getinstanceup_impl(SQInstance *inst, SQUserPointer *p,
                                   SQUserPointer typetag)
{
    *p = inst->_userpointer;
    if (typetag != 0) {
        SQClass *cl = inst->_class;
        do {
            if (cl->_typetag == typetag)
                return SQ_OK;
            cl = cl->_base;
        } while (cl != NULL);
        return SQ_ERROR;
    }
    return SQ_OK;
}

SQRESULT sq_obj_getinstanceup(const HSQOBJECT *obj, SQUserPointer *p,
                              SQUserPointer typetag)
{
    if (obj->_type != OT_INSTANCE)
        return SQ_ERROR;
    return getinstanceup_impl(_instance(*obj), p, typetag);
}

// Shared raw-set logic for sq_rawset and sq_obj_set(raw=true).
// Does NOT check immutability - callers handle that with their own error reporting.
static SQRESULT rawset_impl(HSQUIRRELVM v, const SQObjectPtr &self,
                            const SQObjectPtr &key, const SQObjectPtr &val)
{
    switch (sq_type(self)) {
    case OT_TABLE:
        _table(self)->NewSlot(key, val);
        return SQ_OK;
    case OT_CLASS:
        _class(self)->NewSlot(_ss(v), key, val, false);
        return SQ_OK;
    case OT_INSTANCE:
        return _instance(self)->Set(key, val) == SLOT_STATUS_OK ? SQ_OK : SQ_ERROR;
    case OT_ARRAY:
        if (sq_isnumeric(key))
            return _array(self)->Set(tointeger(key), val) ? SQ_OK : SQ_ERROR;
        return SQ_ERROR;
    default:
        return SQ_ERROR;
    }
}

SQRESULT sq_obj_set(HSQUIRRELVM v, const HSQOBJECT *obj,
                    const HSQOBJECT *key, const HSQOBJECT *val,
                    bool raw)
{
    v->ValidateThreadAccess();
    const SQObjectPtr &selfPtr = static_cast<const SQObjectPtr &>(*obj);
    const SQObjectPtr &keyPtr  = static_cast<const SQObjectPtr &>(*key);
    const SQObjectPtr &valPtr  = static_cast<const SQObjectPtr &>(*val);

    if (raw) {
        if (obj->_flags & SQOBJ_FLAG_IMMUTABLE)
            return SQ_ERROR;
        return rawset_impl(v, selfPtr, keyPtr, valPtr);
    }
    return v->Set(selfPtr, keyPtr, valPtr) ? SQ_OK : SQ_ERROR;
}

SQRESULT sq_obj_newslot(HSQUIRRELVM v, const HSQOBJECT *obj,
                        const HSQOBJECT *key, const HSQOBJECT *val,
                        bool bstatic)
{
    v->ValidateThreadAccess();
    SQObjectType tp = sq_type(*obj);
    if (tp != OT_TABLE && tp != OT_CLASS && tp != OT_INSTANCE)
        return SQ_ERROR;

    const SQObjectPtr &selfPtr = static_cast<const SQObjectPtr &>(*obj);
    const SQObjectPtr &keyPtr  = static_cast<const SQObjectPtr &>(*key);
    const SQObjectPtr &valPtr  = static_cast<const SQObjectPtr &>(*val);

    if (!v->NewSlot(selfPtr, keyPtr, valPtr, bstatic))
        return SQ_ERROR;
    return SQ_OK;
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


void sq_pushnull(HSQUIRRELVM v)
{
    v->PushNull();
}

void sq_pushstring(HSQUIRRELVM v,const char *s,SQInteger len)
{
    if(s || len==0)
        v->Push(SQObjectPtr(SQString::Create(_ss(v), s, len)));
    else v->PushNull();
}

void sq_pushinteger(HSQUIRRELVM v,SQInteger n)
{
    v->Push(SQObjectPtr(n));
}

void sq_pushbool(HSQUIRRELVM v,SQBool b)
{
    v->Push(SQObjectPtr(b?true:false));
}

void sq_pushfloat(HSQUIRRELVM v,SQFloat n)
{
    v->Push(SQObjectPtr(n));
}

void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p)
{
    v->Push(SQObjectPtr(p));
}

void sq_pushthread(HSQUIRRELVM v, HSQUIRRELVM thread)
{
    v->Push(SQObjectPtr(thread));
}

SQUserPointer sq_newuserdata(HSQUIRRELVM v,SQUnsignedInteger size)
{
    v->ValidateThreadAccess();
    SQUserData *ud = SQUserData::Create(_ss(v), size + SQ_ALIGNMENT);
    v->Push(SQObjectPtr(ud));
    return (SQUserPointer)sq_aligning(ud + 1);
}

void sq_newtable(HSQUIRRELVM v)
{
    v->ValidateThreadAccess();
    v->Push(SQObjectPtr(SQTable::Create(_ss(v), 0)));
}

void sq_newtableex(HSQUIRRELVM v,SQInteger initialcapacity)
{
    v->Push(SQObjectPtr(SQTable::Create(_ss(v), initialcapacity)));
}

void sq_newarray(HSQUIRRELVM v,SQInteger size)
{
    v->Push(SQObjectPtr(SQArray::Create(_ss(v), size)));
}

SQRESULT sq_newclass(HSQUIRRELVM v,SQBool hasbase)
{
    SQClass *baseclass = NULL;
    if(hasbase) {
        SQObjectPtr &base = stack_get(v,-1);
        if(sq_type(base) != OT_CLASS)
            return sq_throwerror(v,"invalid base type");
        baseclass = _class(base);
    }
    SQClass *newclass = SQClass::Create(v, baseclass);
    if(baseclass) v->Pop();
    if (newclass) {
        v->Push(SQObjectPtr(newclass));
        return SQ_OK;
    }
    else
        return SQ_ERROR; // propagate the raised error
}

SQBool sq_instanceof(HSQUIRRELVM v)
{
    SQObjectPtr &obj = stack_get(v,-1);
    SQObjectPtr &cl = stack_get(v,-2);
    if(sq_type(cl) != OT_CLASS)
        return sq_throwerror(v,"invalid param type");

    SQClass *cls = _class(cl);
    bool result = false;

    if (sq_type(obj) == OT_INSTANCE) {
        result = _instance(obj)->InstanceOf(cls);
    } else if (cls->_is_builtin_type) {
        SQObjectType obj_type = sq_type(obj);
        // both closures and native closures map to Function class
        if (cls->_builtin_type_id == OT_CLOSURE && (obj_type == OT_CLOSURE || obj_type == OT_NATIVECLOSURE)) {
            result = true;
        } else {
            result = (obj_type == cls->_builtin_type_id);
        }
    }

    return result ? SQTrue : SQFalse;
}

SQRESULT sq_arrayappend(HSQUIRRELVM v,SQInteger idx)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v,2);
    SQObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    _array(*arr)->Append(v->GetUp(-1));
    v->Pop();
    return SQ_OK;
}

SQRESULT sq_arraypop(HSQUIRRELVM v,SQInteger idx,SQBool pushval)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 1);
    SQObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    if(_array(*arr)->Size() > 0) {
        if(pushval != 0){ v->Push(_array(*arr)->Top()); }
        _array(*arr)->Pop();
        return SQ_OK;
    }
    return sq_throwerror(v, "empty array");
}

SQRESULT sq_arrayresize(HSQUIRRELVM v,SQInteger idx,SQInteger newsize)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v,1);
    SQObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    if(newsize >= 0) {
        _array(*arr)->Resize(newsize);
        return SQ_OK;
    }
    return sq_throwerror(v,"negative size");
}


SQRESULT sq_arrayreverse(HSQUIRRELVM v,SQInteger idx)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 1);
    SQObjectPtr *o;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,o);
    SQArray *arr = _array(*o);
    if(arr->Size() > 0) {
        SQObjectPtr t;
        SQInteger size = arr->Size();
        SQInteger n = size >> 1; size -= 1;
        for(SQInteger i = 0; i < n; i++) {
            t = arr->_values[i];
            arr->_values[i] = arr->_values[size-i];
            arr->_values[size-i] = t;
        }
        return SQ_OK;
    }
    return SQ_OK;
}

SQRESULT sq_arrayremove(HSQUIRRELVM v,SQInteger idx,SQInteger itemidx)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 1);
    SQObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    return _array(*arr)->Remove(itemidx) ? SQ_OK : sq_throwerror(v,"index out of range");
}

SQRESULT sq_arrayinsert(HSQUIRRELVM v,SQInteger idx,SQInteger destpos)
{
    sq_aux_paramscheck(v, 1);
    SQObjectPtr *arr;
    _GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
    SQRESULT ret = _array(*arr)->Insert(destpos, v->GetUp(-1)) ? SQ_OK : sq_throwerror(v,"index out of range");
    v->Pop();
    return ret;
}

void sq_newclosure(HSQUIRRELVM v,SQFUNCTION func,SQUnsignedInteger nfreevars)
{
    SQNativeClosure *nc = SQNativeClosure::Create(_ss(v), func,nfreevars);
    nc->_nparamscheck = 0;
    for(SQUnsignedInteger i = 0; i < nfreevars; i++) {
        nc->_outervalues[i] = v->Top();
        v->Pop();
    }
    v->Push(SQObjectPtr(nc));
}

SQRESULT sq_getclosureinfo(HSQUIRRELVM v,SQInteger idx,SQInteger *nparams,SQInteger *nfreevars)
{
    SQObject o = stack_get(v, idx);
    if(sq_type(o) == OT_CLOSURE) {
        SQClosure *c = _closure(o);
        SQFunctionProto *proto = c->_function;
        *nparams = proto->_nparameters;
        *nfreevars = proto->_noutervalues;
        return SQ_OK;
    }
    else if(sq_type(o) == OT_NATIVECLOSURE)
    {
        SQNativeClosure *c = _nativeclosure(o);
        *nparams = c->_nparamscheck;
        *nfreevars = (SQInteger)c->_noutervalues;
        return SQ_OK;
    }
    return sq_throwerror(v,"the object is not a closure");
}

SQRESULT sq_setnativeclosurename(HSQUIRRELVM v,SQInteger idx,const char *name)
{
    SQObject o = stack_get(v, idx);
    if(sq_isnativeclosure(o)) {
        SQNativeClosure *nc = _nativeclosure(o);
        nc->_name = SQString::Create(_ss(v),name);
        return SQ_OK;
    }
    return sq_throwerror(v,"the object is not a nativeclosure");
}

SQRESULT sq_setnativeclosuredocstring(HSQUIRRELVM v,SQInteger idx,const char *docstring)
{
    assert(docstring);
    SQObject o = stack_get(v, idx);
    if(sq_isnativeclosure(o)) {
        SQObjectPtr docValue(SQString::Create(_ss(v), docstring));
        SQObjectPtr docKey;
        docKey._type = OT_USERPOINTER;
        docKey._unVal.pUserPointer = (void *)_nativeclosure(o)->_function;
        _table(_ss(v)->doc_objects)->NewSlot(docKey, docValue);
        return SQ_OK;
    }
    return sq_throwerror(v,"the object is not a nativeclosure");
}

SQRESULT sq_setobjectdocstring(HSQUIRRELVM v, const HSQOBJECT *obj, const char *docstring)
{
    assert(obj);
    assert(docstring);
    if (sq_isclass(*obj) || sq_istable(*obj) || sq_isnativeclosure(*obj) || sq_isclosure(*obj)) {
        SQObjectPtr docValue(SQString::Create(_ss(v), docstring));
        SQObjectPtr docKey;
        docKey._type = OT_USERPOINTER;
        docKey._unVal.pUserPointer =
            sq_isclass(*obj) || sq_istable(*obj) ? (void *)_userpointer(*obj) :
            sq_isnativeclosure(*obj) ? (void *)_nativeclosure(*obj)->_function :
            sq_isclosure(*obj) ? (void *)_closure(*obj)->_function : NULL;
        _table(_ss(v)->doc_objects)->NewSlot(docKey, docValue);
        return SQ_OK;
    }
    return sq_throwerror(v,"the object is not a table, class or function");
}

SQRESULT sq_setparamscheck(HSQUIRRELVM v,SQInteger nparamscheck,const char *typemask)
{
    SQObject o = stack_get(v, -1);
    if(!sq_isnativeclosure(o))
        return sq_throwerror(v, "native closure expected");
    SQNativeClosure *nc = _nativeclosure(o);
    nc->_nparamscheck = nparamscheck;
    if(typemask) {
        SQIntVec res(_ss(v)->_alloc_ctx);
        if(!CompileTypemask(res, typemask))
            return sq_throwerror(v, "invalid typemask");
        nc->_typecheck.copy(res);
    }
    else {
        nc->_typecheck.resize(0);
    }
    if(nparamscheck == SQ_MATCHTYPEMASKSTRING) {
        nc->_nparamscheck = nc->_typecheck.size();
    }
    return SQ_OK;
}

SQRESULT sq_new_closure_slot_from_decl_string(HSQUIRRELVM v, SQFUNCTION func, SQUnsignedInteger nfreevars, const char *function_decl,
    const char *docstring)
{
//  instead of:
//  sq_pushstring(v, function_name, -1);
//  sq_newclosure(v, func_ptr,0);
//  sq_setparamscheck(v, nparamscheck, typemask);
//  sq_setnativeclosurename(v, -1, name);
//  sq_newslot(v, -3, SQFalse);

    SQNativeClosure *nc = SQNativeClosure::Create(_ss(v), func, nfreevars);

    for(SQUnsignedInteger i = 0; i < nfreevars; i++) {
        nc->_outervalues[i] = v->Top();
        v->Pop();
    }

    SQFunctionType ft(_ss(v));
    SQInteger error_pos = 0;
    SQObjectPtr error_string;
    if (!sq_parse_function_type_string(v, function_decl, ft, error_pos, error_string)) {
        SQPRINTFUNCTION printErrorFunc = sq_geterrorfunc(v);
        assert(printErrorFunc && "set 'print error function' using sq_setprintfunc()");
        if (printErrorFunc) {
            printErrorFunc(v, "%s at position %d of type '%s'\n",
                _stringval(error_string), int(error_pos), function_decl);
        }
        assert(0 && "Invalid function type string, see error message in the log");
        return sq_throwerror(v, "invalid function type string");
    }

    sq_pushstring(v, _stringval(ft.functionName), -1);

    nc->_name = ft.functionName;
    nc->_purefunction = ft.pure;
    nc->_nodiscard = ft.nodiscard;

    nc->_typecheck.resize(ft.argTypeMask.size() + 1);
    nc->_typecheck[0] = ft.objectTypeMask;

    for (SQInteger i = 0; i < ft.argTypeMask.size(); i++)
        nc->_typecheck[i + 1] = ft.argTypeMask[i];

    if (ft.ellipsisArgTypeMask != 0 || ft.requiredArgs != ft.argTypeMask.size())
        nc->_nparamscheck = -ft.requiredArgs - 1;
    else
        nc->_nparamscheck = ft.requiredArgs + 1;

    nc->_result_type_mask = ft.returnTypeMask;

#if SQ_STORE_DOC_OBJECTS
    if (docstring) {
        SQObjectPtr docValue(SQString::Create(_ss(v), docstring));
        SQObjectPtr docKey;
        docKey._type = OT_USERPOINTER;
        docKey._unVal.pUserPointer = (void *)func;
        _table(_ss(v)->doc_objects)->NewSlot(docKey, docValue);
    }

    {
        SQObjectPtr declValue(SQString::Create(_ss(v), function_decl));
        SQObjectPtr declKey;
        declKey._type = OT_USERPOINTER;
        declKey._unVal.pUserPointer = (void *)(((size_t)(void *)func) ^ ~size_t(0));
        _table(_ss(v)->doc_objects)->NewSlot(declKey, declValue);
    }
#else
    (void)(docstring);
#endif

    v->Push(SQObjectPtr(nc));

    //printf("%s: typecheck.size() = %d, nparamscheck = %d\n", function_decl,
    //    int(nc->_typecheck.size()), int(nc->_nparamscheck));

    return sq_newslot(v, -3, SQFalse);
}


SQRESULT sq_bindenv(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &o = stack_get(v,idx);
    if(!sq_isnativeclosure(o) &&
        !sq_isclosure(o))
        return sq_throwerror(v,"the target is not a closure");
    SQObjectPtr &env = stack_get(v,-1);
    if(!sq_istable(env) &&
        !sq_isarray(env) &&
        !sq_isclass(env) &&
        !sq_isinstance(env))
        return sq_throwerror(v,"invalid environment");
    SQWeakRef *w = _refcounted(env)->GetWeakRef(_ss(v)->_alloc_ctx, sq_type(env), env._flags);
    SQObjectPtr ret;
    if(sq_isclosure(o)) {
        SQClosure *c = _closure(o)->Clone();
        __ObjRelease(c->_env);
        c->_env = w;
        __ObjAddRef(c->_env);
        if(_closure(o)->_base) {
            c->_base = _closure(o)->_base;
            __ObjAddRef(c->_base);
        }
        ret = c;
    }
    else { //then must be a native closure
        SQNativeClosure *c = _nativeclosure(o)->Clone();
        __ObjRelease(c->_env);
        c->_env = w;
        __ObjAddRef(c->_env);
        ret = c;
    }
    v->Pop();
    v->Push(ret);
    return SQ_OK;
}

SQRESULT sq_getclosurename(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &o = stack_get(v,idx);
    if(!sq_isnativeclosure(o) &&
        !sq_isclosure(o))
        return sq_throwerror(v,"the target is not a closure");
    if(sq_isnativeclosure(o))
    {
        v->Push(_nativeclosure(o)->_name);
    }
    else { //closure
        v->Push(_closure(o)->_function->_name);
    }
    return SQ_OK;
}


SQRESULT sq_clear(HSQUIRRELVM v,SQInteger idx, SQBool freemem)
{
    SQObject &o=stack_get(v,idx);
    switch(sq_type(o)) {
        case OT_TABLE: _table(o)->Clear(freemem);  break;
        case OT_ARRAY: _array(o)->Resize(0, freemem); break;
        default:
            return sq_throwerror(v, "clear only works on table and array");
        break;

    }
    return SQ_OK;
}

void sq_pushroottable(HSQUIRRELVM v)
{
    v->Push(v->_roottable);
}

void sq_pushregistrytable(HSQUIRRELVM v)
{
    v->Push(_ss(v)->_registry);
}

void sq_pushconsttable(HSQUIRRELVM v)
{
    v->Push(_ss(v)->_consts);
}

SQRESULT sq_setroottable(HSQUIRRELVM v)
{
    SQObject o = stack_get(v, -1);
    if(sq_istable(o) || sq_isnull(o)) {
        v->_roottable = o;
        v->Pop();
        return SQ_OK;
    }
    return sq_throwerror(v, "invalid type");
}

SQRESULT sq_setconsttable(HSQUIRRELVM v)
{
    SQObject o = stack_get(v, -1);
    if(sq_istable(o)) {
        _ss(v)->_consts = o;
        v->Pop();
        return SQ_OK;
    }
    return sq_throwerror(v, "invalid type, expected table");
}

void sq_setforeignptr(HSQUIRRELVM v,SQUserPointer p)
{
    v->_foreignptr = p;
}

SQUserPointer sq_getforeignptr(HSQUIRRELVM v)
{
    return v->_foreignptr;
}

void sq_setsharedforeignptr(HSQUIRRELVM v,SQUserPointer p)
{
    _ss(v)->_foreignptr = p;
}

SQUserPointer sq_getsharedforeignptr(HSQUIRRELVM v)
{
    return _ss(v)->_foreignptr;
}

void sq_setvmreleasehook(HSQUIRRELVM v,SQRELEASEHOOK hook)
{
    v->_releasehook = hook;
}

SQRELEASEHOOK sq_getvmreleasehook(HSQUIRRELVM v)
{
    return v->_releasehook;
}

void sq_setsharedreleasehook(HSQUIRRELVM v,SQRELEASEHOOK hook)
{
    _ss(v)->_releasehook = hook;
}

SQRELEASEHOOK sq_getsharedreleasehook(HSQUIRRELVM v)
{
    return _ss(v)->_releasehook;
}

void sq_push(HSQUIRRELVM v,SQInteger idx)
{
    v->Push(stack_get(v, idx));
}

SQObjectType sq_gettype(HSQUIRRELVM v,SQInteger idx)
{
    return sq_type(stack_get(v, idx));
}

SQRESULT sq_typeof(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    SQObjectPtr res;
    if(!v->TypeOf(o,res)) {
        return SQ_ERROR;
    }
    v->Push(res);
    return SQ_OK;
}

SQRESULT sq_tostring(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    SQObjectPtr res;
    if(!v->ToString(o,res)) {
        return SQ_ERROR;
    }
    v->Push(res);
    return SQ_OK;
}

void sq_tobool(HSQUIRRELVM v, SQInteger idx, SQBool *b)
{
    SQObjectPtr &o = stack_get(v, idx);
    *b = SQVM::IsFalse(o)?SQFalse:SQTrue;
}

SQRESULT sq_getinteger(HSQUIRRELVM v,SQInteger idx,SQInteger *i)
{
    SQObjectPtr &o = stack_get(v, idx);
    if(sq_isnumeric(o)) {
        *i = tointeger(o);
        return SQ_OK;
    }
    if(sq_isbool(o)) {
        *i = SQVM::IsFalse(o)?SQFalse:SQTrue;
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQRESULT sq_getfloat(HSQUIRRELVM v,SQInteger idx,SQFloat *f)
{
    SQObjectPtr &o = stack_get(v, idx);
    if(sq_isnumeric(o)) {
        *f = tofloat(o);
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQRESULT sq_getbool(HSQUIRRELVM v,SQInteger idx,SQBool *b)
{
    SQObjectPtr &o = stack_get(v, idx);
    if(sq_isbool(o)) {
        *b = _integer(o);
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQRESULT sq_getstringandsize(HSQUIRRELVM v,SQInteger idx,const char **c,SQInteger *size)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_STRING,o);
    *c = _stringval(*o);
    *size = _string(*o)->_len;
    return SQ_OK;
}

SQRESULT sq_getstring(HSQUIRRELVM v,SQInteger idx,const char **c)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_STRING,o);
    *c = _stringval(*o);
    return SQ_OK;
}

SQRESULT sq_getthread(HSQUIRRELVM v,SQInteger idx,HSQUIRRELVM *thread)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_THREAD,o);
    *thread = _thread(*o);
    return SQ_OK;
}

SQRESULT sq_clone(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &o = stack_get(v,idx);
    v->PushNull();
    if(!v->Clone(o, stack_get(v, -1))){
        v->Pop();
        return SQ_ERROR;
    }
    return SQ_OK;
}

SQInteger sq_getsize(HSQUIRRELVM v, SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    SQInteger res = sq_obj_getsize(&o);
    if (res == SQ_ERROR)
        return sq_aux_invalidtype(v, sq_type(o));
    return res;
}

SQHash sq_gethash(HSQUIRRELVM v, SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    return HashObj(o);
}

SQRESULT sq_getuserdata(HSQUIRRELVM v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_USERDATA,o);
    (*p) = _userdataval(*o);
    if(typetag) *typetag = _userdata(*o)->_typetag;
    return SQ_OK;
}

SQRESULT sq_settypetag(HSQUIRRELVM v,SQInteger idx,SQUserPointer typetag)
{
    SQObjectPtr &o = stack_get(v,idx);
    switch(sq_type(o)) {
        case OT_USERDATA:   _userdata(o)->_typetag = typetag;   break;
        case OT_CLASS:      _class(o)->_typetag = typetag;      break;
        default:            return sq_throwerror(v,"invalid object type");
    }
    return SQ_OK;
}

SQRESULT sq_getobjtypetag(const HSQOBJECT *o,SQUserPointer * typetag)
{
  switch(sq_type(*o)) {
    case OT_INSTANCE: *typetag = _instance(*o)->_class->_typetag; break;
    case OT_USERDATA: *typetag = _userdata(*o)->_typetag; break;
    case OT_CLASS:    *typetag = _class(*o)->_typetag; break;
    default: return SQ_ERROR;
  }
  return SQ_OK;
}

SQRESULT sq_gettypetag(HSQUIRRELVM v,SQInteger idx,SQUserPointer *typetag)
{
    SQObjectPtr &o = stack_get(v,idx);
    if (SQ_FAILED(sq_getobjtypetag(&o, typetag)))
        return SQ_ERROR;// this is not an error it should be a bool but would break backward compatibility
    return SQ_OK;
}

SQRESULT sq_getuserpointer(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_USERPOINTER,o);
    (*p) = _userpointer(*o);
    return SQ_OK;
}

SQRESULT sq_setinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    SQObjectPtr &o = stack_get(v,idx);
    if(sq_type(o) != OT_INSTANCE) return sq_throwerror(v,"the object is not a class instance");
    _instance(o)->_userpointer = p;
    return SQ_OK;
}

SQRESULT sq_setclassudsize(HSQUIRRELVM v, SQInteger idx, SQInteger udsize)
{
    SQObjectPtr &o = stack_get(v,idx);
    if(sq_type(o) != OT_CLASS) return sq_throwerror(v,"the object is not a class");
    if(_class(o)->isLocked()) return sq_throwerror(v,"the class is locked");
    _class(o)->_udsize = udsize;
    return SQ_OK;
}

SQRESULT sq_registernativefield(HSQUIRRELVM v, SQInteger classidx,
    const char *name, SQInteger offset, SQInteger fieldtype)
{
    SQObjectPtr &o = stack_get(v, classidx);

    if (sq_type(o) != OT_CLASS)
        return sq_throwerror(v, "the object is not a class");

    if (fieldtype < 0 || fieldtype > SQNFT_BOOL)
        return sq_throwerror(v, "invalid native field type");

    if (offset < 0 || offset > 0xFFFF)
        return sq_throwerror(v, "native field offset out of range");

    SQClass *cls = _class(o);

    if (cls->isLocked())
        return sq_throwerror(v, "native field cannot be registered on a locked class");

    if (cls->_udsize <= 0)
        return sq_throwerror(v, "native field requires inline userdata (call sq_setclassudsize first)");

    SQObjectPtr key(SQString::Create(_ss(v), name));
    if (!cls->RegisterNativeField(_ss(v), key, (uint16_t)offset, (uint8_t)fieldtype))
        return sq_throwerror(v, "failed to register native field");

    return SQ_OK;
}

SQRESULT sq_getinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag)
{
    SQObjectPtr &o = stack_get(v, idx);
    if (sq_type(o) != OT_INSTANCE)
        return sq_throwerror(v, "the object is not a class instance");
    if (SQ_FAILED(getinstanceup_impl(_instance(o), p, typetag)))
        return sq_throwerror(v, "invalid type tag");
    return SQ_OK;
}

SQInteger sq_gettop(HSQUIRRELVM v)
{
    return (v->_top) - v->_stackbase;
}

void sq_settop(HSQUIRRELVM v, SQInteger newtop)
{
    SQInteger top = sq_gettop(v);
    if(top > newtop)
        sq_pop(v, top - newtop);
    else
        while(top++ < newtop) sq_pushnull(v);
}

void sq_pop(HSQUIRRELVM v, SQInteger nelemstopop)
{
    assert(v->_top >= nelemstopop);
    v->Pop(nelemstopop);
}

void sq_poptop(HSQUIRRELVM v)
{
    assert(v->_top >= 1);
    v->Pop();
}


void sq_remove(HSQUIRRELVM v, SQInteger idx)
{
    v->Remove(idx);
}

SQInteger sq_cmp(HSQUIRRELVM v)
{
    SQInteger res;
    v->ObjCmp(stack_get(v, -1), stack_get(v, -2),res);
    return res;
}

bool sq_cmpraw(HSQUIRRELVM v, HSQOBJECT &lhs, HSQOBJECT &rhs, SQInteger &res)
{
  return v->ObjCmp(SQObjectPtr(lhs), SQObjectPtr(rhs), res);
}


SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx, SQBool bstatic)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 3);
    SQObjectPtr &self = stack_get(v, idx);
    if(sq_type(self) == OT_TABLE || sq_type(self) == OT_CLASS) {
        SQObjectPtr &key = v->GetUp(-2);
        if (!v->NewSlot(self, key, v->GetUp(-1),bstatic?true:false))
            return SQ_ERROR;
        v->Pop(2);
        return SQ_OK;
    }
    else {
        v->Raise_Error("cannot add slot to '%s'", GetTypeName(self));
        return SQ_ERROR;
    }
}

SQRESULT sq_deleteslot(HSQUIRRELVM v,SQInteger idx,SQBool pushval)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 2);
    SQObjectPtr *self;
    _GETSAFE_OBJ(v, idx, OT_TABLE,self);
    SQObjectPtr &key = v->GetUp(-1);
    SQObjectPtr res;
    if(!v->DeleteSlot(*self, key, res)){
        v->Pop();
        return SQ_ERROR;
    }
    if(pushval) v->GetUp(-1) = res;
    else v->Pop();
    return SQ_OK;
}

SQRESULT sq_set(HSQUIRRELVM v,SQInteger idx)
{
    v->ValidateThreadAccess();

    SQObjectPtr &self = stack_get(v, idx);
    if(v->Set(self, v->GetUp(-2), v->GetUp(-1))) {
        v->Pop(2);
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQRESULT sq_rawset(HSQUIRRELVM v,SQInteger idx)
{
    v->ValidateThreadAccess();

    SQObjectPtr &self = stack_get(v, idx);

    if (self._flags & SQOBJ_FLAG_IMMUTABLE) {
        v->Raise_Error("trying to modify immutable '%s'",GetTypeName(self));
        return SQ_ERROR;
    }

    if (SQ_SUCCEEDED(rawset_impl(v, self, v->GetUp(-2), v->GetUp(-1)))) {
        v->Pop(2);
        return SQ_OK;
    }

    SQObjectType tp = sq_type(self);
    if (tp == OT_TABLE || tp == OT_CLASS || tp == OT_INSTANCE || tp == OT_ARRAY) {
        v->Raise_IdxError(v->GetUp(-2));
        return SQ_ERROR;
    }
    v->Pop(2);
    return sq_throwerror(v, "rawset works only on array/table/class and instance");
}

SQRESULT sq_newmember(HSQUIRRELVM v,SQInteger idx,SQBool bstatic)
{
    v->ValidateThreadAccess();

    SQObjectPtr &self = stack_get(v, idx);
    if(sq_type(self) != OT_CLASS) return sq_throwerror(v, "new member only works with classes");
    SQObjectPtr &key = v->GetUp(-2);
    if(!v->NewSlot(self,key,v->GetUp(-1),bstatic?true:false)) {
        v->Pop(2);
        return SQ_ERROR;
    }
    v->Pop(2);
    return SQ_OK;
}


SQRESULT sq_setdelegate(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &self = stack_get(v, idx);
    SQObjectPtr &mt = v->GetUp(-1);
    SQObjectType type = sq_type(self);
    switch(type) {
    case OT_TABLE:
        if(sq_type(mt) == OT_TABLE) {
            if(!_table(self)->SetDelegate(_table(mt))) {
                return sq_throwerror(v, "delagate cycle");
            }
            v->Pop();
        }
        else if(sq_type(mt)==OT_NULL) {
            _table(self)->SetDelegate(NULL); v->Pop(); }
        else return sq_aux_invalidtype(v,type);
        break;
    case OT_USERDATA:
        if(sq_type(mt)==OT_TABLE) {
            _userdata(self)->SetDelegate(_table(mt)); v->Pop(); }
        else if(sq_type(mt)==OT_NULL) {
            _userdata(self)->SetDelegate(NULL); v->Pop(); }
        else return sq_aux_invalidtype(v, type);
        break;
    default:
            return sq_aux_invalidtype(v, type);
        break;
    }
    return SQ_OK;
}

SQRESULT sq_rawdeleteslot(HSQUIRRELVM v,SQInteger idx,SQBool pushval)
{
    v->ValidateThreadAccess();

    sq_aux_paramscheck(v, 2);
    SQObjectPtr *self;
    _GETSAFE_OBJ(v, idx, OT_TABLE,self);

    if (self->_flags & SQOBJ_FLAG_IMMUTABLE) {
        v->Raise_Error("trying to modify immutable '%s'",GetTypeName(*self));
        return SQ_ERROR;
    }

    SQObjectPtr &key = v->GetUp(-1);
    SQObjectPtr t;
    if(_table(*self)->Get(key,t)) {
        _table(*self)->Remove(key);
    }
    if(pushval != 0)
        v->GetUp(-1) = t;
    else
        v->Pop();
    return SQ_OK;
}

SQRESULT sq_getdelegate(HSQUIRRELVM v,SQInteger idx)
{
    v->ValidateThreadAccess();

    SQObjectPtr &self=stack_get(v,idx);
    switch(sq_type(self)){
    case OT_TABLE:
    case OT_USERDATA:
        if(!_delegable(self)->_delegate){
            v->PushNull();
            break;
        }
        v->Push(SQObjectPtr(_delegable(self)->_delegate));
        break;
    default: return sq_throwerror(v,"wrong type"); break;
    }
    return SQ_OK;

}

SQRESULT sq_get(HSQUIRRELVM v,SQInteger idx)
{
    const SQObjectPtr &self = stack_get(v,idx);
    SQObjectPtr &obj = v->GetUp(-1);
    if (v->Get(self,obj,obj,GET_FLAG_DO_NOT_RAISE_ERROR))
        return SQ_OK;
    v->Pop();
    return SQ_ERROR;
}

static inline void propagate_immutable(const SQObject &obj, SQObject &slot_val)
{
    if (sq_objflags(obj) & SQOBJ_FLAG_IMMUTABLE)
        slot_val._flags |= SQOBJ_FLAG_IMMUTABLE;
}

SQRESULT sq_rawget(HSQUIRRELVM v, SQInteger idx)
{
    v->ValidateThreadAccess();

    const SQObjectPtr &self = stack_get(v, idx);
    SQObjectPtr &obj = v->GetUp(-1);
    if (v->Get(self, obj, obj, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW))
        return SQ_OK;
    v->Pop();
    return SQ_ERROR;
}

SQRESULT sq_getstackobj(HSQUIRRELVM v,SQInteger idx,HSQOBJECT *po)
{
    *po=stack_get(v,idx);
    return SQ_OK;
}

const char *sq_getlocal(HSQUIRRELVM v,SQUnsignedInteger level,SQUnsignedInteger idx)
{
    SQUnsignedInteger cstksize=v->_callsstacksize;
    SQUnsignedInteger lvl=(cstksize-level)-1;
    SQInteger stackbase=v->_stackbase;
    if(lvl<cstksize){
        for(SQUnsignedInteger i=0;i<level;i++){
            SQVM::CallInfo &ci=v->_callsstack[(cstksize-i)-1];
            stackbase-=ci._prevstkbase;
        }
        SQVM::CallInfo &ci=v->_callsstack[lvl];
        if(sq_type(ci._closure)!=OT_CLOSURE)
            return NULL;
        SQClosure *c=_closure(ci._closure);
        SQFunctionProto *func=c->_function;
        if(func->_noutervalues > (SQInteger)idx) {
            v->Push(*_outer(c->_outervalues[idx])->_valptr);
            return _stringval(func->_outervalues[idx]._name);
        }
        idx -= func->_noutervalues;
        return func->GetLocal(v,stackbase,idx,(SQInteger)(ci._ip-func->_instructions));
    }
    return NULL;
}

void sq_pushobj(HSQUIRRELVM v,const HSQOBJECT *po)
{
    v->Push(SQObjectPtr(*po));
}

void sq_throwparamtypeerror(HSQUIRRELVM v, SQInteger nparam, SQInteger typemask, SQInteger type)
{
    v->Raise_ParamTypeError(nparam, typemask, type);
}

SQRESULT sq_throwerror(HSQUIRRELVM v,const char *err)
{
    v->_lasterror=SQString::Create(_ss(v),err);
    return SQ_ERROR;
}

SQRESULT sq_throwobject(HSQUIRRELVM v)
{
    v->_lasterror = v->GetUp(-1);
    v->Pop();
    return SQ_ERROR;
}


void sq_reseterror(HSQUIRRELVM v)
{
    v->_lasterror.Null();
}

void sq_getlasterror(HSQUIRRELVM v)
{
    v->Push(v->_lasterror);
}

SQRESULT sq_reservestack(HSQUIRRELVM v,SQInteger nsize)
{
    if (((SQUnsignedInteger)v->_top + nsize) > v->_stack.size()) {
        if(v->_nmetamethodscall) {
            return sq_throwerror(v,"cannot resize stack while in a metamethod");
        }
        v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
    }
    return SQ_OK;
}

SQRESULT sq_resume(HSQUIRRELVM v,SQBool retval,SQBool invoke_err_handler)
{
    if (sq_type(v->GetUp(-1)) == OT_GENERATOR)
    {
        v->PushNull(); //retval
        bool res = v->_debughook ?
            v->Execute<true>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), invoke_err_handler, SQVM::ET_RESUME_GENERATOR) :
            v->Execute<false>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), invoke_err_handler, SQVM::ET_RESUME_GENERATOR);

        if (!res)
        {
            v->Raise_Error(v->_lasterror);
            return SQ_ERROR;
        }
        if(!retval)
            v->Pop();
        return SQ_OK;
    }
    return sq_throwerror(v,"only generators can be resumed");
}

SQRESULT sq_call(HSQUIRRELVM v,SQInteger params,SQBool retval,SQBool invoke_err_handler)
{
    v->ValidateThreadAccess();
    if (v->_sq_call_hook)
        v->_sq_call_hook(v);

    SQObjectPtr res;
    if(!v->Call(v->GetUp(-(params+1)),params,v->_top-params,res,invoke_err_handler?true:false)){
        v->Pop(params); //pop args
        return SQ_ERROR;
    }
    if(!v->_suspended)
        v->Pop(params); //pop args
    if(retval)
        v->Push(res); // push result
    return SQ_OK;
}

SQRESULT sq_tailcall(HSQUIRRELVM v, SQInteger nparams)
{
    v->ValidateThreadAccess();

    SQObjectPtr &res = v->GetUp(-(nparams + 1));
    if (sq_type(res) != OT_CLOSURE) {
        return sq_throwerror(v, "only closure can be tail called");
    }
    SQClosure *clo = _closure(res);
    if (clo->_function->_bgenerator)
    {
        return sq_throwerror(v, "generators cannot be tail called");
    }

    SQInteger stackbase = (v->_top - nparams) - v->_stackbase;
    if (!v->TailCall(clo, stackbase, nparams)) {
        return SQ_ERROR;
    }
    return SQ_TAILCALL_FLAG;
}

SQRESULT sq_suspendvm(HSQUIRRELVM v)
{
    return v->Suspend();
}

SQRESULT sq_wakeupvm(HSQUIRRELVM v,SQBool wakeupret,SQBool retval,SQBool invoke_err_handler,SQBool throwerror)
{
    SQObjectPtr ret;
    if(!v->_suspended)
        return sq_throwerror(v,"cannot resume a vm that is not running any code");
    SQInteger target = v->_suspended_target;
    if(wakeupret) {
        if(target != -1) {
            v->GetAt(v->_stackbase+v->_suspended_target)=v->GetUp(-1); //retval
        }
        v->Pop();
    } else if(target != -1) { v->GetAt(v->_stackbase+v->_suspended_target).Null(); }
    SQObjectPtr dummy;

    bool res = v->_debughook ?
        v->Execute<true>(dummy,-1,-1,ret,invoke_err_handler,throwerror?SQVM::ET_RESUME_THROW_VM : SQVM::ET_RESUME_VM) :
        v->Execute<false>(dummy,-1,-1,ret,invoke_err_handler,throwerror?SQVM::ET_RESUME_THROW_VM : SQVM::ET_RESUME_VM);

    if(!res)
        return SQ_ERROR;

    if(retval)
        v->Push(ret);
    return SQ_OK;
}

void sq_setreleasehook(HSQUIRRELVM v,SQInteger idx,SQRELEASEHOOK hook)
{
    SQObjectPtr &ud=stack_get(v,idx);
    switch(sq_type(ud) ) {
    case OT_USERDATA:   _userdata(ud)->_hook = hook;    break;
    case OT_INSTANCE:   _instance(ud)->_hook = hook;    break;
    case OT_CLASS:      _class(ud)->_hook = hook;       break;
    default: return;
    }
}

SQRELEASEHOOK sq_getreleasehook(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr &ud=stack_get(v,idx);
    switch(sq_type(ud) ) {
    case OT_USERDATA:   return _userdata(ud)->_hook;    break;
    case OT_INSTANCE:   return _instance(ud)->_hook;    break;
    case OT_CLASS:      return _class(ud)->_hook;       break;
    default: return NULL;
    }
}

void sq_setcompilererrorhandler(HSQUIRRELVM v,SQCOMPILERERROR f)
{
    _ss(v)->_compilererrorhandler = f;
}

SQCOMPILERERROR sq_getcompilererrorhandler(HSQUIRRELVM v)
{
    return _ss(v)->_compilererrorhandler;
}

void sq_setcompilerdiaghandler(HSQUIRRELVM v, SQ_COMPILER_DIAG_CB f)
{
    _ss(v)->_compilerdiaghandler = f;
}


SQRESULT sq_writeclosure(HSQUIRRELVM v,SQWRITEFUNC w,SQUserPointer up)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, -1, OT_CLOSURE,o);
    unsigned short tag = SQ_BYTECODE_STREAM_TAG;
    if(_closure(*o)->_function->_noutervalues)
        return sq_throwerror(v,"a closure with free variables bound cannot be serialized");
    if(w(up,&tag,2) != 2)
        return sq_throwerror(v,"io error");
    if(!_closure(*o)->Save(v,up,w))
        return SQ_ERROR;
    return SQ_OK;
}

SQRESULT sq_readclosure(HSQUIRRELVM v,SQREADFUNC r,SQUserPointer up)
{
    SQObjectPtr closure;

    unsigned short tag;
    if(r(up,&tag,2) != 2)
        return sq_throwerror(v,"io error");
    if(tag != SQ_BYTECODE_STREAM_TAG)
        return sq_throwerror(v,"invalid stream");
    if(!SQClosure::Load(v,up,r,closure))
        return SQ_ERROR;
    v->Push(closure);
    return SQ_OK;
}

char *sq_getscratchpad(HSQUIRRELVM v,SQInteger minsize)
{
    return _ss(v)->GetScratchPad(minsize);
}

SQRESULT sq_resurrectunreachable(HSQUIRRELVM v)
{
#ifndef NO_GARBAGE_COLLECTOR
    _ss(v)->ResurrectUnreachable(v);
    return SQ_OK;
#else
    return sq_throwerror(v,"sq_resurrectunreachable requires a garbage collector build");
#endif
}

SQInteger sq_collectgarbage(HSQUIRRELVM v)
{
#ifndef NO_GARBAGE_COLLECTOR
    return _ss(v)->CollectGarbage(v);
#else
    return -1;
#endif
}

SQRESULT sq_getcallee(HSQUIRRELVM v)
{
    if(v->_callsstacksize > 1)
    {
        v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
        return SQ_OK;
    }
    return sq_throwerror(v,"no closure in the calls stack");
}

const char *sq_getfreevariable(HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval)
{
    SQObjectPtr &self=stack_get(v,idx);
    const char *name = NULL;
    switch(sq_type(self))
    {
    case OT_CLOSURE:{
        SQClosure *clo = _closure(self);
        SQFunctionProto *fp = clo->_function;
        if(((SQUnsignedInteger)fp->_noutervalues) > nval) {
            v->Push(*(_outer(clo->_outervalues[nval])->_valptr));
            SQOuterVar &ov = fp->_outervalues[nval];
            name = _stringval(ov._name);
        }
                    }
        break;
    case OT_NATIVECLOSURE:{
        SQNativeClosure *clo = _nativeclosure(self);
        if(clo->_noutervalues > nval) {
            v->Push(clo->_outervalues[nval]);
            name = "@NATIVE";
        }
                          }
        break;
    default: break; //shutup compiler
    }
    return name;
}

SQRESULT sq_setfreevariable(HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval)
{
    SQObjectPtr &self=stack_get(v,idx);
    switch(sq_type(self))
    {
    case OT_CLOSURE:{
        SQFunctionProto *fp = _closure(self)->_function;
        if(((SQUnsignedInteger)fp->_noutervalues) > nval){
            *(_outer(_closure(self)->_outervalues[nval])->_valptr) = stack_get(v,-1);
        }
        else return sq_throwerror(v,"invalid free var index");
                    }
        break;
    case OT_NATIVECLOSURE:
        if(_nativeclosure(self)->_noutervalues > nval){
            _nativeclosure(self)->_outervalues[nval] = stack_get(v,-1);
        }
        else return sq_throwerror(v,"invalid free var index");
        break;
    default:
        return sq_aux_invalidtype(v, sq_type(self));
    }
    v->Pop();
    return SQ_OK;
}

SQRESULT sq_getmemberhandle(HSQUIRRELVM v,SQInteger idx,HSQMEMBERHANDLE *handle)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    SQObjectPtr &key = stack_get(v,-1);
    SQTable *m = _class(*o)->_members;
    SQObjectPtr val;
    if(m->Get(key,val)) {
        handle->_static = !(_isfield(val) || _isnativefield(val));
        handle->_isNativeField = _isnativefield(val);
        handle->_index = _member_idx(val);
        v->Pop();
        return SQ_OK;
    }
    return sq_throwerror(v,"wrong index");
}

// Handles field and method access. Native fields are handled directly
// in sq_getbyhandle/sq_setbyhandle before calling this.
SQRESULT _getmemberbyhandle(HSQUIRRELVM v,SQObjectPtr &self,const HSQMEMBERHANDLE *handle,SQObjectPtr *&val)
{
    switch(sq_type(self)) {
        case OT_INSTANCE: {
                SQInstance *i = _instance(self);
                if(handle->_static) {
                    SQClass *c = i->_class;
                    val = &c->_methods[handle->_index].val;
                }
                else {
                    val = &i->_values[handle->_index];
                }
            }
            break;
        case OT_CLASS: {
                SQClass *c = _class(self);
                if(handle->_static) {
                    val = &c->_methods[handle->_index].val;
                }
                else {
                    val = &c->_defaultvalues[handle->_index].val;
                }
            }
            break;
        default:
            return sq_throwerror(v,"wrong type(expected class or instance)");
    }
    return SQ_OK;
}

SQRESULT sq_getbyhandle(HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle)
{
    SQObjectPtr &self = stack_get(v,idx);
    if (handle->_isNativeField) {
        if (sq_type(self) != OT_INSTANCE)
            return sq_throwerror(v, "expected instance for native field");
        SQObjectPtr tmp;
        _instance(self)->GetNativeField(handle->_index, tmp);
        propagate_immutable(self, tmp);
        v->Push(tmp);
        return SQ_OK;
    }
    else {
        SQObjectPtr *val = NULL;
        if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
            return SQ_ERROR;
        }
        SQObjectPtr res(_realval(*val));
        propagate_immutable(self, res);
        v->Push(res);
        return SQ_OK;
    }
}

SQRESULT sq_setbyhandle(HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle)
{
    if (handle->_isNativeField) {
        SQObjectPtr &self = stack_get(v,idx);
        SQObjectPtr &newval = stack_get(v,-1);
        if (sq_type(self) != OT_INSTANCE)
            return sq_throwerror(v,"expected instance for native field");
        if (!_instance(self)->SetNativeField(handle->_index, newval))
            return sq_throwerror(v,"type mismatch in native field assignment");
        v->Pop();
        return SQ_OK;
    }
    SQObjectPtr &self = stack_get(v,idx);
    SQObjectPtr &newval = stack_get(v,-1);
    SQObjectPtr *val = NULL;
    if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
        return SQ_ERROR;
    }
    *val = newval;
    v->Pop();
    return SQ_OK;
}

SQRESULT sq_getbase(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    if(_class(*o)->_base)
        v->Push(SQObjectPtr(_class(*o)->_base));
    else
        v->PushNull();
    return SQ_OK;
}

SQRESULT sq_getclass(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_INSTANCE,o);
    v->Push(SQObjectPtr(_instance(*o)->_class));
    return SQ_OK;
}

SQRESULT sq_createinstance(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr *o = NULL;
    _GETSAFE_OBJ(v, idx, OT_CLASS,o);
    SQInstance *inst = _class(*o)->CreateInstance(v);
    if (!inst)
        return SQ_ERROR;
    v->Push(SQObjectPtr(inst));
    return SQ_OK;
}

void sq_weakref(HSQUIRRELVM v,SQInteger idx)
{
    const SQObjectPtr &o=stack_get(v,idx);
    if(ISREFCOUNTED(sq_type(o))) {
        v->Push(SQObjectPtr(_refcounted(o)->GetWeakRef(_ss(v)->_alloc_ctx, sq_type(o), o._flags)));
        return;
    }
    v->Push(o);
}

SQRESULT sq_getweakrefval(HSQUIRRELVM v,SQInteger idx)
{
    const SQObjectPtr &o = stack_get(v,idx);
    if(sq_type(o) != OT_WEAKREF) {
        return sq_throwerror(v,"the object must be a weakref");
    }
    v->Push(SQObjectPtr(_weakref(o)->_obj));
    return SQ_OK;
}


SQRESULT sq_next(HSQUIRRELVM v,SQInteger idx)
{
    SQObjectPtr o=stack_get(v,idx),&refpos = stack_get(v,-1),realkey,val;
    if(sq_type(o) == OT_GENERATOR) {
        return sq_throwerror(v,"cannot iterate a generator");
    }
    int faketojump;
    if(!v->FOREACH_OP(o,realkey,val,refpos,666,faketojump))
        return SQ_ERROR;
    if(faketojump != 666) {
        v->Push(realkey);
        v->Push(val);
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQCompilation::SqASTData *sq_parsetoast(HSQUIRRELVM v, const char *s, SQInteger size, const char *sourcename, SQBool preserveComments, SQBool raiseerror)
{
    return ParseToAST(v, s, size, sourcename, preserveComments, raiseerror);
}

void sq_dumpast(HSQUIRRELVM v, SQCompilation::SqASTData *astData, bool nodesLocation, OutputStream *s)
{
    IndentedTreeRenderer rv(s);
    rv.printNodesLocation = nodesLocation;
    rv.render(astData->root);
}

void sq_dumpbytecode(HSQUIRRELVM v, HSQOBJECT obj, OutputStream *s, int instruction_index)
{
    if (sq_isfunction(obj)) {
        Dump(s, _funcproto(obj), true, instruction_index);
    }
    else if (sq_isclosure(obj)) {
        SQFunctionProto *proto = _closure(obj)->_function;
        Dump(s, proto, true, instruction_index);
    }
    else {
        s->writeString("Object is not a FunctionProto");
    }
}

void sq_reset_static_memos(HSQUIRRELVM v, HSQOBJECT func)
{
    if (sq_isfunction(func)) {
        ResetStaticMemos(_funcproto(func), _ss(v));
    }
    else if (sq_isclosure(func)) {
        ResetStaticMemos(_closure(func)->_function, _ss(v));
    }
}

SQRESULT sq_translateasttobytecode(HSQUIRRELVM v, SQCompilation::SqASTData *astData, const HSQOBJECT *bindings, const char *s, SQInteger size, SQBool raiseerror)
{
    SQObjectPtr o;
    if (TranslateASTToBytecode(v, astData, bindings, s, size, o, raiseerror))
    {
        v->Push(SQObjectPtr(SQClosure::Create(_ss(v), _funcproto(o))));
        return SQ_OK;
    }
    return SQ_ERROR;
}

SQRESULT sq_getimports(HSQUIRRELVM v, SQCompilation::SqASTData *astData, SQInteger *num, SQModuleImport **imports)
{
    return AstGetImports(v, astData, num, imports) ? SQ_OK : SQ_ERROR;
}

void sq_freeimports(HSQUIRRELVM v, SQInteger num, SQModuleImport *imports)
{
    AstFreeImports(v, num, imports);
}


void sq_analyzeast(HSQUIRRELVM v, SQCompilation::SqASTData *astData, const HSQOBJECT *bindings, const char *s, SQInteger size)
{
    AnalyzeCode(v, astData, bindings, s, size);
}

void sq_checktrailingspaces(HSQUIRRELVM v, const char *sourceName, const char *s, SQInteger size)
{
    StaticAnalyzer::checkTrailingWhitespaces(v, sourceName, s, size);
}

SQCompilation::SqASTData *sq_allocateASTData(HSQUIRRELVM v)
{
    SqASTData *astData = (SqASTData *)sq_vm_malloc(_ss(v)->_alloc_ctx, sizeof(SqASTData));

    void *arenaPtr = sq_vm_malloc(_ss(v)->_alloc_ctx, sizeof(Arena));
    astData->astArena = new (arenaPtr) Arena(_ss(v)->_alloc_ctx, "AST");

    void *commentsPtr = sq_vm_malloc(_ss(v)->_alloc_ctx, sizeof(Comments));
    astData->comments = new (commentsPtr) Comments(_ss(v)->_alloc_ctx);

    astData->root = nullptr;
    astData->sourceName[0] = 0;

    return astData;
}

void sq_releaseASTData(HSQUIRRELVM v, SQCompilation::SqASTData *astData)
{
    if (!astData)
        return;

    Comments *comments = astData->comments;
    if (comments)
    {
        comments->~Comments();
        sq_vm_free(_ss(v)->_alloc_ctx, comments, sizeof(Comments));
    }

    Arena *arena = astData->astArena;
    arena->~Arena();
    sq_vm_free(_ss(v)->_alloc_ctx, arena, sizeof(Arena));

    sq_vm_free(_ss(v)->_alloc_ctx, astData, sizeof(SQCompilation::SqASTData));
}

void sq_move(HSQUIRRELVM dest,HSQUIRRELVM src,SQInteger idx)
{
    dest->Push(stack_get(src,idx));
}

static bool validate_freezable(HSQUIRRELVM v, SQObjectPtr &o)
{
    SQObjectType tp = sq_type(o);
    if (tp != OT_ARRAY && tp != OT_TABLE && tp != OT_INSTANCE && tp != OT_CLASS && tp != OT_USERDATA) {
        v->Raise_Error("Cannot freeze %s", IdType2Name(tp));
        return false;
    }
    return true;
}

SQRESULT sq_freeze(HSQUIRRELVM v, SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    if (!validate_freezable(v, o))
        return SQ_ERROR;

    SQObjectPtr dst = o;
    dst._flags |= SQOBJ_FLAG_IMMUTABLE;
    v->Push(dst);
    return SQ_OK;
}

SQUIRREL_API SQRESULT sq_freeze_inplace(HSQUIRRELVM v, SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    if (!validate_freezable(v, o))
        return SQ_ERROR;

    o._flags |= SQOBJ_FLAG_IMMUTABLE;
    return SQ_OK;
}

SQRESULT sq_mark_pure_inplace(HSQUIRRELVM v, SQInteger idx)
{
    SQObjectPtr &o = stack_get(v, idx);
    SQObjectType tp = sq_type(o);
    if (tp == OT_CLOSURE)
        _closure(o)->_function->_purefunction = true;
    else if (tp == OT_NATIVECLOSURE)
        _nativeclosure(o)->_purefunction = true;
    else {
        v->Raise_Error("Type '%s' cannot be marked as a pure function", IdType2Name(tp));
        return SQ_ERROR;
    }

    return SQ_OK;
}

bool sq_is_pure_function(HSQOBJECT *func)
{
    if (!func)
        return false;

    SQObjectType tp = sq_type(*func);

    if (tp == OT_CLOSURE)
        return _closure(*func)->_function->_purefunction;
    else if (tp == OT_NATIVECLOSURE)
        return _nativeclosure(*func)->_purefunction;
    else
        return false;
}

void sq_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc)
{
    _ss(v)->_printfunc = printfunc;
    _ss(v)->_errorfunc = errfunc;
}

SQPRINTFUNCTION sq_getprintfunc(HSQUIRRELVM v)
{
    return _ss(v)->_printfunc;
}

SQPRINTFUNCTION sq_geterrorfunc(HSQUIRRELVM v)
{
    return _ss(v)->_errorfunc;
}

SQAllocContext sq_getallocctx(HSQUIRRELVM v)
{
    return _ss(v)->_alloc_ctx;
}

void *sq_malloc(SQAllocContext ctx, SQUnsignedInteger size)
{
    return SQ_MALLOC(ctx, size);
}

void *sq_realloc(SQAllocContext ctx, void* p,SQUnsignedInteger oldsize,SQUnsignedInteger newsize)
{
    return SQ_REALLOC(ctx,p,oldsize,newsize);
}

void sq_free(SQAllocContext ctx, void *p,SQUnsignedInteger size)
{
    SQ_FREE(ctx,p,size);
}

SQRESULT sq_limitthreadaccess(HSQUIRRELVM vm, int64_t tid)
{
    vm->check_thread_access = tid;
    return SQ_OK;
}

bool sq_canaccessfromthisthread(HSQUIRRELVM vm)
{
    return vm->CanAccessFromThisThread();
}

void sq_resetanalyzerconfig() {
  SQCompilation::resetAnalyzerConfig();
}

bool sq_loadanalyzerconfig(const char *configFileName) {
  return SQCompilation::loadAnalyzerConfigFile(configFileName);
}

bool sq_loadanalyzerconfigblk(const KeyValueFile &config) {
  return SQCompilation::loadAnalyzerConfigFile(config);
}

bool sq_setdiagnosticstatebyname(const char *diagId, bool state) {
  return SQCompilationContext::enableWarning(diagId, state);
}

bool sq_setdiagnosticstatebyid(int32_t id, bool state) {
  return SQCompilationContext::enableWarning(id, state);
}

void sq_printwarningslist(FILE *ostream) {
  SQCompilationContext::printAllWarnings(ostream);
}

void sq_enablesyntaxwarnings(bool on) {
  SQCompilationContext::switchSyntaxWarningsState(on);
}

void sq_checkglobalnames(HSQUIRRELVM v) {
  StaticAnalyzer::reportGlobalNamesWarnings(v);
}

void sq_mergeglobalnames(const HSQOBJECT *bindings) {
  StaticAnalyzer::mergeKnownBindings(bindings);
}
