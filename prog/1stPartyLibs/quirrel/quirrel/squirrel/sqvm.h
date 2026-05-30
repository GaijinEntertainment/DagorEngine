/*  see copyright notice in squirrel.h */
#ifndef _SQVM_H_
#define _SQVM_H_

#include "opcodes.h"
#include "sqobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

// keep 256 stack slots reserved for locals and function arguments,
// and another 256 slots for stack operations from native code
#define STACK_GROW_THRESHOLD (256 * 2)

#define MAX_SQ_STACK_SIZE 100000

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002
#define GET_FLAG_NO_TYPE_METHODS    0x00000004
#define GET_FLAG_TYPE_METHODS_ONLY  0x00000008

//base lib
void sq_base_register(HSQUIRRELVM v);

struct SQExceptionTrap{
    SQExceptionTrap() {}
    SQExceptionTrap(SQInteger ss, SQInteger stackbase,SQInstruction *ip, SQInteger ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
    SQExceptionTrap(const SQExceptionTrap &et) { (*this) = et;  }
    SQExceptionTrap &operator=(const SQExceptionTrap &et) = default;
    SQInteger _stackbase;
    SQInteger _stacksize;
    SQInstruction *_ip;
    SQInteger _extarget;
};

typedef sqvector<SQExceptionTrap> ExceptionsTraps;

struct SQVM : public CHAINABLE_OBJ
{
    struct CallInfo{
        SQInstruction *_ip;
        SQObjectPtr *_literals;
        SQObjectPtr _closure;
        SQGenerator *_generator;
        SQInt32 _etraps;
        SQInt32 _prevstkbase;
        SQInt32 _prevtop;
        SQInt32 _target;
        SQInt32 _ncalls;
        SQBool _root;
    };

typedef sqvector<CallInfo> CallInfoVec;
public:
    void DebugHookProxy(SQInteger type, const char * sourcename, SQInteger line, const char * funcname);
    static void _DebugHookProxy(HSQUIRRELVM v, SQInteger type, const char * sourcename, SQInteger line, const char * funcname);
    // _THROW variants: caller pre-loads `v->_lasterror`; resume jumps to exception_trap.
    enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_GENERATOR_THROW, ET_RESUME_VM,ET_RESUME_THROW_VM };
    SQVM(SQSharedState *ss);
    ~SQVM();
    bool Init(SQVM *friendvm, SQInteger stacksize);

    template <bool debughookPresent>
    bool Execute(const SQObjectPtr &func, SQInteger nargs, SQInteger stackbase, SQObjectPtr &outres, SQBool invoke_err_handler, ExecutionType et = ET_CALL);

    //starts a native call return when the NATIVE closure returns
    bool CallNative(SQNativeClosure *nclosure, SQInteger nargs, SQInteger newbase, SQObjectPtr &retval, SQInt32 target, bool &suspend,bool &tailcall);
    bool TailCall(SQClosure *closure, SQInteger firstparam, SQInteger nparams);
    //starts a SQUIRREL call in the same "Execution loop"
    template <bool debughookPresent>
    bool StartCall(SQClosure *closure, SQInteger target, SQInteger nargs, SQInteger stackbase, bool tailcall);
    bool CreateClassInstance(SQClass *theclass, SQObjectPtr &inst, SQObjectPtr &constructor);
    //call a generic closure pure SQUIRREL or NATIVE
    bool Call(const SQObjectPtr &closure, SQInteger nparams, SQInteger stackbase, SQObjectPtr &outres,SQBool invoke_err_handler);
    SQRESULT Suspend();

    bool GetVarTrace(const SQObjectPtr &self, const SQObjectPtr &key, char * buf, int buf_size);

