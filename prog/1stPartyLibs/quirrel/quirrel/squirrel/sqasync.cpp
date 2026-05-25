#include "sqpcheader.h"
#include <sqasync.h>
#include <string.h>
#include <mutex>
#include <new>

#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "squserdata.h"
#include "sqclass.h"
#include "sqclosure.h"
#include "squtils.h"


namespace sqasync
{

static int promise_type_tag_anchor = 0;

static inline SQUserPointer promiseTypeTag() { return (SQUserPointer)&promise_type_tag_anchor; }


enum Lifecycle : SQUnsignedInteger32
{
    L_Open = 0,        // pending; settle-able
    L_Adopted = 1,     // pending; locked on another Promise (`value` is that instance)
    L_Fulfilled = 2,
    L_Rejected = 3,
};


enum ResumeKind
{
    Resume_Initial = 0,   // task-promise has never run; first step
    Resume_Send = 1,      // await resolved; inject value at the parked yield slot
    Resume_Throw = 2,     // await rejected; raise inside the resumed frame
};


struct PromiseImpl
{
    Lifecycle lifecycle;
    bool isTask;                // latched true in buildTaskPromise
    bool awaited;               // someone has been (or is being) parked on this

    HSQOBJECT value;            // addref'd when non-null; adoption-target instance when L_Adopted
    HSQOBJECT generator;        // addref'd when non-null; valid only while isTask && L_Open

    sqvector<HSQOBJECT> waiters; // each addref'd; mix of awaiting tasks and chain-locked adopters

    SQAllocContext alloc_ctx;   // free path in promise_releasehook when vm is null

    // Unhandled-rejection diagnostic context. Leaf SQStrings only: an
    // SQString has no outgoing refs, so addref'ing one cannot form a GC
    // cycle (same safety class as `value`). Captured at creation, not
    // reject - the async closure is Kill()'d during the throwing Execute.
    HSQOBJECT asyncSourceName;      // null for a bare Promise (no generator)
    HSQOBJECT asyncFuncName;
    HSQOBJECT launchSourceName;     // null for a native/root immediate caller
    HSQOBJECT launchFuncName;
    SQInt32 asyncDefLine;
    SQInt32 launchLine;

    PromiseImpl(SQAllocContext ctx)
        : lifecycle(L_Open), isTask(false), awaited(false),
          waiters(ctx), alloc_ctx(ctx), asyncDefLine(0), launchLine(0)
    {
        sq_resetobject(&value);
        sq_resetobject(&generator);
        sq_resetobject(&asyncSourceName);
        sq_resetobject(&asyncFuncName);
        sq_resetobject(&launchSourceName);
        sq_resetobject(&launchFuncName);
    }
};


// Tagged union for the runtime queue. K_Callback comes from post_on_vm_thread
// or post_from_any_thread; K_Resume from scheduleStep. Fields are POD so
// sqvector's memmove growth is safe. K_Resume is VM-thread-only - the
// HSQOBJECT threading rules forbid addref off the owning thread - so only
// K_Callback can land in inbox.
struct QueueEntry
{
    enum Kind { K_Callback, K_Resume };
    Kind kind;
    ResumeKind resumeKind;   // K_Resume only
    VmCallback fn;           // K_Callback only
    void *user;              // K_Callback only
    HSQOBJECT taskInstance;  // K_Resume only; addref'd
    HSQOBJECT resumeValue;   // K_Resume only; addref'd
};


// Concrete async-runtime state. SQSharedState owns one of these (typed
// pointer, allocated by sqasync::bind, freed by sqasync::shutdown). Forward
// declared in squirrel/sqstate.h.
struct AsyncState
{
    HSQUIRRELVM rootVm;

    // bound class object; addref'd
    HSQOBJECT promiseClass;

    // VM-thread-only step queue (no lock)
    sqvector<QueueEntry> queue;

    // protects `inbox` and the `shuttingDown` write
    std::mutex inboxLock;

    // Cross-thread landing pad for K_Callback posts; pump() drains it into queue.
    // (K_Resume entries can't appear here - see QueueEntry above.)
    sqvector<QueueEntry> inbox;

    // written under inboxLock so producers see it
    bool shuttingDown;

