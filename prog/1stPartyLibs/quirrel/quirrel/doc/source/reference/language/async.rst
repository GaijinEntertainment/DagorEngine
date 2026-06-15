.. _async:


=============
Async / await
=============

.. index::
    single: Async
    single: Await
    single: Future

Quirrel ships a cooperative ``async``/``await`` runtime built on top of
generators. It lets a function suspend at an ``await`` point, return a
``Future`` to its caller right away, and resume later on the same VM
thread when the awaited value is ready.

.. note::
    The runtime must be initialized by the host before any ``async``
    function runs (see :ref:`async_runtime`). The standalone Quirrel
    interpreter (``sq``) does this automatically; embedders must call
    ``sqasync::bind`` and arrange for ``sqasync::pump`` to be called.

---------------------------
Background and design notes
---------------------------

Modelled on JavaScript ``Promise``; named ``Future`` because the script
surface has no ``.reject()`` - faulted state comes only from a throw
inside an ``async``.

``await`` - only valid inside an ``async`` function - is how a
caller consumes a Future's value. A consequence is that any caller
that needs to ``await`` a ``Future`` (rather than just pass it
through) must itself be ``async`` - the requirement propagates outward.

This is the pattern Bob Nystrom describes as "function coloring"
in `What Color is Your Function?
<https://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/>`_,
recommended reading for the analytical framing and the trade-offs
the JS-shaped model carries.

-------------------------
Declaring async functions
-------------------------

.. index::
    single: Async; declaration

Prefix ``async`` in front of any function declaration form to make it
async-capable. Calling such a function never runs the body synchronously:
it immediately returns a ``Future`` (a "task-future") and the body's
first step is queued for the next ``pump()``.

Statement form::

    async function loadStuff() {
      let data = await fetch()
      return data
    }

Expression form (anywhere a regular ``function`` expression is legal)::

    let f = async function() {
      return await pending()
    }

Lambda form::

    let f = async @() "direct"
    let g = async @(p) await p

Async methods on a class::

    class Counter {
      count = 0
      async function bump(n) {
        let delta = await someSource(n)
        this.count = this.count + delta
        return this.count
      }
    }

The body of an async function may contain ``await`` (the whole point) and
may ``return`` a value or ``throw``: the returned task-future settles
fulfilled with the returned value or faulted with the thrown error.

Returning a ``Future`` from an async function does NOT yield a
``Future<Future<value>>`` to the caller: the runtime adopts the
returned future's settlement (the JS-style "Promise Resolution
Procedure"). ``return p`` is therefore equivalent to ``return await p``
from the caller's perspective, with one fewer suspension::

    async function inner() {
      let p = Future()
      p.resolve(42)
      return p                    // outer task-future adopts p
    }
    async function outer() {
      let v = await inner()       // v is 42, not a Future
      print(v)
    }

If the returned future is still pending, the task-future stays pending
until the inner settles, then mirrors its fulfilment or fault. The
fault path is not unwrapped: ``throw someFuture`` faults the
task-future with the Future object as the value (matching JS).

Because ``await`` accepts any ``Future`` regardless of who returned
it, the natural shape for a callback-to-Future adapter is a sync
function::

    function timeoutFuture(sec, value): instance {
      let p = Future()
      gui_scene.setTimeout(sec, @() p.resolve(value))
      return p
    }
    async function f() {
      let v = await timeoutFuture(0.1, "x")   // v is "x"
    }

The ``: instance`` return annotation tells the static analyzer that the
function may return a Future, suppressing the ``w328 redundant-await``
diagnostic at callers. Without an annotation the analyzer still warns
because it cannot infer the body's return type; annotate, or mark the
helper ``async function`` if you prefer the analyzer to know
automatically.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Where ``async`` is not allowed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* On constructors: ``async constructor`` is rejected because a
  constructor must return the new instance, not a Future.
* On metamethods: ``_add``, ``_unm``, ``_cmp``, ``_tostring`` and the
  rest of the metamethod set must return their expected value
  synchronously, so ``async`` is rejected on them.

----------
``await``
----------

.. index::
    single: Await

``await <expr>`` evaluates ``<expr>`` and suspends the async function
until the result is available.

