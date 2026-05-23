.. _async_runtime:


=============
Async runtime
=============

.. index::
    single: Async; embedding
    single: sqasync

The async runtime that powers script-side :ref:`async` is a small
cooperative scheduler with C-level entry points declared in
``include/sqasync.h``. Embedders are responsible for:

1. Calling :cpp:func:`sqasync::bind` once per shared state, before any
   ``async`` function in script can run.
2. Publishing the ``Promise`` class to script (typically through
   ``SqModules::registerAsyncLib`` or by calling
   :cpp:func:`sqasync::sqasync_register` directly).
3. Calling :cpp:func:`sqasync::pump` periodically - once per host frame,
   and in a drain loop on shutdown.


----------------
Threading model
----------------

The affinity unit is the VM. Each VM owns:

* A step queue - single-threaded, no lock. Only the VM thread
  touches it.
* An inbox - protected by a mutex. Any thread may push into it.

Whichever thread calls ``sqasync::pump(v)`` drains the inbox into the
step queue and runs both. Embedders may run different VMs on different
threads; the runtime never assumes a single global thread.

Two posting primitives match this split:

* :cpp:func:`sqasync::post_on_vm_thread` - lock-free, must be called on
  the thread that pumps ``v``.
* :cpp:func:`sqasync::post_from_any_thread` - locks the inbox, callable
  from any thread.

-------------
Lifecycle API
-------------

.. cpp:function:: SQRESULT sqasync::bind(HSQUIRRELVM v)

    Install the async runtime into ``v``'s shared state and build the
    ``Promise`` class. Cache the class internally so further publishing
    paths see the same object.

    Call it once per shared state, before any ``async`` function runs.

.. cpp:function:: SQInteger sqasync::pump(HSQUIRRELVM v, SQInteger maxSteps = 1024)

    Drain the inbox into the step queue and run up to ``maxSteps``
    entries from the queue. Returns the number of entries dispatched.
    Zero means the queues are empty (or the runtime is not bound).

    The per-call cap bounds latency under reentrant scheduling: a
    callback that schedules another callback can extend the same pump
    up to ``maxSteps`` times. Anything beyond that waits for the next
    pump.

    Typical wiring is one ``pump(v)`` per host frame.

.. cpp:function:: SQBool sqasync::has_pending(HSQUIRRELVM v)

    ``SQTrue`` iff the runtime has work it can drive forward on its own
    (step queue or inbox non-empty). Returns ``SQFalse`` when the
    runtime is not bound.

    A task parked on an unsettled Promise does not count here: the
    runtime cannot wake it on its own, only the producer that started
    the work (timer, in-flight HTTP, eventbus, ...) can, by calling
    ``resolve_promise`` / ``reject_promise``. A drain loop that only
    checks ``has_pending`` will therefore exit too soon while such
    producers still have work to deliver - OR the check with whatever
    in-flight signals you already track for them::

        while (sqasync::has_pending(v) || timers.has_due() || http_in_flight)
            sqasync::pump(v);


---------------
Posting work
---------------

.. cpp:function:: SQRESULT sqasync::post_on_vm_thread(HSQUIRRELVM v, VmCallback fn, void *user)

    Schedule ``fn(user)`` to run on the VM thread inside the next
    ``pump()``. Lock-free fast path; must be called on the VM thread.

    Returns ``SQ_ERROR`` if the runtime is not bound or is shutting
    down. On failure, the caller owns ``user`` and must free it.