    AsyncState(HSQUIRRELVM v, SQAllocContext ctx)
        : rootVm(v), queue(ctx), inbox(ctx), shuttingDown(false)
    {
        sq_resetobject(&promiseClass);
    }
};


namespace
{

inline AsyncState *getAsyncState(HSQUIRRELVM v)
{
    return _ss(v)->_asyncState;
}


inline PromiseImpl *getPromiseFromInstance(HSQUIRRELVM v, const HSQOBJECT &inst)
{
    if (sq_type(inst) != OT_INSTANCE)
        return nullptr;
    SQClass *cls = _instance(inst)->_class;
    if (cls->_typetag != promiseTypeTag())
        return nullptr;
    // _userpointer is nullptr until promise_constructor (or buildTaskPromise/the
    // await fast path) installs the PromiseImpl. A subclass that skips
    // super.constructor() lands here with a matching typetag but no impl.
    return (PromiseImpl *)_instance(inst)->_userpointer;
}


// Resolve `this` (stack slot 1) into a PromiseImpl for the script-callable
// methods. Throws and returns nullptr if the receiver isn't a Promise instance,
// or is a subclass instance whose super.constructor() was never invoked.
PromiseImpl *requirePromiseSelf(HSQUIRRELVM v, const char *errMsg)
{
    SQUserPointer up = nullptr;
    if (SQ_FAILED(sq_getinstanceup(v, 1, &up, promiseTypeTag())) || up == nullptr) {
        sq_throwerror(v, errMsg);
        return nullptr;
    }
    return (PromiseImpl *)up;
}


void runStep(HSQUIRRELVM v, HSQOBJECT taskInstance, ResumeKind kind, HSQOBJECT resumeValue);
void scheduleStep(HSQUIRRELVM v, const HSQOBJECT &taskInstance, ResumeKind kind, const HSQOBJECT &resumeValue);
SQRESULT settleTerminal(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT &selfInstance, const HSQOBJECT &value, bool reject);
SQRESULT resolveAndAdopt(HSQUIRRELVM v, PromiseImpl *target, HSQOBJECT targetInstance, HSQOBJECT valueIn, bool throwOnCycle);
SQRESULT externalSettle(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT &p_inst, const HSQOBJECT &value, bool reject);


//-----------------------------------------------------------------------------
// Promise script-class methods.

// Render a rejection reason without invoking script metamethods and
// without recursion (the hook runs in a decref/GC context), and with no
// pointers/sizes so the test goldens stay stable. Cf. SQVM::PrintObjVal.
static void format_reason(const HSQOBJECT &o, char *buf, size_t n)
{
    switch (sq_type(o)) {
        case OT_STRING: {
            const SQInteger kMax = 96;
            SQInteger len = _string(o)->_len;
            scsprintf(buf, n, "string \"%.*s%s\"",
                (int)(len > kMax ? kMax : len), _stringval(o),
                len > kMax ? "..." : "");
            break;
        }
        case OT_INTEGER:
            scsprintf(buf, n, "integer " _PRINT_INT_FMT, _integer(o));
            break;
        case OT_FLOAT:
            scsprintf(buf, n, "float %.14g", _float(o));
            break;
        case OT_BOOL:
            scsprintf(buf, n, "bool %s", _integer(o) ? "true" : "false");
            break;
        case OT_NULL:
            scsprintf(buf, n, "null");
            break;
        default:
            scsprintf(buf, n, "%s", GetTypeName(o));
            break;
    }
}


SQInteger promise_releasehook(HSQUIRRELVM vm, SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    PromiseImpl *self = (PromiseImpl *)p;

    // vm is null when we're called from _refs_table.Finalize() during shutdown:
    // ~SQSharedState already Null'd _root_vm. The ref table is nulling every
    // slot we'd sq_release from anyway, so the per-object refcount still balances.
    if (vm) {
        // Unhandled-rejection diagnostic. v1 default: a one-line warning to
        // the VM's error stream when a Rejected promise dies without ever
        // being awaited. No hook in v1 (see design decision 15).
        if (self->lifecycle == L_Rejected && !self->awaited) {
            SQPRINTFUNCTION errfn = _ss(vm)->_errorfunc;
            if (errfn) {
                char reason[160];
                format_reason(self->value, reason, sizeof(reason));

                char line[768];
                if (sq_type(self->asyncSourceName) == OT_STRING) {
                    // Task-promise: an async function rejected.
                    const char *an = sq_type(self->asyncFuncName) == OT_STRING ? _stringval(self->asyncFuncName) : "";
                    if (sq_type(self->launchSourceName) == OT_STRING) {
                        const char *ln = sq_type(self->launchFuncName) == OT_STRING ? _stringval(self->launchFuncName) : "";
                        scsprintf(line, sizeof(line),
                            "[sqasync] unhandled promise rejection in async %.80s (%.160s:%d); reason: %s; launched at %.160s:%d %.80s\n",
                            an, _stringval(self->asyncSourceName), (int)self->asyncDefLine, reason,
                            _stringval(self->launchSourceName), (int)self->launchLine, ln);
                    } else {
                        // Native / root immediate caller: no launch clause.
                        scsprintf(line, sizeof(line),
                            "[sqasync] unhandled promise rejection in async %.80s (%.160s:%d); reason: %s\n",
                            an, _stringval(self->asyncSourceName), (int)self->asyncDefLine, reason);
                    }
                } else {
                    // Bare Promise().reject(...) with no generator.
                    scsprintf(line, sizeof(line),
                        "[sqasync] unhandled promise rejection on Promise.reject; reason: %s\n", reason);
                }
                errfn(vm, line);
            }
        }

        sq_release(vm, &self->value);
        sq_release(vm, &self->generator);
        for (SQUnsignedInteger32 i = 0; i < self->waiters.size(); ++i)
            sq_release(vm, &self->waiters._vals[i]);
        sq_release(vm, &self->asyncSourceName);
        sq_release(vm, &self->asyncFuncName);
        sq_release(vm, &self->launchSourceName);
        sq_release(vm, &self->launchFuncName);
    }

    SQAllocContext ctx = self->alloc_ctx;
    self->~PromiseImpl();
    sq_free(ctx, self, sizeof(PromiseImpl));
    return 1;
}


SQInteger promise_constructor(HSQUIRRELVM v)
{
    SQUserPointer existing = nullptr;
    if (SQ_SUCCEEDED(sq_getinstanceup(v, 1, &existing, promiseTypeTag())) && existing != nullptr)
        return sq_throwerror(v, "Promise: constructor called more than once");

    SQAllocContext ctx = sq_getallocctx(v);
    void *mem = sq_malloc(ctx, sizeof(PromiseImpl));
    PromiseImpl *p = new (mem) PromiseImpl(ctx);

    if (SQ_FAILED(sq_setinstanceup(v, 1, p))) {
        p->~PromiseImpl();
        sq_free(ctx, p, sizeof(PromiseImpl));
        return sq_throwerror(v, "cannot create Promise");
    }
    sq_setreleasehook(v, 1, promise_releasehook);
    return 0;
}


SQInteger promise_getState(HSQUIRRELVM v)
{
    PromiseImpl *p = requirePromiseSelf(v, "Promise.getState: invalid 'this'");
    if (!p) return SQ_ERROR;
    const char *s = "pending";
    if (p->lifecycle == L_Fulfilled) s = "fulfilled";
    else if (p->lifecycle == L_Rejected) s = "rejected";
    // L_Adopted reports as "pending" - it is, from the outside.
    sq_pushstring(v, s, -1);
    return 1;
}


// Shared script entry for Promise._resolve and Promise._reject.
SQInteger promise_settle(HSQUIRRELVM v, bool reject)
{
    PromiseImpl *p = requirePromiseSelf(v, "Promise: invalid 'this'");
    if (!p) return SQ_ERROR;

    HSQOBJECT v_obj;
    sq_resetobject(&v_obj);
    if (sq_gettop(v) >= 2)
        sq_getstackobj(v, 2, &v_obj);

    HSQOBJECT self_obj;
    sq_resetobject(&self_obj);
    sq_getstackobj(v, 1, &self_obj);

    return SQ_FAILED(externalSettle(v, p, self_obj, v_obj, reject)) ? SQ_ERROR : 0;
}


SQInteger promise_resolve_method(HSQUIRRELVM v) { return promise_settle(v, false); }
SQInteger promise_reject_method(HSQUIRRELVM v)  { return promise_settle(v, true); }


//-----------------------------------------------------------------------------
// Settlement + waiter dispatch.

struct CascadeJob
{
    PromiseImpl *p;
    HSQOBJECT instance;  // owns one ref; pins `p` against sq_release teardown
    HSQOBJECT pinned;    // owns one ref; transfers into p->value
    bool reject;
};

// selfInstance feeds the per-iteration cycle check below; required.
SQRESULT settleTerminal(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT &selfInstance,
                        const HSQOBJECT &value, bool reject)
{
    if (p->lifecycle == L_Fulfilled || p->lifecycle == L_Rejected)
        return SQ_OK;

    // Pin `value` BEFORE releasing the old p->value: in the L_Adopted ->
    // terminal cascade, releasing the old adoption target can drop the
    // last ref on whoever owns the incoming `value`.
    HSQOBJECT curPinned = value;
    sq_addref(v, &curPinned);

    sqvector<CascadeJob> worklist(p->alloc_ctx);
    SQUnsignedInteger32 head = 0;

    PromiseImpl *cur = p;
    bool curReject = reject;

    HSQOBJECT curInstance = selfInstance;
    sq_addref(v, &curInstance);

    bool more = true;
    while (more) {
        // cur->value = cur would form an uncollectable refcount cycle.
        // Substitute and force reject. Matches JS PRP on a chaining cycle.
        if (sq_type(curPinned) == OT_INSTANCE && sq_type(curInstance) == OT_INSTANCE
            && _instance(curPinned) == _instance(curInstance)) {
            sq_pushstring(v, "Promise: chaining cycle detected", -1);
            HSQOBJECT subst;
            sq_resetobject(&subst);
            sq_getstackobj(v, -1, &subst);
            sq_addref(v, &subst);
            sq_pop(v, 1);
            sq_release(v, &curPinned);
            curPinned = subst;
            curReject = true;
        }

        if (cur->lifecycle == L_Adopted) {
            sq_release(v, &cur->value);
            sq_resetobject(&cur->value);
        }

        cur->lifecycle = curReject ? L_Rejected : L_Fulfilled;
        cur->value = curPinned;  // ref transfers from curPinned into cur->value

        sq_release(v, &cur->generator);
        sq_resetobject(&cur->generator);

        // Snapshot up front so any reentrant settle sees an empty list.
        sqvector<HSQOBJECT> drained = cur->waiters;
        cur->waiters.resize(0);

        if (drained.size() > 0) {
            cur->awaited = true;
            ResumeKind kind = curReject ? Resume_Throw : Resume_Send;
            for (SQUnsignedInteger32 i = 0; i < drained.size(); ++i) {
                PromiseImpl *w = getPromiseFromInstance(v, drained._vals[i]);
                if (w && w->lifecycle == L_Adopted) {
                    CascadeJob job;
                    job.p = w;
                    job.instance = drained._vals[i];  // waiters-list ref transfers in
                    job.pinned = cur->value;
                    sq_addref(v, &job.pinned);
                    job.reject = curReject;
                    worklist.push_back(job);
                }
                else {
                    scheduleStep(v, drained._vals[i], kind, cur->value);
                    sq_release(v, &drained._vals[i]);
                }
            }
        }

        // Release after the cur reads/writes above: dropping curInstance may
        // free cur synchronously via its releasehook.
        sq_release(v, &curInstance);
        sq_resetobject(&curInstance);

        more = false;
        while (head < worklist.size()) {
            CascadeJob job = worklist._vals[head++];
            // Defensive: each adopter sits in exactly one waiters list, so
            // a popped target shouldn't already be terminal. Balance both
            // refs if invariants ever drift.
            if (job.p->lifecycle == L_Fulfilled || job.p->lifecycle == L_Rejected) {
                sq_release(v, &job.pinned);
                sq_release(v, &job.instance);
                continue;
            }
            cur = job.p;
            curInstance = job.instance;
            curPinned = job.pinned;
            curReject = job.reject;
            more = true;
            break;
        }
    }
    return SQ_OK;
}


// Caller MUST hold one strong ref on `targetInstance`; this function
// always consumes it (transferred into q->waiters on adoption, released
// otherwise). Indirect cycles are caught by the L_Adopted forward-follow.
SQRESULT resolveAndAdopt(HSQUIRRELVM v, PromiseImpl *target,
                         HSQOBJECT targetInstance, HSQOBJECT valueIn,
                         bool throwOnCycle)
{
    if (target->lifecycle != L_Open) {
        sq_release(v, &targetInstance);
        return SQ_OK;
    }

    // No addref: each visited q is held alive transitively via the chain
    // from valueIn through L_Adopted->value links for the walk duration.
    sqvector<PromiseImpl *> visited(target->alloc_ctx);

    HSQOBJECT current = valueIn;
    while (PromiseImpl *q = getPromiseFromInstance(v, current)) {
        if (q == target) {
            if (throwOnCycle) {
                sq_release(v, &targetInstance);
                return sq_throwerror(v, "Promise.resolve: cannot resolve with self");
            }
            // Async-return path: no script frame to throw into.
            sq_pushstring(v, "Promise chain-unwrap: self-cycle", -1);
            HSQOBJECT errObj;
            sq_resetobject(&errObj);
            sq_getstackobj(v, -1, &errObj);
            settleTerminal(v, target, targetInstance, errObj, /*reject*/true);
            sq_pop(v, 1);
            sq_release(v, &targetInstance);
            return SQ_OK;
        }
        visited.push_back(q);
        switch (q->lifecycle) {
            case L_Open:
                for (SQUnsignedInteger32 i = 0; i < visited.size(); ++i)
                    visited._vals[i]->awaited = true;
                sq_release(v, &target->generator);
                sq_resetobject(&target->generator);
                target->lifecycle = L_Adopted;
                target->value = current;
                sq_addref(v, &target->value);
                q->waiters.push_back(targetInstance);  // transfers caller's ref
                return SQ_OK;
            case L_Adopted:
            case L_Fulfilled:
                current = q->value;
                continue;
            case L_Rejected:
                for (SQUnsignedInteger32 i = 0; i < visited.size(); ++i)
                    visited._vals[i]->awaited = true;
                settleTerminal(v, target, targetInstance, q->value, /*reject*/true);
                sq_release(v, &targetInstance);
                return SQ_OK;
        }
    }
    for (SQUnsignedInteger32 i = 0; i < visited.size(); ++i)
        visited._vals[i]->awaited = true;
    settleTerminal(v, target, targetInstance, current, /*reject*/false);
    sq_release(v, &targetInstance);
    return SQ_OK;
}


SQRESULT externalSettle(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT &p_inst,
                        const HSQOBJECT &value, bool reject)
{
    // Task-promise terminal state is owned by runStep's exit paths.
    if (p->isTask)
        return sq_throwerror(v, reject
            ? "Promise.reject: cannot reject the promise returned by an async function"
            : "Promise.resolve: cannot resolve the promise returned by an async function");

    if (p->lifecycle != L_Open)
        return SQ_OK;

    if (reject) {
        // Direct self-reject is a script error (uncollectable refcount cycle).
        if (sq_type(value) == OT_INSTANCE && sq_type(p_inst) == OT_INSTANCE
            && _instance(value) == _instance(p_inst))
            return sq_throwerror(v, "Promise.reject: cannot reject with self");
        return settleTerminal(v, p, p_inst, value, /*reject*/true);
    }

    HSQOBJECT inst_owned = p_inst;
    sq_addref(v, &inst_owned);  // consumed by resolveAndAdopt
    return resolveAndAdopt(v, p, inst_owned, value, /*throwOnCycle*/true);
}


//-----------------------------------------------------------------------------
// Runner: drives a parked task-promise one step at a time.


void scheduleStep(HSQUIRRELVM v, const HSQOBJECT &taskInstance, ResumeKind kind, const HSQOBJECT &resumeValue)
{
    AsyncState *st = getAsyncState(v);
    if (!st || st->shuttingDown) return;

    QueueEntry e;
    e.kind = QueueEntry::K_Resume;
    e.resumeKind = kind;
    e.fn = nullptr;
    e.user = nullptr;
    e.taskInstance = taskInstance;
    sq_addref(v, &e.taskInstance);
    e.resumeValue = resumeValue;
    sq_addref(v, &e.resumeValue);
    st->queue.push_back(e);
}


// Resumes a parked async task one step. The caller has addref'd taskInstance
// and resumeValueIn for us once each; we sq_release both on every return,
// except when we park on a pending await - there taskInstance's ref transfers
// into that promise's waiters list.
void runStep(HSQUIRRELVM v, HSQOBJECT taskInstance, ResumeKind kind, HSQOBJECT resumeValueIn)
{
    // Pin resumeValueIn into a local SQObjectPtr so it survives Execute and
    // releases automatically on scope exit. The release + assignment below is
    // one ownership transfer.
    SQObjectPtr resumeValue;
    resumeValue = resumeValueIn;
    sq_release(v, &resumeValueIn);

    AsyncState *st = getAsyncState(v);
    PromiseImpl *p = getPromiseFromInstance(v, taskInstance);
    if (!st || st->shuttingDown || !p || p->lifecycle != L_Open) {
        sq_release(v, &taskInstance);
        return;
    }

    // Resume_Send: write the value into the generator's parked yield slot; when
    //   Execute resumes the generator, it picks the value up as the await result.
    // Resume_Throw: preload _lasterror and switch ExecutionType so Execute jumps
    //   straight into the exception trap inside the resumed frame.
    // Resume_Initial: nothing to deliver - this is the generator's first run.
    SQGenerator *gen = _generator(p->generator);
    SQVM::ExecutionType et = SQVM::ET_RESUME_GENERATOR;
    if (kind == Resume_Send) {
        if (gen->_yield_arg1 != MAX_FUNC_STACKSIZE)
            gen->_stack[gen->_yield_arg1] = resumeValue;
    }
    else if (kind == Resume_Throw) {
        v->_lasterror = resumeValue;
        et = SQVM::ET_RESUME_GENERATOR_THROW;
    }

    // Mirror sq_resume's calling convention exactly.
    sq_pushobject(v, p->generator);
    sq_pushnull(v);
    bool execOk = v->_debughook
        ? v->Execute<true>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), SQFalse, et)
        : v->Execute<false>(v->GetUp(-2), 0, v->_top, v->GetUp(-1), SQFalse, et);
    SQObjectPtr outres;
    if (execOk)
        outres = v->GetUp(-1);
    sq_pop(v, 2);  // pop generator + placeholder