* If the result is a ``Future`` in the ``pending`` state, the current
  async function parks on it. When the future is later settled, the
  scheduler queues a resume step:

  * Fulfilled -> the await expression yields the resolved value.
  * Faulted -> the thrown value is re-raised at the await site
    (a surrounding ``try/catch`` will catch it).

* If the result is a ``Future`` already in the ``fulfilled`` or
  ``faulted`` state, no parking happens: the resume step is queued
  and runs at the next scheduler step, typically within the same
  ``pump()`` call.

* If the result is not a Future, the value is delivered to the
  await site as-is at the next scheduler step (no transient Future
  is allocated). This is what makes ``await scalar`` a valid no-op.

``await`` is only valid inside an async function. Using it in a
plain function is a compile-time error.

``await`` and partial expressions::

    async function compute() {
      let r = await a + await b               // two suspensions
      let r2 = await f(await g())             // nested call
    }

Each ``await`` is a real suspension point; in-flight expression
operands are preserved across the suspension on the VM stack.

``await`` inside loops works as expected::

    async function loop() {
      for (local i = 0; i < 3; i++) {
        let v = await source(i)
        print(v)
      }
      foreach (idx, item in array) {
        let v = await source(item)
        ...
      }
    }

----------------------
The ``async`` module
----------------------

.. index::
    single: Async; module
    single: Future; class

The ``async`` built-in module exposes the ``Future`` class::

    from "async" import Future

^^^^^^^^^^^^^^^^^^^
``Future`` class
^^^^^^^^^^^^^^^^^^^

A Future is in one of three states: ``"pending"``, ``"fulfilled"``,
``"faulted"``. State transitions are monotonic: once settled
(fulfilled or faulted), a Future stays in that state and further
``resolve()`` calls are silent no-ops.

.. sq:function:: Future()

    Construct a fresh pending future. No arguments - there is no
    JS-style executor callback. Settle it later with ``.resolve``.
    A future built this way can only be fulfilled directly; there is
    no ``reject``. It becomes faulted only second-hand, by adopting a
    faulted future - for example when you ``.resolve`` it with the
    result of an ``async`` call whose body threw.

.. sq:function:: Future.getState()

    Returns the current state as one of the strings
    ``"pending"`` / ``"fulfilled"`` / ``"faulted"``.

.. sq:function:: Future.resolve(value)

    Settle a pending future as fulfilled with ``value``. Idempotent on
    an already-settled future: a Future transitions from pending to
    settled exactly once; second and later calls to ``.resolve`` are
    silently ignored and leave the original state and value unchanged.

    If ``value`` is itself a ``Future``, the receiver adopts its
    settlement instead of storing the Future as the value
    (chain-unwrap, matching JS). If the inner future is already
    fulfilled the receiver settles immediately with the inner's value;
    if faulted, the receiver settles faulted with the same value;
    if pending, the receiver stays pending until the inner settles and
    then mirrors it. Recursive: an inner Future that itself wraps
    another Future unwraps all the way to a non-Future value.

    Adoption locks the receiver: once ``p.resolve(otherFuture)`` has
    been accepted, subsequent ``p.resolve(...)`` calls are silent
    no-ops, exactly as on an already-settled future. Only
    ``otherFuture``'s eventual settlement can complete ``p``.

    Two errors throw:

    * Resolving a future with itself (``p.resolve(p)``) or with a
      Future that chains back to ``p`` via any depth of adoption
      (``a.resolve(b); b.resolve(a)`` or longer chains): would form an
      uncollectable cycle, detected by walking the adoption chain.
    * Resolving the future returned by an ``async`` function: a
      task-future is owned by its generator and must settle through
      the body's return / throw, not externally.

Example::

    let p = Future()
    p.resolve(42)

    async function f() {
      let x = await p
      print(x)            // 42
    }
    f()

.. note::

   A pending ``p.resolve(q)`` makes ``p`` and ``q`` reference each
   other so settlement can propagate. If nothing settles ``q`` and the
   script drops all external references, the pair leaks until the VM
   is closed; refcounting cannot break the cycle, and the
   unhandled-error diagnostic does not surface it.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Combinators: ``Future.all`` and ``Future.race``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two static combinators run several futures concurrently and settle a
