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

// Futures form reference cycles only the collector can break (a pending Future owns its
// suspended generator, whose frame closes back over the Future; adoption adds mutual
// Future refs), so without it nothing reclaims them - not even sq_close, whose chain
// sweep is itself GC-gated - and binding the module would leak every pending Future.
#if defined(NO_GARBAGE_COLLECTOR)
#error "sqasync requires the cycle collector"
#endif

namespace sqasync
{

static int future_type_tag_anchor = 0;

static inline SQUserPointer futureTypeTag() { return (SQUserPointer)&future_type_tag_anchor; }


enum Lifecycle : SQUnsignedInteger32
{
    L_Open = 0,        // pending; settle-able
    L_Adopted = 1,     // pending; locked on another Future (`value` is that instance)
    L_Fulfilled = 2,
    L_Faulted = 3,
};


enum ResumeKind
{
    Resume_Initial = 0,   // task-future has never run; first step
    Resume_Send = 1,      // await resolved; inject value at the parked yield slot
    Resume_Throw = 2,     // await faulted; raise inside the resumed frame
};


struct FutureImpl
{
    Lifecycle lifecycle;
    bool isTask;                // latched true in buildTaskFuture
    bool awaited;               // someone has been (or is being) parked on this
    bool reported;              // unhandled-fault diagnostic has fired (sweep or releasehook)

    HSQOBJECT value;            // addref'd when non-null; adoption-target instance when L_Adopted
    HSQOBJECT generator;        // addref'd when non-null; valid only while isTask && L_Open

    // Trace carrier for a non-Error fault (acyclic SQArray of leaf frame tables).
    // SQObjectPtr + future_markhook keep it alive and GC-reachable with no
    // _refs_table rooting, so it lives iff the owning Future does.
    SQObjectPtr faultTrace;

    sqvector<HSQOBJECT> waiters; // each addref'd; mix of awaiting tasks and chain-locked adopters

    SQAllocContext alloc_ctx;   // free path in future_releasehook when vm is null

    // Unhandled-fault diagnostic context (leaf SQStrings). Captured at creation,
    // not at fault: the async closure is Kill()'d during the throwing Execute.
    // SQObjectPtr + future_markhook, like faultTrace.
    SQObjectPtr asyncSourceName;    // null for a bare Future (no generator)
    SQObjectPtr asyncFuncName;
    SQObjectPtr launchSourceName;   // null for a native/root immediate caller
    SQObjectPtr launchFuncName;
    SQInt32 asyncDefLine;
    SQInt32 launchLine;

    FutureImpl(SQAllocContext ctx)
        : lifecycle(L_Open), isTask(false), awaited(false), reported(false),
          waiters(ctx), alloc_ctx(ctx), asyncDefLine(0), launchLine(0)
    {
        sq_resetobject(&value);
        sq_resetobject(&generator);
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
    HSQOBJECT faultTrace;    // K_Resume + Resume_Throw of a non-Error value; addref'd, may be null
};


// Concrete async-runtime state. SQSharedState owns one of these (typed
// pointer, allocated by sqasync::bind, freed by sqasync::shutdown). Forward
// declared in squirrel/sqstate.h.
struct AsyncState
{
    HSQUIRRELVM rootVm;

    // bound class object; addref'd
    HSQOBJECT futureClass;

    // VM-thread-only step queue (no lock)
    sqvector<QueueEntry> queue;

    // protects `inbox` and the `shuttingDown` write
    std::mutex inboxLock;

    // Cross-thread landing pad for K_Callback posts; pump() drains it into queue.
    // (K_Resume entries can't appear here - see QueueEntry above.)
    sqvector<QueueEntry> inbox;

    // written under inboxLock so producers see it
    bool shuttingDown;

    // Faulted Futures that settled since the previous pump-end sweep. Each
    // entry owns one strong ref on the Future instance, released at sweep
    // time (or in shutdown's final drain). VM-thread-only - no lock.
    sqvector<HSQOBJECT> unhandledCandidates;