    if (!execOk) {
        // Generator threw an unhandled error. Take a strong ref before clearing
        // _lasterror so we don't lose the value if it was the last holder.
        SQObjectPtr err = v->_lasterror;
        v->_lasterror.Null();  // don't leak across steps
        settleTerminal(v, p, taskInstance, err, true);
        sq_release(v, &taskInstance);
        return;
    }
    if (gen->_state == SQGenerator::eDead) {
        // resolveAndAdopt consumes taskInstance.
        resolveAndAdopt(v, p, taskInstance, outres, /*throwOnCycle*/false);
        return;
    }

    // Yielded an awaitable: either park on a pending Promise, or scheduleStep
    // the next step right away. Chains of already-resolved awaits still unwind
    // in a single tick - pump drains entries pushed during dispatch up to maxSteps.
    PromiseImpl *q = getPromiseFromInstance(v, outres);
    if (!q) {
        scheduleStep(v, taskInstance, Resume_Send, outres);
        sq_release(v, &taskInstance);
        return;
    }
    q->awaited = true;
    if (q->lifecycle != L_Fulfilled && q->lifecycle != L_Rejected) {
        q->waiters.push_back(taskInstance);  // ref transfers; do NOT release
        return;
    }
    ResumeKind nextKind = (q->lifecycle == L_Rejected) ? Resume_Throw : Resume_Send;
    scheduleStep(v, taskInstance, nextKind, q->value);
    sq_release(v, &taskInstance);
}