    void CallDebugHook(SQInteger type,SQInteger forcedline=0);
    void CallErrorHandler(const SQObjectPtr &e);
    SQInteger GetImpl(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, SQUnsignedInteger getflags);
    bool Get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, SQUnsignedInteger getflags);
    SQInteger FallBackGet(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
    bool InvokeTypeMethod(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
    SQClass* GetBuiltInClassForType(SQObjectType type);
    bool Set(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val);
    SQInteger FallBackSet(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
    bool NewSlot(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val,bool bstatic);
    bool DeleteSlot(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &res);
    bool Clone(const SQObjectPtr &self, SQObjectPtr &target);
    bool ObjCmp(const SQObjectPtr &o1, const SQObjectPtr &o2,SQInteger &res);
    bool StringCat(const SQObjectPtr &str, const SQObjectPtr &obj, SQObjectPtr &dest);
    static bool IsEqual(const SQObject &o1,const SQObject &o2);
    static bool IsInstanceOf(const SQObject &obj, const SQClass *cls);
    bool ToString(const SQObjectPtr &o,SQObjectPtr &res);
    SQString *PrintObjVal(const SQObject &o);


    void Raise_Error(const char *s, ...);
    void Raise_Error(const SQObjectPtr &desc);
    void Raise_IdxError(const SQObjectPtr &o);
    void Raise_MetamethodError(const char *mmname);
    void Raise_CompareError(const SQObject &o1, const SQObject &o2);
    void Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type,const char *funcname = nullptr);

    void FindOuter(SQObjectPtr &target, SQObjectPtr *stackindex);
    void RelocateOuters();
    void CloseOuters(SQObjectPtr *stackindex);

    bool TypeOf(const SQObjectPtr &obj1, SQObjectPtr &dest);
    bool CallMetaMethod(SQObjectPtr &closure, SQMetaMethod mm, SQInteger nparams, SQObjectPtr &outres);
    bool ArithMetaMethod(SQInteger op, const SQObjectPtr &o1, const SQObjectPtr &o2, SQObjectPtr &dest);
    template <bool debughookPresent>
    bool Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr &retval);

    bool ARITH_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
    bool BW_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
    bool NEG_OP(SQObjectPtr &trg,const SQObjectPtr &o1);
    bool CMP_OP(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &res);
    bool CMP_OP_RES(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,int &res);
    bool CMP_OP_RESI(CmpOP op, const SQObjectPtr &o1,const SQInteger o2,int &res);
    bool CMP_OP_RESF(CmpOP op, const SQObjectPtr &o1,const SQFloat o2,int &res);
    bool ObjCmpI(const SQObjectPtr &o1, const SQInteger o2,SQInteger &res);
    bool ObjCmpF(const SQObjectPtr &o1, const SQFloat o2,SQInteger &res);
    bool CLOSURE_OP(SQObjectPtr &target, SQFunctionProto *func);
    bool CLASS_OP(SQObjectPtr &target,SQInteger base);
    //return true if the loop is finished
    bool FOREACH_OP(SQObjectPtr &o1,SQObjectPtr &o2,SQObjectPtr &o3,SQObjectPtr &o4,int exitpos,int &jump);
    bool PLOCAL_INC(SQInteger op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
    bool DerefInc(SQInteger op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix);
#ifdef _DEBUG_DUMP
    void dumpstack(SQInteger stackbase=-1, bool dumpall = false);
#endif

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    SQObjectType GetType() {return OT_THREAD;}
#endif
    void Finalize();
    void GrowCallStack() {
        SQInteger newsize = _alloccallsstacksize*2;
        _callstackdata.resize(newsize);
        _callsstack = &_callstackdata[0];
        _alloccallsstacksize = newsize;
    }
    bool EnterFrame(SQInteger newbase, SQInteger newtop, bool tailcall);
    void LeaveFrame();
    void Release();

    //stack functions for the api
    void Remove(SQInteger n);

    static bool IsFalse(const SQObject &o);

    void Pop();
    void Pop(SQInteger n);
    void Push(const SQObjectPtr &o);
    void Push(SQObjectPtr &&o);
    void PushNull();
    SQObjectPtr &Top();
    SQObjectPtr &PopGet();
    SQObjectPtr &GetUp(SQInteger n);
    SQObjectPtr &GetAt(SQInteger n);

    #if SQ_CHECK_THREAD >= SQ_CHECK_THREAD_LEVEL_DEEP
    inline void ValidateThreadAccess() {
        assert(!_get_current_thread_id_func || check_thread_access==0 || check_thread_access==_get_current_thread_id_func());
    }
    #else
    inline void ValidateThreadAccess() {}
    #endif

    inline bool CanAccessFromThisThread() {
        #if SQ_CHECK_THREAD >= SQ_CHECK_THREAD_LEVEL_FAST
        if (_nnativecalls && _get_current_thread_id_func)
            return _get_current_thread_id_func() == _current_thread;
        #endif
        return true;
    }

    SQObjectPtrVec _stack;

    SQInteger _top;
    SQInteger _stackbase;
    SQOuter *_openouters;
    SQObjectPtr _roottable;
    SQObjectPtr _lasterror;
    SQObjectPtr _errorhandler;

    bool _debughook;
    SQDEBUGHOOK _debughook_native;
    SQObjectPtr _debughook_closure;
    SQInteger _current_thread;
    SQGETTHREAD _get_current_thread_id_func;
    SQCOMPILELINEHOOK _compile_line_hook;
    SQWATCHDOGHOOK _watchdog_hook;

    SQObjectPtr temp_reg;


    CallInfo* _callsstack;
    SQInteger _callsstacksize;
    SQInteger _alloccallsstacksize;
    sqvector<CallInfo>  _callstackdata;

    ExceptionsTraps _etraps;
    CallInfo *ci;
    SQUserPointer _foreignptr;
    //VMs sharing the same state
    SQSharedState *_sharedstate;
    SQInteger _nnativecalls;
    SQInteger _nmetamethodscall;
    SQRELEASEHOOK _releasehook;
    //suspend infos
    SQBool _suspended;
    SQBool _suspended_root;
    SQInteger _suspended_target;
    SQInteger _suspended_traps;

    int64_t check_thread_access = 0;
};

struct AutoDec{
    AutoDec(SQInteger *n) { _n = n; }
    ~AutoDec() { (*_n)--; }
    SQInteger *_n;
};

// `ip` picks which instruction's line to report; the fault path
// passes ci._ip - 1 to land on the faulting (or call-site) instruction.
// funcnameObj/sourceObj alias the live SQString when one exists, so a trace
// snapshot can reuse it instead of re-interning; OT_NULL when the name is a
// literal fallback. funcname/source stay a valid C-string view either way.
struct SQFrameInfo {
    const char *funcname; const char *source; SQInteger line;
    SQObject funcnameObj; SQObject sourceObj;
};
void sq_get_frame_info(const SQVM::CallInfo &ci, const SQInstruction *ip, SQFrameInfo &out);

// Snapshot the live call stack from v->ci down to its root frame as an array of
// { func, source, line } tables, innermost first.
SQObjectPtr sq_capture_error_trace(SQVM *v);

// Append errVal's captured trace to buf, one "at <func> (<source>:<line>)" line
// per frame (`awaited at` for await-hops). Returns true if anything was
// appended; false (buf untouched) for a non-Error or missing trace.
bool sq_append_error_trace(SQVM *v, const SQObjectPtr &errVal, sqvector<char> &buf);

// Render an Error instance's captured trace (one "at <func> (<source>:<line>)"
// line per frame) through errfn. No-op when errVal is not an Error carrying an
// array trace.
void sq_print_error_trace(SQVM *v, const SQObjectPtr &errVal, SQPRINTFUNCTION errfn);

// Append an `awaited at` frame to errVal's trace, read from a suspended
// generator's saved _ci. The settle-time ancestry walk calls this per parked
// async ancestor. No-op when errVal cannot carry a trace (value-types).
void sq_append_awaited_frame(SQVM *v, const SQObjectPtr &errVal, const SQVM::CallInfo &ci);

// Per-consumer carrier for a fork: shallow-clone an Error and give the clone its
// own copy of the trace array, so each fork branch stitches its own `awaited at`
// chain without polluting siblings. Returns errVal unchanged for non-Error
// values (they have no carrier; the fork shares them as before).
SQObjectPtr sq_clone_error_for_branch(SQVM *v, const SQObjectPtr &errVal);

inline SQObjectPtr &stack_get(HSQUIRRELVM v,SQInteger idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

#define _ss(_vm_) (_vm_)->_sharedstate

#ifndef NO_GARBAGE_COLLECTOR
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
// This change is needed to make use of alloc_ctx
// Without alloc_ctx NULL can be returned
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#endif

#endif //_SQVM_H_