    AsyncState(HSQUIRRELVM v, SQAllocContext ctx)
        : rootVm(v), queue(ctx), inbox(ctx), shuttingDown(false), unhandledCandidates(ctx)
    {
        sq_resetobject(&futureClass);
    }
};


namespace
{

inline AsyncState *getAsyncState(HSQUIRRELVM v)
{
    return _ss(v)->_asyncState;
}


inline FutureImpl *getFutureFromInstance(HSQUIRRELVM v, const HSQOBJECT &inst)
{
    if (sq_type(inst) != OT_INSTANCE)
        return nullptr;
    // Consider reviewing if we need Future subclassing at all
    bool isFuture = false;
    for (const SQClass *cls = _instance(inst)->_class; cls; cls = cls->_base) {
        if (cls->_typetag == futureTypeTag()) { isFuture = true; break; }
    }
    if (!isFuture)
        return nullptr;
    // null until future_constructor installs the impl, so a subclass that skips
    // super.constructor() returns null here (treated as "not a Future").
    return (FutureImpl *)_instance(inst)->_userpointer;
}


// Resolve `this` (stack slot 1) into a FutureImpl for the script-callable
// methods. Throws and returns nullptr if the receiver isn't a Future instance,
// or is a subclass instance whose super.constructor() was never invoked.
FutureImpl *requireFutureSelf(HSQUIRRELVM v, const char *errMsg)
{
    SQUserPointer up = nullptr;
    if (SQ_FAILED(sq_getinstanceup(v, 1, &up, futureTypeTag())) || up == nullptr) {
        sq_throwerror(v, errMsg);
        return nullptr;
    }
    return (FutureImpl *)up;
}


void runStep(HSQUIRRELVM v, HSQOBJECT taskInstance, ResumeKind kind, HSQOBJECT resumeValue, HSQOBJECT faultTrace);
void scheduleStep(HSQUIRRELVM v, const HSQOBJECT &taskInstance, ResumeKind kind, const HSQOBJECT &resumeValue,
                  const HSQOBJECT *faultTrace = nullptr);
SQRESULT settleTerminal(HSQUIRRELVM v, FutureImpl *p, const HSQOBJECT &selfInstance, const HSQOBJECT &value, bool reject);
SQRESULT resolveAndAdopt(HSQUIRRELVM v, FutureImpl *target, HSQOBJECT targetInstance, HSQOBJECT valueIn, bool throwOnCycle);
SQRESULT externalSettle(HSQUIRRELVM v, FutureImpl *p, const HSQOBJECT &p_inst, const HSQOBJECT &value, bool reject);


//-----------------------------------------------------------------------------
// Future script-class methods.

// Assemble the fault's report trace: a private clone of the captured origin /
// await-hop frames (FutureImpl.faultTrace, the sole carrier) plus the task-root
// and launch-site tail frames. Cloned first so the report-only tail never leaks
// back into the stored carrier (a late awaiter / adoption read must not inherit
// it). Returns null when there is no trace to report.
static SQObjectPtr buildReportTrace(HSQUIRRELVM v, FutureImpl *p)
{
    SQObjectPtr trace;
    if (sq_type(p->faultTrace) == OT_ARRAY)
        trace = _array(p->faultTrace)->Clone();
    if (sq_type(p->asyncSourceName) == OT_STRING) {
        if (sq_isnull(trace))
            trace = SQArray::Create(_ss(v), 0);
        _array(trace)->Append(sq_make_diag_frame(v, SQObjectPtr(p->asyncSourceName),
            SQObjectPtr(p->asyncFuncName), p->asyncDefLine, "taskroot"));
        if (sq_type(p->launchSourceName) == OT_STRING)
            _array(trace)->Append(sq_make_diag_frame(v, SQObjectPtr(p->launchSourceName),
                SQObjectPtr(p->launchFuncName), p->launchLine, "launched"));
    }
    return trace;
}

static void format_reason(const HSQOBJECT &o, char *buf, size_t n)
{
    if (sq_type(o) == OT_STRING) {
        const SQInteger kMax = 96;
        SQInteger len = _string(o)->_len;
        scsprintf(buf, n, "string \"%.*s%s\"",
            (int)(len > kMax ? kMax : len), _stringval(o),
            len > kMax ? "..." : "");
        return;
    }
    sq_describe_fault_value(o, buf, n);
}


// One-line unhandled-fault diagnostic to the VM's error stream. Callers
// are responsible for marking self->reported.
static void emitReleaseHookDiag(HSQUIRRELVM vm, FutureImpl *self)
{
    SQPRINTFUNCTION errfn = _ss(vm)->_errorfunc;
    if (!errfn)
        return;

    char reason[160];
    format_reason(self->value, reason, sizeof(reason));

    SQObjectPtr trace = buildReportTrace(vm, self);

    char line[768];
    if (sq_type(self->asyncSourceName) == OT_STRING) {
        // Task-future: an async function threw. The launch site renders as the
        // trace's `launched at` frame (buildReportTrace always carries it).
        const char *an = sq_type(self->asyncFuncName) == OT_STRING ? _stringval(self->asyncFuncName) : "";
        scsprintf(line, sizeof(line),
            "[sqasync] unhandled error in async %.80s (%.160s:%d); reason: %s\n",
            an, _stringval(self->asyncSourceName), (int)self->asyncDefLine, reason);
    } else {
        // Bare Future faulted from native code via future_throw.
        scsprintf(line, sizeof(line),
            "[sqasync] unhandled error on Future faulted from native code; reason: %s\n", reason);
    }
    errfn(vm, line);

    sq_print_error_trace(vm, trace, errfn);
}


// Mark-hook counterpart to future_releasehook (see SQMARKHOOK). value/generator/
// waiters keep their _refs_table rooting. Marking the diag strings is a no-op
// (SQStrings aren't cycle-collectable), kept uniform.
static void future_markhook(SQUserPointer p, SQCollectable **chain)
{
#ifndef NO_GARBAGE_COLLECTOR
    FutureImpl *self = (FutureImpl *)p;
    SQSharedState::MarkObject(self->faultTrace, chain);
    SQSharedState::MarkObject(self->asyncSourceName, chain);
    SQSharedState::MarkObject(self->asyncFuncName, chain);
    SQSharedState::MarkObject(self->launchSourceName, chain);
    SQSharedState::MarkObject(self->launchFuncName, chain);
#else
    (void)p; (void)chain;
#endif
}


SQInteger future_releasehook(HSQUIRRELVM vm, SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    FutureImpl *self = (FutureImpl *)p;

    // vm is null when we're called from _refs_table.Finalize() during shutdown:
    // ~SQSharedState already Null'd _root_vm. The ref table is nulling every
    // slot we'd sq_release from anyway, so the per-object refcount still balances.
    // (faultTrace + diag strings are SQObjectPtr now; ~FutureImpl releases them.)
    if (vm) {
        // Defensive net for a fault GC'd before either the pump-tick sweep
        // or the shutdown drain ran (e.g. manual collectgarbage() with no
        // pump in between).
        if (self->lifecycle == L_Faulted && !self->awaited && !self->reported) {
            emitReleaseHookDiag(vm, self);
            self->reported = true;
        }

        sq_release(vm, &self->value);
        sq_release(vm, &self->generator);
        for (SQUnsignedInteger32 i = 0; i < self->waiters.size(); ++i)
            sq_release(vm, &self->waiters._vals[i]);
    }

    SQAllocContext ctx = self->alloc_ctx;
    self->~FutureImpl();
    sq_free(ctx, self, sizeof(FutureImpl));
    return 1;
}


SQInteger future_constructor(HSQUIRRELVM v)
{
    SQUserPointer existing = nullptr;
    if (SQ_SUCCEEDED(sq_getinstanceup(v, 1, &existing, futureTypeTag())) && existing != nullptr)
        return sq_throwerror(v, "Future: constructor called more than once");

    SQAllocContext ctx = sq_getallocctx(v);
    void *mem = sq_malloc(ctx, sizeof(FutureImpl));
    FutureImpl *p = new (mem) FutureImpl(ctx);

    if (SQ_FAILED(sq_setinstanceup(v, 1, p))) {
        p->~FutureImpl();
        sq_free(ctx, p, sizeof(FutureImpl));
        return sq_throwerror(v, "cannot create Future");
    }
    sq_setreleasehook(v, 1, future_releasehook);
    return 0;
}


SQInteger future_getState(HSQUIRRELVM v)
{
    FutureImpl *p = requireFutureSelf(v, "Future.getState: invalid 'this'");
    if (!p) return SQ_ERROR;
    const char *s = "pending";
    if (p->lifecycle == L_Fulfilled) s = "fulfilled";
    else if (p->lifecycle == L_Faulted) s = "faulted";
    // L_Adopted reports as "pending" - it is, from the outside.
    sq_pushstring(v, s, -1);
    return 1;
}


// Script entry for Future.resolve. The faulted side is only reachable from
// native code via future_throw, or from a throw escaping an async body.
SQInteger future_resolve_method(HSQUIRRELVM v)
{
    FutureImpl *p = requireFutureSelf(v, "Future: invalid 'this'");
    if (!p) return SQ_ERROR;

    HSQOBJECT v_obj;
    sq_resetobject(&v_obj);
    if (sq_gettop(v) >= 2)
        sq_getstackobj(v, 2, &v_obj);

    HSQOBJECT self_obj;
    sq_resetobject(&self_obj);
    sq_getstackobj(v, 1, &self_obj);

    return SQ_FAILED(externalSettle(v, p, self_obj, v_obj, /*reject*/false)) ? SQ_ERROR : 0;
}


//-----------------------------------------------------------------------------
// Settlement + waiter dispatch.

struct CascadeJob
{
    // K_Adopt unwinds an L_Adopted chain lock; K_FoldFault settles a parked
    // async ancestor (no catch on its frame) inline rather than via a queued
    // Resume_Throw.
    enum Kind { K_Adopt, K_FoldFault };
    Kind kind;
    FutureImpl *p;
    HSQOBJECT instance;    // owns one ref; pins `p` against sq_release teardown
    HSQOBJECT pinned;      // owns one ref; transfers into p->value
    HSQOBJECT faultTrace;  // owns one ref (may be null); value-type trace for this branch
    bool reject;           // K_FoldFault is always true
};

// Each reject branch gets its own trace container (frame tables stay shared).
// Cloned unconditionally: a folded branch appends its `awaited at` frame, so a
// shared array would leak that hop back into the source future's carrier even
// with a single waiter.
static void makeBranchTrace(HSQUIRRELVM v, const HSQOBJECT &src, HSQOBJECT &out)
{
    SQObjectPtr cloned;  // must outlive the addref below, else its dtor drops the only ref
    sq_resetobject(&out);
    if (sq_isnull(src))
        return;
    cloned = _array(src)->Clone();
    out = cloned;
    sq_addref(v, &out);
}

// selfInstance feeds the per-iteration cycle check below; required.
SQRESULT settleTerminal(HSQUIRRELVM v, FutureImpl *p, const HSQOBJECT &selfInstance,
                        const HSQOBJECT &value, bool reject)
{
    if (p->lifecycle == L_Fulfilled || p->lifecycle == L_Faulted)
        return SQ_OK;

    // Pin `value` BEFORE releasing the old p->value: in the L_Adopted ->
    // terminal cascade, releasing the old adoption target can drop the
    // last ref on whoever owns the incoming `value`.
    HSQOBJECT curPinned = value;
    sq_addref(v, &curPinned);

    sqvector<CascadeJob> worklist(p->alloc_ctx);
    SQUnsignedInteger32 head = 0;

    FutureImpl *cur = p;
    bool curReject = reject;
    // Only folded ancestors get an `awaited at` frame; the initial fault `p` is
    // the throwing step, whose origin sub-stack is already on the trace.
    bool curIsFold = false;

    // Non-Error fault: await-hop frames accumulate on this per-node carrier
    // (seeded from p->faultTrace, where runStep or adoption left the origin)
    // instead of on the value, which has no trace slot.
    HSQOBJECT curFaultTrace = p->faultTrace;
    sq_addref(v, &curFaultTrace);

    HSQOBJECT curInstance = selfInstance;
    sq_addref(v, &curInstance);

    bool more = true;
    while (more) {
        // cur->value = cur would form an uncollectable refcount cycle.
        // Substitute and force fault. Matches JS PRP on a chaining cycle.
        if (sq_type(curPinned) == OT_INSTANCE && sq_type(curInstance) == OT_INSTANCE
            && _instance(curPinned) == _instance(curInstance)) {
            sq_pushstring(v, "Future: chaining cycle detected", -1);
            HSQOBJECT subst;
            sq_resetobject(&subst);
            sq_getstackobj(v, -1, &subst);
            sq_addref(v, &subst);
            sq_pop(v, 1);
            sq_release(v, &curPinned);
            curPinned = subst;
            curReject = true;
            // The substitution discards the original fault value; drop its origin trace too.
            sq_release(v, &curFaultTrace);
            sq_resetobject(&curFaultTrace);
        }

        if (cur->lifecycle == L_Adopted) {
            sq_release(v, &cur->value);
            sq_resetobject(&cur->value);
        }

        cur->lifecycle = curReject ? L_Faulted : L_Fulfilled;
        cur->value = curPinned;  // ref transfers from curPinned into cur->value

        // Push every fault unconditionally; the pump-end sweep filters by
        // !awaited && !reported so an intra-tick await suppresses it.
        if (curReject) {
            AsyncState *st = getAsyncState(v);
            if (st) {
                HSQOBJECT pinned = curInstance;
                sq_addref(v, &pinned);
                st->unhandledCandidates.push_back(pinned);
            }
        }

        // A folded ancestor never resumes: Kill it before the generator is
        // released below.
        if (curIsFold && cur->isTask && sq_type(cur->generator) == OT_GENERATOR) {
            SQGenerator *gen = _generator(cur->generator);
            // Frame + locals are read from gen's saved _ci/_stack here, before Kill() (below)
            // resizes _stack to 0.
            if (sq_isnull(curFaultTrace)) {
                SQObjectPtr arr(SQArray::Create(_ss(v), 0));  // keep a ref across the addref below
                curFaultTrace = arr;
                sq_addref(v, &curFaultTrace);
            }
            sq_append_awaited_frame_to(v, _array(curFaultTrace), gen->_ci, gen);
            gen->Kill();
        }

        // Carry the trace onto this node (buildReportTrace reports it).
        // Aliasing the still-held curFaultTrace at the root node is safe.
        if (curReject && !sq_isnull(curFaultTrace)) {
            cur->faultTrace = curFaultTrace;
        }

        sq_release(v, &cur->generator);
        sq_resetobject(&cur->generator);

        // Snapshot up front so any reentrant settle sees an empty list.
        sqvector<HSQOBJECT> drained = cur->waiters;
        cur->waiters.resize(0);

        if (drained.size() > 0) {
            cur->awaited = true;
            ResumeKind kind = curReject ? Resume_Throw : Resume_Send;
            for (SQUnsignedInteger32 i = 0; i < drained.size(); ++i) {
                FutureImpl *w = getFutureFromInstance(v, drained._vals[i]);
                bool foldFault = curReject && w && w->isTask && w->lifecycle == L_Open
                    && sq_type(w->generator) == OT_GENERATOR
                    && _generator(w->generator)->_ci._etraps == 0;
                // foldFault reads _etraps while w is still L_Open, before it
                // becomes `cur` and has its generator torn down.
                SQObjectPtr branchVal(cur->value);
                // The value is shared across waiters; per-branch divergence lives
                // only in the trace carrier, so each reject branch gets its own
                // clone (a folded branch appends its `awaited at` hop to it).
                HSQOBJECT branchTrace;
                if (curReject)
                    makeBranchTrace(v, curFaultTrace, branchTrace);
                else
                    sq_resetobject(&branchTrace);
                if ((w && w->lifecycle == L_Adopted) || foldFault) {
                    CascadeJob job;
                    job.kind = foldFault ? CascadeJob::K_FoldFault : CascadeJob::K_Adopt;
                    job.p = w;
                    job.instance = drained._vals[i];  // waiters-list ref transfers in
                    job.pinned = branchVal;
                    sq_addref(v, &job.pinned);
                    job.faultTrace = branchTrace;  // ref transfers into the job
                    job.reject = curReject;
                    worklist.push_back(job);
                }
                else {
                    scheduleStep(v, drained._vals[i], kind, branchVal, &branchTrace);
                    sq_release(v, &branchTrace);
                    sq_release(v, &drained._vals[i]);
                }
            }
        }

        // Release after the cur reads/writes above: dropping curInstance may
        // free cur synchronously via its releasehook.
        sq_release(v, &curInstance);
        sq_resetobject(&curInstance);
        // This node's carrier ref is done; the next node gets its own from the job.
        sq_release(v, &curFaultTrace);
        sq_resetobject(&curFaultTrace);

        more = false;
        while (head < worklist.size()) {
            CascadeJob job = worklist._vals[head++];
            // Defensive: each adopter sits in exactly one waiters list, so
            // a popped target shouldn't already be terminal. Balance all
            // refs if invariants ever drift.
            if (job.p->lifecycle == L_Fulfilled || job.p->lifecycle == L_Faulted) {
                sq_release(v, &job.pinned);
                sq_release(v, &job.instance);
                sq_release(v, &job.faultTrace);
                continue;
            }
            cur = job.p;
            curInstance = job.instance;
            curPinned = job.pinned;
            curFaultTrace = job.faultTrace;  // ref transfers from the job
            curReject = job.reject;
            curIsFold = (job.kind == CascadeJob::K_FoldFault);
            more = true;
            break;
        }
    }
    return SQ_OK;
}


// Caller MUST hold one strong ref on `targetInstance`; this function
// always consumes it (transferred into q->waiters on adoption, released
// otherwise). Indirect cycles are caught by the L_Adopted forward-follow.
SQRESULT resolveAndAdopt(HSQUIRRELVM v, FutureImpl *target,
                         HSQOBJECT targetInstance, HSQOBJECT valueIn,
                         bool throwOnCycle)
{
    if (target->lifecycle != L_Open) {
        sq_release(v, &targetInstance);
        return SQ_OK;
    }

    // No addref: each visited q is held alive transitively via the chain
    // from valueIn through L_Adopted->value links for the walk duration.
    sqvector<FutureImpl *> visited(target->alloc_ctx);

    HSQOBJECT current = valueIn;
    while (FutureImpl *q = getFutureFromInstance(v, current)) {
        if (q == target) {
            if (throwOnCycle) {
                sq_release(v, &targetInstance);
                return sq_throwerror(v, "Future.resolve: cannot resolve with self");
            }
            // Async-return path: no script frame to throw into.
            sq_pushstring(v, "Future chain-unwrap: self-cycle", -1);
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
            case L_Faulted:
                for (SQUnsignedInteger32 i = 0; i < visited.size(); ++i)
                    visited._vals[i]->awaited = true;
                // Seed the adopter's carrier so a value-type trace survives
                // adoption (settleTerminal reads target->faultTrace).
                if (!sq_isnull(q->faultTrace)) {
                    target->faultTrace = q->faultTrace;
                }
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


SQRESULT externalSettle(HSQUIRRELVM v, FutureImpl *p, const HSQOBJECT &p_inst,
                        const HSQOBJECT &value, bool reject)
{
    // Task-future terminal state is owned by runStep's exit paths.
    if (p->isTask)
        return sq_throwerror(v, reject
            ? "future_throw: cannot fault the future returned by an async function"
            : "Future.resolve: cannot resolve the future returned by an async function");

    if (p->lifecycle != L_Open)
        return SQ_OK;

    if (reject) {
        // Direct self-fault is a script error (uncollectable refcount cycle).
        if (sq_type(value) == OT_INSTANCE && sq_type(p_inst) == OT_INSTANCE
            && _instance(value) == _instance(p_inst))
            return sq_throwerror(v, "future_throw: cannot fault with self");
        return settleTerminal(v, p, p_inst, value, /*reject*/true);
    }

    HSQOBJECT inst_owned = p_inst;
    sq_addref(v, &inst_owned);  // consumed by resolveAndAdopt
    return resolveAndAdopt(v, p, inst_owned, value, /*throwOnCycle*/true);
}


//-----------------------------------------------------------------------------
// Runner: drives a parked task-future one step at a time.


void scheduleStep(HSQUIRRELVM v, const HSQOBJECT &taskInstance, ResumeKind kind, const HSQOBJECT &resumeValue,
                  const HSQOBJECT *faultTrace)
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
    sq_resetobject(&e.faultTrace);
    if (faultTrace && !sq_isnull(*faultTrace)) {
        e.faultTrace = *faultTrace;
        sq_addref(v, &e.faultTrace);
    }
    st->queue.push_back(e);
}


// Resumes a parked async task one step. The caller has addref'd taskInstance,
// resumeValueIn, and faultTraceIn (the last may be null) for us once each.
// resumeValueIn and faultTraceIn are pinned into locals and released here right
// away; taskInstance is released on every return, except when we park on a
// pending await - there its ref transfers into that future's waiters list.
void runStep(HSQUIRRELVM v, HSQOBJECT taskInstance, ResumeKind kind, HSQOBJECT resumeValueIn, HSQOBJECT faultTraceIn)
{
    // Pin resumeValueIn / faultTraceIn into locals so they survive Execute and
    // release automatically on scope exit. Each release + assignment below is
    // one ownership transfer.
    SQObjectPtr resumeValue;
    resumeValue = resumeValueIn;
    sq_release(v, &resumeValueIn);
    SQObjectPtr resumeTrace;
    resumeTrace = faultTraceIn;
    sq_release(v, &faultTraceIn);

    AsyncState *st = getAsyncState(v);
    FutureImpl *p = getFutureFromInstance(v, taskInstance);
    if (!st || st->shuttingDown || !p || p->lifecycle != L_Open) {
        sq_release(v, &taskInstance);
        return;
    }

    SQGenerator *gen = _generator(p->generator);
    SQGenerator::ResumeMode mode =
        (kind == Resume_Throw) ? SQGenerator::ResumeThrow : SQGenerator::ResumeNormal;
    // Seed the carrier from the awaited future: if this re-injected throw
    // escapes uncaught, exception_trap keeps the inherited trace instead of
    // overwriting it with an await-site snapshot.
    if (mode == SQGenerator::ResumeThrow && !sq_isnull(resumeTrace))
        v->_pendingValueFaultTrace = resumeTrace;
    SQObjectPtr outres;
    bool execOk = gen->RunStep(v, outres, mode, resumeValue);

    if (!execOk) {
        // Generator threw an unhandled error. Take a strong ref before clearing
        // _lasterror so we don't lose the value if it was the last holder.
        SQObjectPtr err = v->_lasterror;
        v->_lasterror.Null();  // don't leak across steps
        // Move the throw-site origin (exception_trap) onto the carrier before
        // settleTerminal stitches the await-hop frames onto it.
        if (!sq_isnull(v->_pendingValueFaultTrace)) {
            p->faultTrace = v->_pendingValueFaultTrace;
        }
        v->_pendingValueFaultTrace.Null();
        settleTerminal(v, p, taskInstance, err, true);
        sq_release(v, &taskInstance);
        return;
    }
    // Drop a snapshot left by a fault caught inside the body, so it cannot bleed
    // into a later fault.
    v->_pendingValueFaultTrace.Null();
    if (gen->_state == SQGenerator::eDead) {
        // resolveAndAdopt consumes taskInstance.
        resolveAndAdopt(v, p, taskInstance, outres, /*throwOnCycle*/false);
        return;
    }

    // Yielded an awaitable: either park on a pending Future, or scheduleStep
    // the next step right away. Chains of already-resolved awaits still unwind
    // in a single tick - pump drains entries pushed during dispatch up to maxSteps.
    FutureImpl *q = getFutureFromInstance(v, outres);
    if (!q) {
        scheduleStep(v, taskInstance, Resume_Send, outres);
        sq_release(v, &taskInstance);
        return;
    }
    q->awaited = true;
    if (q->lifecycle != L_Fulfilled && q->lifecycle != L_Faulted) {
        q->waiters.push_back(taskInstance);  // ref transfers; do NOT release
        return;
    }
    ResumeKind nextKind = (q->lifecycle == L_Faulted) ? Resume_Throw : Resume_Send;
    // Carry the awaited future's trace so a re-injected throw escaping this
    // task uncaught still reports its origin.
    scheduleStep(v, taskInstance, nextKind, q->value,
                 q->lifecycle == L_Faulted ? &q->faultTrace : nullptr);
    sq_release(v, &taskInstance);
}


//-----------------------------------------------------------------------------
// Push a fresh Future instance with FutureImpl + releasehook installed.
// Returns the impl pointer (also gettable via _userpointer); on failure,
// returns nullptr with the stack restored to the entry top.
FutureImpl *createFutureInstanceOnStack(HSQUIRRELVM v, AsyncState *st)
{
    sq_pushobject(v, st->futureClass);
    if (SQ_FAILED(sq_createinstance(v, -1))) {
        sq_pop(v, 1);
        return nullptr;
    }

    SQAllocContext ctx = sq_getallocctx(v);
    void *mem = sq_malloc(ctx, sizeof(FutureImpl));
    FutureImpl *p = new (mem) FutureImpl(ctx);

    if (SQ_FAILED(sq_setinstanceup(v, -1, p))) {
        p->~FutureImpl();
        sq_free(ctx, p, sizeof(FutureImpl));
        sq_pop(v, 2);  // drop instance + class
        return nullptr;
    }
    sq_setreleasehook(v, -1, future_releasehook);
    sq_remove(v, -2);  // drop class; instance on top
    return p;
}


// No-op unless `src` is a string, so a null dst reliably means "structurally
// unavailable" rather than an addref that silently failed.
static inline void captureProtoString(const SQObjectPtr &src, SQObjectPtr &dst)
{
    if (sq_type(src) == OT_STRING)
        dst = src;
}


// Capture the diagnostic context. Must run at creation, not fault: the
// async closure is Kill()'d (its _closure Null'd) while the throwing
// Execute unwinds, and the caller frame is gone by fault time.
static void captureDiag(HSQUIRRELVM v, FutureImpl *p, const HSQOBJECT &gen)
{
    if (sq_type(gen) == OT_GENERATOR) {
        const SQObjectPtr &cl = _generator(gen)->_closure;
        if (sq_type(cl) == OT_CLOSURE) {
            SQFunctionProto *fp = _closure(cl)->_function;
            captureProtoString(fp->_sourcename, p->asyncSourceName);
            captureProtoString(fp->_name, p->asyncFuncName);
            p->asyncDefLine = (SQInt32)(fp->_nlineinfos > 0 ? fp->_lineinfos->_first_line : 0);
        }
    }

    // v->ci is the caller here: StartCall already Return<>()'d out of the
    // async fn's own frame before calling wrap_generator.
    SQVM::CallInfo *cci = v->ci;
    if (cci && sq_type(cci->_closure) == OT_CLOSURE) {
        SQFunctionProto *cfp = _closure(cci->_closure)->_function;
        captureProtoString(cfp->_sourcename, p->launchSourceName);
        captureProtoString(cfp->_name, p->launchFuncName);
        p->launchLine = (SQInt32)cfp->GetLine(cci->_ip);
    }
}


// Build a fresh task-future wrapping `gen`, schedule its initial step, and
// write the future instance handle through `outInstance`. Called from the
// engine's StartCall via sqasync::wrap_generator.
bool buildTaskFuture(HSQUIRRELVM v, const HSQOBJECT &gen, HSQOBJECT &outInstance)
{
    AsyncState *st = getAsyncState(v);
    if (!st || st->shuttingDown) return false;

    FutureImpl *p = createFutureInstanceOnStack(v, st);
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

void installCombinators(HSQUIRRELVM v, AsyncState *st)
{
    static const char *combinatorSrc = R"SQ(
        Future.all <- function all(arr = null) {
            if (type(arr) != "array") throw "Future.all: expected an array"
            let n = arr.len()
            let results = array(n)
            if (n == 0) {
                let d = Future(); d.resolve(results); return d
            }
            let done = Future()
            local left = n
            foreach (idx, fut in arr) {
                (async function() {
                    try {
                        results[idx] = await fut;
                        if (--left == 0)
                            done.resolve(results)
                    }
                    catch (_) {
                        done.resolve(fut)
                    }
                })()
            }
            return done
        }
        Future.race <- function race(arr = null) {
            if (type(arr) != "array") throw "Future.race: expected an array"
            if (arr.len() == 0) throw "Future.race: empty array"
            let done = Future()
            foreach (fut in arr) {
                (async function() {
                    try {
                        done.resolve(await fut)
                    }
                    catch (_) {
                        done.resolve(fut)
                    }
                })()
            }
            return done
        }
    )SQ";

    SQInteger prevTop = sq_gettop(v);
    sq_newtable(v);
    sq_registerbaselib(v);
    sq_pushstring(v, "Future", -1);
    sq_pushobject(v, st->futureClass);
    sq_rawset(v, -3);

    HSQOBJECT bindings;
    sq_resetobject(&bindings);
    sq_getstackobj(v, -1, &bindings);

    if (SQ_FAILED(sq_compile(v, combinatorSrc, (SQInteger)strlen(combinatorSrc),
                             "sqasync_combinators", SQTrue, &bindings))) {
        assert(!"[sqasync] combinator source failed to compile");
        sq_settop(v, prevTop);
        return;
    }

    sq_pushnull(v);  // 'this'
    if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue)))
        assert(!"[sqasync] combinator install failed");
    sq_settop(v, prevTop);
}

void buildAndCacheFutureClass(HSQUIRRELVM v, AsyncState *st)
{
    SQInteger top = sq_gettop(v);

    sq_newclass(v, SQFalse);
    sq_settypetag(v, -1, futureTypeTag());
    _class(stack_get(v, -1))->_markhook = future_markhook;

    static const SQRegFunctionFromStr kFutureMethods[] = {
        {future_constructor,    "instance.constructor()",
            "Constructs a pending Future"},
        {future_getState,       "instance.getState(): string",
            "Returns the lifecycle state: 'pending', 'fulfilled' or 'faulted'"},
        {future_resolve_method, "instance.resolve([value])",
            "Settles a pending Future with value (default null); ignored if already settled"},
    };
    for (const auto &m : kFutureMethods)
        sq_new_closure_slot_from_decl_string(v, m.f, 0, m.declstring, m.docstring);

    sq_resetobject(&st->futureClass);
    sq_getstackobj(v, -1, &st->futureClass);
    sq_addref(v, &st->futureClass);

    sq_settop(v, top);

    installCombinators(v, st);
}

}  // anonymous namespace


// Route each still-unhandled fault in unhandledCandidates[0, limit) through
// VM::CallErrorHandler, or fall back to the legacy console line if no
// handler is installed. Entries at [limit, size) stay queued for the next
// sweep - pump() uses this to defer faults that settled during its drain.
// forcedFallback skips the handler unconditionally - see shutdown.
static void sweepUnhandledFaults(HSQUIRRELVM v, AsyncState *st, bool forcedFallback, SQUnsignedInteger32 limit)
{
    if (limit > st->unhandledCandidates.size())
        limit = st->unhandledCandidates.size();
    if (limit == 0)
        return;

    sqvector<HSQOBJECT> batch(st->unhandledCandidates._alloc_ctx);
    for (SQUnsignedInteger32 i = 0; i < limit; ++i)
        batch.push_back(st->unhandledCandidates._vals[i]);

    SQUnsignedInteger32 remaining = st->unhandledCandidates.size() - limit;
    if (remaining > 0)
        memmove(&st->unhandledCandidates._vals[0],
                &st->unhandledCandidates._vals[limit],
                remaining * sizeof(HSQOBJECT));
    st->unhandledCandidates.resize(remaining);

    for (SQUnsignedInteger32 i = 0; i < batch.size(); ++i) {
        HSQOBJECT inst = batch._vals[i];
        FutureImpl *p = getFutureFromInstance(v, inst);
        if (p && p->lifecycle == L_Faulted && !p->awaited && !p->reported) {
            if (!forcedFallback && sq_type(v->_errorhandler) != OT_NULL) {
                v->CallErrorHandler(SQObjectPtr(p->value), buildReportTrace(v, p));
            } else {
                emitReleaseHookDiag(v, p);
            }
            p->reported = true;
        }
        sq_release(v, &inst);
    }
}


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
    bool ok = buildTaskFuture(v, genObj, taskInstance);