//-----------------------------------------------------------------------------
// Push a fresh Promise instance with PromiseImpl + releasehook installed.
// Returns the impl pointer (also gettable via _userpointer); on failure,
// returns nullptr with the stack restored to the entry top.
PromiseImpl *createPromiseInstanceOnStack(HSQUIRRELVM v, AsyncState *st)
{
    sq_pushobject(v, st->promiseClass);
    if (SQ_FAILED(sq_createinstance(v, -1))) {
        sq_pop(v, 1);
        return nullptr;
    }

    SQAllocContext ctx = sq_getallocctx(v);
    void *mem = sq_malloc(ctx, sizeof(PromiseImpl));
    PromiseImpl *p = new (mem) PromiseImpl(ctx);

    if (SQ_FAILED(sq_setinstanceup(v, -1, p))) {
        p->~PromiseImpl();
        sq_free(ctx, p, sizeof(PromiseImpl));
        sq_pop(v, 2);  // drop instance + class
        return nullptr;
    }
    sq_setreleasehook(v, -1, promise_releasehook);
    sq_remove(v, -2);  // drop class; instance on top
    return p;
}


// No-op unless `src` is a string, so a handle left null reliably means
// "structurally unavailable" rather than an addref that silently failed.
static inline void captureProtoString(HSQUIRRELVM v, const SQObjectPtr &src, HSQOBJECT &dst)
{
    if (sq_type(src) == OT_STRING) {
        dst = src;
        sq_addref(v, &dst);
    }
}


