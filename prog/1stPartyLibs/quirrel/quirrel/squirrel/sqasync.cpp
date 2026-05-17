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


enum PromiseState
{
    PS_Pending = 0,
    PS_Fulfilled = 1,
    PS_Rejected = 2,
};


enum ResumeKind
{
    Resume_Initial = 0,   // task-promise has never run; first step
    Resume_Send = 1,      // await resolved; inject value at the parked yield slot
    Resume_Throw = 2,     // await rejected; raise inside the resumed frame
};


struct PromiseImpl
{
    SQUnsignedInteger32 state;  // PromiseState
    bool awaited;               // someone has been (or is being) parked on this

    HSQOBJECT value;            // fulfilled value or rejection error; addref'd when non-null
    HSQOBJECT generator;        // task-promise: the SQGenerator; addref'd when non-null

    sqvector<HSQOBJECT> waiters; // task-promise instances waiting on this; each addref'd

    SQAllocContext alloc_ctx;   // free path in promise_releasehook when vm is null

    PromiseImpl(SQAllocContext ctx)
        : state(PS_Pending), awaited(false), waiters(ctx), alloc_ctx(ctx)
    {
        sq_resetobject(&value);
        sq_resetobject(&generator);
    }

    bool isTaskPromise() const { return !sq_isnull(generator); }
};


// Tagged union for the runtime queue. K_Callback comes from post_on_vm_thread
// or post_from_any_thread; K_Resume from scheduleStep. Fields are POD so
// sqvector's memmove growth is safe. K_Resume is VM-thread-only -- the
// HSQOBJECT threading rules forbid addref off the owning thread -- so only
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
    // (K_Resume entries can't appear here -- see QueueEntry above.)
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
SQRESULT settlePromise(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT *self_or_null, const HSQOBJECT &value, bool reject);


//-----------------------------------------------------------------------------
// Promise script-class methods.

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
        if (self->state == PS_Rejected && !self->awaited) {
            SQPRINTFUNCTION errfn = _ss(vm)->_errorfunc;
            if (errfn) errfn(vm, "[sqasync] unhandled promise rejection\n");
        }

        sq_release(vm, &self->value);
        sq_release(vm, &self->generator);
        for (SQUnsignedInteger32 i = 0; i < self->waiters.size(); ++i)
            sq_release(vm, &self->waiters._vals[i]);
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
    if (p->state == PS_Fulfilled) s = "fulfilled";
    else if (p->state == PS_Rejected) s = "rejected";
    sq_pushstring(v, s, -1);
    return 1;
}


// Shared transition core for both _resolve and _reject. `reject` controls the
// resulting state and the resume kind we send to waiters.
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

    return SQ_FAILED(settlePromise(v, p, &self_obj, v_obj, reject)) ? SQ_ERROR : 0;
}


SQInteger promise_resolve_method(HSQUIRRELVM v) { return promise_settle(v, false); }
SQInteger promise_reject_method(HSQUIRRELVM v)  { return promise_settle(v, true); }


//-----------------------------------------------------------------------------
// Settlement + waiter dispatch.

