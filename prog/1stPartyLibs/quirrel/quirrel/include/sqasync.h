#pragma once

#include <squirrel.h>

// sqasync: cooperative async/await runtime built on Quirrel generators.
//
// Threading model
// ---------------
// The affinity unit is the VM, not "the main thread." Each VM owns an inbox
// (thread-safe queue) and a step queue (single-threaded). Whichever thread
// calls sqasync::pump(v) drains the inbox into the step queue and runs both.
// Embedders may run different VMs on different threads.
//
// Two posting primitives below: post_on_vm_thread (lock-free, VM thread only)
// and post_from_any_thread (locks the inbox, any thread). See declarations
// for failure / ownership semantics.
//
// HSQOBJECT cross-thread contract: an addref'd HSQOBJECT can travel between
// threads as opaque bytes. Memcpy it, store it, hand it to a worker. The
// non-owning thread must NOT call sq_addref/sq_release on it, dereference
// _unVal.p<T>, or read type/value details. The matching addref/release pair
// happens on the VM thread (typically: addref in future_create on the VM
// thread when starting an op, release in the closure body when it runs at
// drain time).
//
// Practical consequence: capturing a Sqrat::Object in a worker-side lambda
// is broken -- Sqrat's dtor calls sq_release. Hold raw HSQOBJECT in heap
// storage owned by a ctx struct; the VM-side closure (running at drain) is
// the only place that touches Sqrat handles or builds VM objects.

namespace sqasync
{

typedef void (*VmCallback)(void *user);


// Install the async runtime into the VM's shared state and build the Future
// class. The class is cached internally, embedders publish it where they want
// via future_push_class() or via SqModules::registerAsyncLib() / sqasync_register().
//
// Call bind once per shared state, before any async function runs.
// A second call returns SQ_ERROR and asserts in dev builds.
SQUIRREL_API SQRESULT bind(HSQUIRRELVM v);


// Drain the inbox + internal step queue. Call once per host frame, or in a
// drain loop on shutdown. The per-call cap bounds latency under reentrant
// scheduling (a callback that posts another callback). Returns the number of
// callbacks invoked - a zero return is not necessarily side-effect-free: a
// pump with an empty queue may still report previously deferred unhandled
// async faults through the VM error handler before returning.
SQUIRREL_API SQInteger pump(HSQUIRRELVM v, SQInteger maxSteps = 1024);


// True iff the runtime has work it can drive forward on its own (queue or
// inbox non-empty, or a deferred unhandled-fault sweep is pending). Live
// tasks parked on a never-settling Future are NOT pending: they cannot
// advance without external input. Drain loops should OR this with their
// other pending-work signals (timer queues, in-flight HTTP, ...).
SQUIRREL_API SQBool has_pending(HSQUIRRELVM v);


// Schedule fn(user) to run on the VM thread inside the next pump(). VM-thread-only fast path.
// Returns SQ_ERROR if the runtime is not bound or is shutting down; on failure
// the caller owns `user` and must free it.
SQUIRREL_API SQRESULT post_on_vm_thread(HSQUIRRELVM v, VmCallback fn, void *user);


// Thread-safe variant of post_on_vm_thread: any thread may call.
// Returns SQ_ERROR if the runtime is not bound or is shutting down; on failure
// the caller owns `user` and must free it.
SQUIRREL_API SQRESULT post_from_any_thread(HSQUIRRELVM v, VmCallback fn, void *user);


// Push the cached Future class onto the stack. Fails if bind() has not been
// called. Useful for embedders that need to instantiate a Future from the C
// side (e.g. an async.delay native that returns one).
SQUIRREL_API SQRESULT future_push_class(HSQUIRRELVM v);

// Create a fresh Future instance and write an addref'd handle into *out.
// Nothing is left on the stack. Caller owns one strong ref and must either
// settle the future via future_resolve/future_throw + sq_release, or
// release directly. Returns SQ_ERROR (raised) if the runtime is not bound
// or instance creation fails; `*out` is null on error.
SQUIRREL_API SQRESULT future_create(HSQUIRRELVM v, HSQOBJECT *out);

// Settle a Future from C. An internal sq_addref keeps the payload alive.
// Settling an already-settled or already-adopted future is a no-op.
//
// future_resolve applies the JS Promise Resolution Procedure: a Future
// `value` is adopted (mirrors its eventual settlement); other values
// fulfill immediately. future_throw faults the future. Prefer routing
// documented failures as resolved values; reserve future_throw for
// worker-thread bugs with no live Quirrel stack to sq_throwerror into.
//
// Both return SQ_ERROR and throw if `future` is not a Future instance or
// is a task-future. future_resolve also throws on an adoption cycle that
// closes back on `future` (direct self, or a.resolve(b); b.resolve(a)).
// A cycle discovered through an already-faulted future during the
// settlement cascade yields a diagnostic value instead of throwing.
SQUIRREL_API SQRESULT future_resolve(HSQUIRRELVM v, HSQOBJECT future, HSQOBJECT value);
SQUIRREL_API SQRESULT future_throw(HSQUIRRELVM v, HSQOBJECT future, HSQOBJECT value);

// SqModules RegFunc-shaped entry point. On entry the new module table is on
// top of the stack; the function adds "Future" -> class into it and leaves
// the table on top. Suitable for SqModules::registerStdLibNativeModule and
// SqModules::registerAsyncLib.
SQUIRREL_API SQInteger sqasync_register(HSQUIRRELVM v);

}  // namespace sqasync