// Capture the diagnostic context. Must run at creation, not reject: the
// async closure is Kill()'d (its _closure Null'd) while the throwing
// Execute unwinds, and the caller frame is gone by reject time.
static void captureDiag(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT &gen)
{
    if (sq_type(gen) == OT_GENERATOR) {
        const SQObjectPtr &cl = _generator(gen)->_closure;
        if (sq_type(cl) == OT_CLOSURE) {
            SQFunctionProto *fp = _closure(cl)->_function;
            captureProtoString(v, fp->_sourcename, p->asyncSourceName);
            captureProtoString(v, fp->_name, p->asyncFuncName);
            p->asyncDefLine = (SQInt32)(fp->_nlineinfos > 0 ? fp->_lineinfos->_first_line : 0);
        }
    }

    // v->ci is the caller here: StartCall already Return<>()'d out of the
    // async fn's own frame before calling wrap_generator.
    SQVM::CallInfo *cci = v->ci;
    if (cci && sq_type(cci->_closure) == OT_CLOSURE) {
        SQFunctionProto *cfp = _closure(cci->_closure)->_function;
        captureProtoString(v, cfp->_sourcename, p->launchSourceName);
        captureProtoString(v, cfp->_name, p->launchFuncName);
        p->launchLine = (SQInt32)cfp->GetLine(cci->_ip);
    }
}