.. cpp:function:: SQRESULT sqasync::post_from_any_thread(HSQUIRRELVM v, VmCallback fn, void *user)

    Thread-safe variant. Any thread may call; the inbox lock guarantees
    safe enqueue. The callback still runs on the VM thread inside
    ``pump()``.

    Returns ``SQ_ERROR`` if the runtime is not bound or is shutting
    down. On failure, the caller owns ``user`` and must free it.

    Lifetime contract: the embedder must guarantee that
    ``sq_close(v)`` (which invokes the runtime's shutdown path) does
    not happen until all VM-bound worker threads have joined, since
    the shutdown frees the state non-atomically from the consumer
    side.

``VmCallback`` is::

    typedef void (*VmCallback)(void *user);

The runtime owns the ``user`` pointer once a post returns ``SQ_OK``:
when the callback runs (or is drained as part of shutdown), it is the
callback's responsibility to free or reinterpret ``user`` as
appropriate.

---------------------
Promise class & C API
---------------------

.. cpp:function:: SQRESULT sqasync::push_promise_class(HSQUIRRELVM v)

    Push the cached ``Promise`` class onto the stack. Useful for
    embedders that want to instantiate a Promise from C (for example,
    an ``async.delay`` native that returns one). Returns ``SQ_ERROR``
    (and raises an error) if :cpp:func:`sqasync::bind` has not been
    called.

.. cpp:function:: SQRESULT sqasync::create_promise(HSQUIRRELVM v, HSQOBJECT *out)

    Create a fresh ``Promise`` instance and write an addref'd handle
    into ``*out``. Nothing is left on the stack. Caller owns one
    strong ref and must either:

    * settle the promise via ``resolve_promise`` / ``reject_promise``
      and then ``sq_release(v, out)``, or
    * ``sq_release(v, out)`` directly to discard the promise.

    Returns ``SQ_ERROR`` (raised) if the runtime is not bound or
    instance creation fails; ``*out`` is reset to null on error.

.. cpp:function:: SQRESULT sqasync::resolve_promise(HSQUIRRELVM v, HSQOBJECT promise, HSQOBJECT value)
.. cpp:function:: SQRESULT sqasync::reject_promise(HSQUIRRELVM v, HSQOBJECT promise, HSQOBJECT reason)

    Settle a manually-created Promise from C. The payload is kept
    alive by an internal ``sq_addref``. Settling an already-settled
    or already-adopted promise is a silent no-op.

    ``resolve_promise`` applies the JS Promise Resolution Procedure:
    a Promise ``value`` is adopted (``promise`` mirrors its eventual
    settlement); a non-Promise ``value`` fulfills immediately.
    ``reject_promise`` stores ``reason`` verbatim regardless of type.

    Both return ``SQ_ERROR`` and throw if ``promise`` is not a
    Promise instance, is a task-promise (returned by an ``async``
    function), or the payload is ``promise`` itself.
    ``resolve_promise`` additionally throws on a cycle that lies
    entirely within the adoption chain
    (``a.resolve(b); b.resolve(a)``). Cycles that pass through a
    rejection (``a.resolve(b); b.reject(a)`` or
    ``b.reject(a); a.resolve(b)``) are detected during settlement
    and produce a diagnostic reason rather than throwing.

Typical worker -> VM result delivery looks like::

    struct OpCtx {
      HSQOBJECT promise;     // addref'd via create_promise
      HSQOBJECT result;      // built on the VM thread
    };

    // VM thread: kick off the op and hand the promise to script
    HSQOBJECT prom;
    sqasync::create_promise(v, &prom);
    OpCtx *ctx = new OpCtx { prom, /*result=*/{} };
    start_worker(ctx);
    // push the promise back to script as the call result, etc.

    // Worker thread: do the work, then post the completion
    sqasync::post_from_any_thread(v, &on_complete, ctx);

    // on_complete runs on the VM thread during pump():
    static void on_complete(void *user) {
      OpCtx *ctx = static_cast<OpCtx *>(user);
      // build ctx->result here (we are on the VM thread)
      sqasync::resolve_promise(v, ctx->promise, ctx->result);
      sq_release(v, &ctx->promise);
      delete ctx;
    }

-------------------------------
Publishing the ``async`` module
-------------------------------

.. cpp:function:: SQInteger sqasync::sqasync_register(HSQUIRRELVM v)

    SqModules-shaped registration entry point. On entry the new module
    table is expected on top of the stack; the function adds
    ``"Promise"`` -> class into it and leaves the table on top.

    Suitable for ``SqModules::registerStdLibNativeModule`` and
    ``SqModules::registerAsyncLib``.

The simplest wiring for an embedder using ``SqModules`` is::

    sqasync::bind(v);
    module_mgr->registerAsyncLib();   // adds "async" -> { Promise }

After that, script code can write ``from "async" import Promise``.

Embedders that do not use ``SqModules`` can publish the class manually
by pushing it via :cpp:func:`sqasync::push_promise_class` and storing it
wherever they want script to see it.

--------------------------------
Async function call integration
--------------------------------

When the engine encounters a call to a script-side ``async`` function
in ``SQVM::StartCall``, it does not run the body. Instead it builds a
generator from the function's bytecode and hands it to the runtime
through a hidden entry point (``sqasync::wrap_generator``). The
runtime builds a task-promise around the generator, schedules the
first step, and returns the task-promise as the call result. Subsequent
``pump()`` calls drive the generator one ``await`` at a time.

This means an embedder does not need a per-async-function binding:
any Quirrel function declared ``async`` is wired automatically once
:cpp:func:`sqasync::bind` has installed the runtime.

--------
Shutdown
--------

The runtime is owned by the shared state and freed automatically
during ``sq_close``. Once shutdown begins, ``post_from_any_thread``
returns ``SQ_ERROR`` and the caller retains ownership of ``user``;
any queued callbacks are still dispatched so they can release their
refs, and tasks still parked on unsettled promises are reclaimed
without completing their bodies.

To drain pending work cleanly before ``sq_close``, loop on
:cpp:func:`sqasync::has_pending` calling :cpp:func:`sqasync::pump` (see
the note under ``has_pending`` about pairing it with any in-flight
producers you still own).