// Single settle path:
//   - script promise.{resolve,reject} and C-API {resolve,reject}_promise:
//     self_or_null = &promise, throws on cycle.
//   - runStep on generator return/throw: self_or_null = nullptr; the
//     generator has no handle to its task-promise, so no cycle is possible.
// Returns SQ_OK on success or already-settled; SQ_ERROR only on cycle.
SQRESULT settlePromise(HSQUIRRELVM v, PromiseImpl *p, const HSQOBJECT *self_or_null,
                       const HSQOBJECT &value, bool reject)
{
    // Settled-twice is a no-op; design doc treats settle as monotonic.
    if (p->state != PS_Pending)
        return SQ_OK;

    // Reject self-referential settle. If we let p->value point at p, the strong
    // ref through _refs_table holds the instance forever -- the cycle collector
    // can't see through the userdata to break it -- so it would leak until
    // sq_close. JS Promise has the same rule.
    if (self_or_null && sq_type(value) == OT_INSTANCE && sq_type(*self_or_null) == OT_INSTANCE
        && _instance(value) == _instance(*self_or_null))
        return sq_throwerror(v, reject
            ? "Promise.reject: cannot reject with self"
            : "Promise.resolve: cannot resolve with self");

    p->state = reject ? PS_Rejected : PS_Fulfilled;
    p->value = value;
    sq_addref(v, &p->value);

    // Generator is done once we settle. Drop the strong ref so the generator
    // (and its captured outers / locals) can be reclaimed promptly.
    sq_release(v, &p->generator);
    sq_resetobject(&p->generator);

    // Snapshot and clear waiters up front, so any reentrant settle (defensive --
    // can't happen today) sees an empty list. The vector copy is just memcpy of
    // POD HSQOBJECTs -- no refcount churn. We release each strong ref after
    // scheduleStep addrefs its own copy into the queue entry.
    sqvector<HSQOBJECT> drained = p->waiters;
    p->waiters.resize(0);

    if (drained.size() > 0) {
        // Clears the unhandled-rejection diagnostic for `p`: someone awaited it.
        p->awaited = true;
        ResumeKind kind = reject ? Resume_Throw : Resume_Send;
        for (SQUnsignedInteger32 i = 0; i < drained.size(); ++i) {
            scheduleStep(v, drained._vals[i], kind, p->value);
            sq_release(v, &drained._vals[i]);
        }
    }
    return SQ_OK;
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
// except when we park on a pending await -- there taskInstance's ref transfers
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
    if (!st || st->shuttingDown || !p || p->state != PS_Pending || sq_isnull(p->generator)) {
        sq_release(v, &taskInstance);
        return;
    }

    // Resume_Send: write the value into the generator's parked yield slot; when
    //   Execute resumes the generator, it picks the value up as the await result.
    // Resume_Throw: preload _lasterror and switch ExecutionType so Execute jumps
    //   straight into the exception trap inside the resumed frame.
    // Resume_Initial: nothing to deliver -- this is the generator's first run.
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
        settlePromise(v, p, /*self_or_null*/ nullptr, err, true);
        sq_release(v, &taskInstance);
        return;
    }
    if (gen->_state == SQGenerator::eDead) {
        settlePromise(v, p, /*self_or_null*/ nullptr, outres, false);
        sq_release(v, &taskInstance);
        return;
    }

    // Yielded an awaitable: either park on a Pending Promise, or scheduleStep
    // the next step right away. Chains of already-resolved awaits still unwind
    // in a single tick -- pump drains entries pushed during dispatch up to maxSteps.
    PromiseImpl *q = getPromiseFromInstance(v, outres);
    if (!q) {
        scheduleStep(v, taskInstance, Resume_Send, outres);
        sq_release(v, &taskInstance);
        return;
    }
    q->awaited = true;
    if (q->state == PS_Pending) {
        q->waiters.push_back(taskInstance);
        // Ref transfers to q->waiters; do NOT release.
        return;
    }
    ResumeKind nextKind = (q->state == PS_Rejected) ? Resume_Throw : Resume_Send;
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
        // dispatching -- if a reentrant push grows _vals, we don't want our
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
    // Tasks parked on a never-settling Promise are NOT pending -- they
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
    e.resumeKind = Resume_Send;            // unused on Callback
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
    return settlePromise(v, p, &promise, value, /*reject=*/false);
}


SQUIRREL_API SQRESULT reject_promise(HSQUIRRELVM v, HSQOBJECT promise, HSQOBJECT reason)
{
    PromiseImpl *p = getPromiseFromInstance(v, promise);
    if (!p)
        return sq_throwerror(v, "sqasync::reject_promise: handle is not a Promise instance");
    return settlePromise(v, p, &promise, reason, /*reject=*/true);
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