// Build a fresh task-promise wrapping `gen`, schedule its initial step, and
// write the promise instance handle through `outInstance`. Called from the
// engine's StartCall via sqasync::wrap_generator.
bool buildTaskPromise(HSQUIRRELVM v, const HSQOBJECT &gen, HSQOBJECT &outInstance)
{
    AsyncState *st = getAsyncState(v);
    if (!st || st->shuttingDown) return false;

    PromiseImpl *p = createPromiseInstanceOnStack(v, st);
    if (!p) {
        sq_resetobject(&outInstance);
        return false;
    }

    HSQOBJECT taskInstance;
    sq_resetobject(&taskInstance);
    sq_getstackobj(v, -1, &taskInstance);

    p->generator = gen;
    sq_addref(v, &p->generator);
    p->isTask = true;

    captureDiag(v, p, gen);

    HSQOBJECT null;
    sq_resetobject(&null);
    scheduleStep(v, taskInstance, Resume_Initial, null);

    outInstance = taskInstance;
    sq_addref(v, &outInstance);
    sq_pop(v, 1);  // pop instance; outInstance keeps its own ref
    return true;
}


//-----------------------------------------------------------------------------
// bind helpers.

void buildAndCachePromiseClass(HSQUIRRELVM v, AsyncState *st)
{
    SQInteger top = sq_gettop(v);

    sq_newclass(v, SQFalse);
    sq_settypetag(v, -1, promiseTypeTag());

    static const struct { const char *name; SQFUNCTION fn; } kPromiseMethods[] = {
        {"constructor", promise_constructor},
        {"getState",    promise_getState},
        {"resolve",     promise_resolve_method},
        {"reject",      promise_reject_method},
    };
    for (const auto &m : kPromiseMethods) {
        sq_pushstring(v, m.name, -1);
        sq_newclosure(v, m.fn, 0);
        sq_newslot(v, -3, SQFalse);
    }

    sq_resetobject(&st->promiseClass);
    sq_getstackobj(v, -1, &st->promiseClass);
    sq_addref(v, &st->promiseClass);

    sq_settop(v, top);
}

}  // anonymous namespace