single result. Each takes one array and returns a fresh ``Future``. A
non-array argument (or none at all) throws synchronously at the call
site, not asynchronously - the validation runs on the calling frame, so
a plain ``try/catch`` (without ``await``) catches it. Array elements that
are not Futures are awaited as-is (an immediate value), exactly like
``await`` on a scalar.

Both treat a fault as an outcome that decides the result, never one to
wait out: ``all`` faults on the first faulting input; ``race`` lets a
fault win exactly like a fulfilment. A propagating fault carries its
thrown value and trace. There is deliberately no ``Future.any`` -
"keep waiting past a fault until one fulfils" would suppress a real bug.

.. sq:function:: Future.all(arr)

    Run all inputs concurrently and fulfil with an array of their values,
    in input order, once every input fulfils. Fail-fast: on the first
    input to fault, the result faults with that thrown value and later
    outcomes are ignored. An empty array fulfils immediately with ``[]``.

    ::

        async function f() {
          let r = await Future.all([loadA(), loadB()])   // both run concurrently
          use(r[0], r[1])
        }

    Fail-fast means ``all`` does not wait for the remaining inputs once
    one faults, and does not aggregate (matching JS ``Promise.all``; it
    differs from .NET ``Task.WhenAll``, which waits and aggregates). For
    partial-success - "run N, collect every outcome, success or fault" -
    wrap each input so it never faults and ``all`` over the wrappers,
    rather than wrapping the aggregate ``try { all() }``, which a
    fail-fast ``all`` would leave holding only the first fault.

.. sq:function:: Future.race(arr)

    Settle as the first input to settle, whichever way it settled: the
    first fulfilment fulfils the result, the first fault faults it.
    A losing fault is discarded (handled by the combinator - no
    unhandled-error diagnostic).

    When several inputs are already settled before the race runs, the
    one earliest in array order wins (matching JS ``Promise.race``
    reaction order). A non-future element settles immediately, so it
    beats a still-pending future input.

    Unlike JS ``Promise.race``, an empty array throws synchronously
    rather than returning a forever-pending future: an empty race can
    never settle, which is almost always a bug, so it is reported at the
    call site.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Task-futures vs. manually constructed futures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two flavours of Future exist at runtime:

* Manual / bare future - created by ``Future()`` from script (or
  ``sqasync::future_create`` from C++). Settle it explicitly through
  ``.resolve`` (or, from native code, ``sqasync::future_throw`` for
  a worker-thread bug). This is what bindings hand to script when
  they begin an async operation.
* Task-future - created implicitly when an ``async`` function is
  called. It is owned by the function's generator frame; calling
  ``.resolve`` on it from outside throws an error. The function's
  own ``return`` / ``throw`` is the only way to settle it.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Subclassing ``Future``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Subclasses of ``Future`` work as long as the subclass constructor
chains to ``super.constructor()``. Skipping it leaves the instance
without its internal state, in which case ``getState`` /
``resolve`` throw ``"invalid 'this'"``.

--------------------------------------
Scheduling model and execution order
--------------------------------------

.. index::
    single: Async; scheduling

Calling an async function is non-blocking. The body starts running only
when the host calls ``sqasync::pump``. As a result, code that comes
after the call in the same script-frame runs first::

    async function main() {
      print("body start\n")
      let x = await fulfilled
      print("body got: ")
      print(x)
    }

    print("before call\n")
    main()
    print("script done\n")

Output::

    before call
    script done
    body start
    body got: 42

The scheduler is a FIFO queue. Long chains of immediately-resolved
awaits are drained inside one ``pump`` call (up to its per-call cap);
other tasks interleave in queue order. The implementation does not
recurse on the C stack across ``await``: it is safe to await tens of
thousands of times inside a single async function.

Multiple async tasks may park on the same pending future; when the
future settles, every waiter is resumed in FIFO order with the same
settled value (or with the same fault value).

-------------------------------
Errors and faulted futures
-------------------------------

Inside an async function, a ``throw`` (or any unhandled runtime error)
faults the task-future with the thrown value. ``await`` re-raises the
fault at the await site::

    async function faulter() {
      throw "boom"
    }

    async function consumer() {
      try {
        await faulter()
      } catch (e) {
        print("caught: " + e)       // "caught: boom"
      }
    }