    if (!ok)
        return sq_throwerror(v, "sqasync: cannot create task-future (runtime missing or shutting down)");

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
            runStep(v, e.taskInstance, e.resumeKind, e.resumeValue, e.faultTrace);
    }
    st->queue.resize(0);

    // Report any faults that no pump-tick sweep claimed. forcedFallback
    // skips _errorhandler: script code is unsafe during teardown -
    // subsystems between "last pump" and "shutdown" may already be gone.
    sweepUnhandledFaults(v, st, /*forcedFallback*/true, st->unhandledCandidates.size());

    // Pending task-futures are torn down by _refs_table.Finalize() later in
    // ~SQSharedState: dropping p->generator's slot starts the closure->outer->
    // instance cascade, and future_releasehook's null-vm path handles cleanup
    // without needing the (already-Null'd) root VM. No force-settle needed.
    sq_release(v, &st->futureClass);

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

    buildAndCacheFutureClass(v, st);

    _ss(v)->_asyncState = st;
    return SQ_OK;
}


SQUIRREL_API SQInteger pump(HSQUIRRELVM v, SQInteger maxSteps)
{
    AsyncState *st = _ss(v)->_asyncState;
    if (!st || st->shuttingDown)
        return 0;

    // Snapshot pre-drain: faults added during this tick defer to the next
    // sweep so a consumer queued behind them can mark them awaited.
    SQUnsignedInteger32 sweepable = st->unhandledCandidates.size();

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
            runStep(v, e.taskInstance, e.resumeKind, e.resumeValue, e.faultTrace);
        ++drained;
    }

    if (head > 0) {
        SQUnsignedInteger32 remaining = st->queue.size() - head;
        if (remaining > 0)
            memmove(&st->queue._vals[0], &st->queue._vals[head], remaining * sizeof(QueueEntry));
        st->queue.resize(remaining);
    }

    sweepUnhandledFaults(v, st, /*forcedFallback*/false, sweepable);
    return drained;
}