//-----------------------------------------------------------------------------
// Engine integration entry points. Called from SQVM::StartCall (when an async
// function is invoked) and ~SQSharedState (when the runtime owns state that
// must be released before the registry / root VM tear down). Forward declared
// in squirrel/sqstate.h.

SQRESULT wrap_generator(HSQUIRRELVM v, struct SQGenerator *gen, SQObjectPtr &out)
{
    if (!_ss(v)->_asyncState) {
        sq_throwerror(v, "async function called but sqasync runtime is not bound");
        return SQ_ERROR;
    }

    SQObjectPtr genObj(gen);

    HSQOBJECT taskInstance;
    sq_resetobject(&taskInstance);
    bool ok = buildTaskPromise(v, genObj, taskInstance);

    if (!ok)
        return sq_throwerror(v, "sqasync: cannot create task-promise (runtime missing or shutting down)");

    // Transfer the addref'd taskInstance into the engine's caller slot.
    out = taskInstance;
    sq_release(v, &taskInstance);
    return SQ_OK;
}


void shutdown(struct SQSharedState *ss)
{
    AsyncState *st = ss->_asyncState;
    if (!st) return;

    HSQUIRRELVM v = st->rootVm;
    SQAllocContext ctx = sq_getallocctx(v);

    // Inbox drain under the lock: concurrent producers either land their post
    // before shuttingDown is set (drained here) or see it after the lock and bail.
    {
        std::lock_guard<std::mutex> lk(st->inboxLock);
        st->shuttingDown = true;
        for (SQUnsignedInteger32 i = 0; i < st->inbox.size(); ++i)
            st->queue.push_back(st->inbox._vals[i]);
        st->inbox.resize(0);
    }

    // Dispatch each queued entry so it can release its refs / free its payload.
    // With shuttingDown set, runStep takes its early-out and any reentrant
    // scheduleStep calls early-return, so the queue cannot grow during the walk.
    for (SQUnsignedInteger32 i = 0; i < st->queue.size(); ++i) {
        QueueEntry &e = st->queue._vals[i];
        if (e.kind == QueueEntry::K_Callback)
            e.fn(e.user);
        else
            runStep(v, e.taskInstance, e.resumeKind, e.resumeValue);
    }
    st->queue.resize(0);

    // Pending task-promises are torn down by _refs_table.Finalize() later in
    // ~SQSharedState: dropping p->generator's slot starts the closure->outer->
    // instance cascade, and promise_releasehook's null-vm path handles cleanup
    // without needing the (already-Null'd) root VM. No force-settle needed.
    sq_release(v, &st->promiseClass);

    st->~AsyncState();
    sq_free(ctx, st, sizeof(AsyncState));
    ss->_asyncState = nullptr;
}


//-----------------------------------------------------------------------------
// Public C API.

SQUIRREL_API SQRESULT bind(HSQUIRRELVM v)
{
    if (_ss(v)->_asyncState) {
        assert(!"[sqasync] bind() called twice");
        return SQ_ERROR;
    }

    HSQUIRRELVM rootVm = _thread(_ss(v)->_root_vm);
    SQAllocContext ctx = sq_getallocctx(v);
    void *mem = sq_malloc(ctx, sizeof(AsyncState));
    AsyncState *st = new (mem) AsyncState(rootVm, ctx);

    buildAndCachePromiseClass(v, st);

    _ss(v)->_asyncState = st;
    return SQ_OK;
}


SQUIRREL_API SQInteger pump(HSQUIRRELVM v, SQInteger maxSteps)
{
    AsyncState *st = _ss(v)->_asyncState;
    if (!st || st->shuttingDown)
        return 0;

    // Splice the inbox into the queue tail under the lock. Posts arriving
    // after the splice wait on the now-empty inbox until the next pump.
    {
        std::lock_guard<std::mutex> lk(st->inboxLock);
        for (SQUnsignedInteger32 i = 0; i < st->inbox.size(); ++i)
            st->queue.push_back(st->inbox._vals[i]);
        st->inbox.resize(0);
    }

    // Drain from the front. Pushes from inside dispatched callbacks land at the
    // back and may be processed in the same pump, up to maxSteps; the rest wait
    // for the next pump. We index from the front and erase the consumed prefix
    // with a single memmove at the end.
    SQInteger drained = 0;
    SQUnsignedInteger32 head = 0;
    while (drained < maxSteps && head < st->queue.size()) {
        // Copy out by value: POD fields plus already-addref'd HSQOBJECTs make
        // this a clean ownership transfer to the local. Bump head BEFORE
        // dispatching - if a reentrant push grows _vals, we don't want our
        // head slot moved out from under us.
        QueueEntry e = st->queue._vals[head];
        ++head;
        if (e.kind == QueueEntry::K_Callback)
            e.fn(e.user);
        else
            runStep(v, e.taskInstance, e.resumeKind, e.resumeValue);
        ++drained;
    }

    if (head > 0) {
        SQUnsignedInteger32 remaining = st->queue.size() - head;
        if (remaining > 0)
            memmove(&st->queue._vals[0], &st->queue._vals[head], remaining * sizeof(QueueEntry));
        st->queue.resize(remaining);
    }
    return drained;
}


SQUIRREL_API SQBool has_pending(HSQUIRRELVM v)
{
    // "Pending" means pump() has work to drive forward (queue or inbox).
    // Tasks parked on a never-settling Promise are NOT pending - they
    // need external input. Drain loops should OR this with external
    // producers' own pending state (timer queues, HTTP, etc).
    AsyncState *st = _ss(v)->_asyncState;
    if (!st)
        return SQFalse;
    if (st->queue.size() > 0)
        return SQTrue;
    std::lock_guard<std::mutex> lk(st->inboxLock);
    return (st->inbox.size() > 0) ? SQTrue : SQFalse;
}


static QueueEntry makeCallbackEntry(VmCallback fn, void *user)
{
    QueueEntry e;
    e.kind = QueueEntry::K_Callback;
    e.fn = fn;
    e.user = user;
    e.resumeKind = Resume_Send;      // unused on Callback
    sq_resetobject(&e.taskInstance); // unused on Callback
    sq_resetobject(&e.resumeValue);  // unused on Callback
    return e;
}


SQUIRREL_API SQRESULT post_on_vm_thread(HSQUIRRELVM v, VmCallback fn, void *user)
{
    AsyncState *st = _ss(v)->_asyncState;
    if (!st || st->shuttingDown)
        return SQ_ERROR;
    st->queue.push_back(makeCallbackEntry(fn, user));
    return SQ_OK;
}


SQUIRREL_API SQRESULT post_from_any_thread(HSQUIRRELVM v, VmCallback fn, void *user)
{
    // Lockless read: shutdown frees `st` from ~SQSharedState, which the
    // embedder must run only after all VM-bound workers have joined.
    AsyncState *st = _ss(v)->_asyncState;
    if (!st)
        return SQ_ERROR;

    std::lock_guard<std::mutex> lk(st->inboxLock);
    if (st->shuttingDown)
        return SQ_ERROR;
    st->inbox.push_back(makeCallbackEntry(fn, user));
    return SQ_OK;
}


SQUIRREL_API SQRESULT push_promise_class(HSQUIRRELVM v)
{
    AsyncState *st = _ss(v)->_asyncState;
    if (!st || sq_isnull(st->promiseClass))
        return sq_throwerror(v, "sqasync: bind() must be called before push_promise_class()");
    sq_pushobject(v, st->promiseClass);
    return SQ_OK;
}


SQUIRREL_API SQRESULT create_promise(HSQUIRRELVM v, HSQOBJECT *out)
{
    sq_resetobject(out);

    AsyncState *st = _ss(v)->_asyncState;
    if (!st || st->shuttingDown)
        return sq_throwerror(v, "sqasync::create_promise: runtime not bound");

    if (!createPromiseInstanceOnStack(v, st))
        return sq_throwerror(v, "sqasync::create_promise: cannot create Promise");

    sq_getstackobj(v, -1, out);
    sq_addref(v, out);
    sq_pop(v, 1);
    return SQ_OK;
}


SQUIRREL_API SQRESULT resolve_promise(HSQUIRRELVM v, HSQOBJECT promise, HSQOBJECT value)
{
    PromiseImpl *p = getPromiseFromInstance(v, promise);
    if (!p)
        return sq_throwerror(v, "sqasync::resolve_promise: handle is not a Promise instance");
    return externalSettle(v, p, promise, value, /*reject*/false);
}


SQUIRREL_API SQRESULT reject_promise(HSQUIRRELVM v, HSQOBJECT promise, HSQOBJECT reason)
{
    PromiseImpl *p = getPromiseFromInstance(v, promise);
    if (!p)
        return sq_throwerror(v, "sqasync::reject_promise: handle is not a Promise instance");
    return externalSettle(v, p, promise, reason, /*reject*/true);
}


SQUIRREL_API SQInteger sqasync_register(HSQUIRRELVM v)
{
    sq_pushstring(v, "Promise", -1);
    if (SQ_FAILED(push_promise_class(v))) {
        sq_pop(v, 1);  // pop the key
        return SQ_ERROR;
    }
    sq_newslot(v, -3, SQFalse);  // consumes key+value; module table remains on top
    return 0;
}

}  // namespace sqasync