SQUIRREL_API SQBool has_pending(HSQUIRRELVM v)
{
    // "Pending" means pump() has work to drive forward (queue, inbox, or a
    // deferred unhandled-fault sweep). Tasks parked on a never-settling
    // Future are NOT pending - they need external input. Drain loops
    // should OR this with external producers' own pending state (timer
    // queues, HTTP, etc).
    AsyncState *st = _ss(v)->_asyncState;
    if (!st)
        return SQFalse;
    if (st->queue.size() > 0 || st->unhandledCandidates.size() > 0)
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
    sq_resetobject(&e.faultTrace);   // unused on Callback
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


SQUIRREL_API SQRESULT future_push_class(HSQUIRRELVM v)
{
    AsyncState *st = _ss(v)->_asyncState;
    if (!st || sq_isnull(st->futureClass))
        return sq_throwerror(v, "sqasync: bind() must be called before future_push_class()");
    sq_pushobject(v, st->futureClass);
    return SQ_OK;
}


SQUIRREL_API SQRESULT future_create(HSQUIRRELVM v, HSQOBJECT *out)
{
    sq_resetobject(out);

    AsyncState *st = _ss(v)->_asyncState;
    if (!st || st->shuttingDown)
        return sq_throwerror(v, "sqasync::future_create: runtime not bound");

    if (!createFutureInstanceOnStack(v, st))
        return sq_throwerror(v, "sqasync::future_create: cannot create Future");

    sq_getstackobj(v, -1, out);
    sq_addref(v, out);
    sq_pop(v, 1);
    return SQ_OK;
}


SQUIRREL_API SQRESULT future_resolve(HSQUIRRELVM v, HSQOBJECT future, HSQOBJECT value)
{
    FutureImpl *p = getFutureFromInstance(v, future);
    if (!p)
        return sq_throwerror(v, "sqasync::future_resolve: handle is not a Future instance");
    return externalSettle(v, p, future, value, /*reject*/false);
}


SQUIRREL_API SQRESULT future_throw(HSQUIRRELVM v, HSQOBJECT future, HSQOBJECT value)
{
    FutureImpl *p = getFutureFromInstance(v, future);
    if (!p)
        return sq_throwerror(v, "sqasync::future_throw: handle is not a Future instance");
    return externalSettle(v, p, future, value, /*reject*/true);
}


SQUIRREL_API SQInteger sqasync_register(HSQUIRRELVM v)
{
    sq_pushstring(v, "Future", -1);
    if (SQ_FAILED(future_push_class(v))) {
        sq_pop(v, 1);  // pop the key
        return SQ_ERROR;
    }
    sq_newslot(v, -3, SQFalse);  // consumes key+value; module table remains on top
    return 0;
}

}  // namespace sqasync
