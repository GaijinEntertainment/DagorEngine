.. _async:


=============
Async / await
=============

.. index::
    single: Async
    single: Await
    single: Promise

Quirrel ships a cooperative ``async``/``await`` runtime built on top of
generators. It lets a function suspend at an ``await`` point, return a
``Promise`` to its caller right away, and resume later on the same VM
thread when the awaited value is ready.

.. note::
    The runtime must be initialized by the host before any ``async``
    function runs (see :ref:`async_runtime`). The standalone Quirrel
    interpreter (``sq``) does this automatically; embedders must call
    ``sqasync::bind`` and arrange for ``sqasync::pump`` to be called.

-------------------------
Declaring async functions
-------------------------

.. index::
    single: Async; declaration

Prefix ``async`` in front of any function declaration form to make it
async-capable. Calling such a function never runs the body synchronously:
it immediately returns a ``Promise`` (a "task-promise") and the body's
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
may ``return`` a value or ``throw``: the returned task-promise resolves
with the returned value or rejects with the thrown error.

Returning a ``Promise`` from an async function does NOT yield a
``Promise<Promise<value>>`` to the caller: the runtime adopts the
returned promise's settlement (the JS-style "Promise Resolution
Procedure"). ``return p`` is therefore equivalent to ``return await p``
from the caller's perspective, with one fewer suspension::

    async function inner() {
      let p = Promise()
      p.resolve(42)
      return p                    // outer task-promise adopts p
    }
    async function outer() {
      let v = await inner()       // v is 42, not a Promise
      print(v)
    }

If the returned promise is still pending, the task-promise stays pending
until the inner settles, then mirrors its fulfilment or rejection. The
reject path is not unwrapped: ``throw somePromise`` rejects the
task-promise with the Promise object as the reason (matching JS).

Because chain-unwrap also applies to ``await`` on a plain function that
returns a ``Promise``, the natural shape for a callback-to-Promise
adapter is a sync function::

    function timeoutPromise(sec, value): instance {
      let p = Promise()
      gui_scene.setTimeout(sec, @() p.resolve(value))
      return p
    }
    async function f() {
      let v = await timeoutPromise(0.1, "x")   // v is "x"
    }

The ``: instance`` return annotation tells the static analyzer that the
function may return a Promise, suppressing the ``w328 redundant-await``
diagnostic at callers. Without an annotation the analyzer still warns
because it cannot infer the body's return type; annotate, or mark the
helper ``async function`` if you prefer the analyzer to know
automatically.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Where ``async`` is not allowed
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* On table members: ``async`` is only legal on free functions and class
  methods, never inside a table literal.
* On constructors: ``async constructor`` is rejected because a
  constructor must return the new instance, not a Promise.
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

* If the result is a ``Promise`` in the ``pending`` state, the current
  async function parks on it. When the promise is later settled, the
  scheduler queues a resume step:

  * Fulfilled -> the await expression yields the resolved value.
  * Rejected -> the rejection reason is re-raised at the await site
    (a surrounding ``try/catch`` will catch it).

* If the result is a ``Promise`` already in the ``fulfilled`` or
  ``rejected`` state, no parking happens: the resume step is queued
  immediately and the body wakes up on the next pump iteration.

* If the result is not a Promise, the value is delivered to the
  await site as-is on the next pump iteration (no transient Promise
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
    single: Promise; class

The ``async`` built-in module exposes the ``Promise`` class::

    from "async" import Promise

^^^^^^^^^^^^^^^^^^^
``Promise`` class
^^^^^^^^^^^^^^^^^^^

A Promise is in one of three states: ``"pending"``, ``"fulfilled"``,
``"rejected"``. State transitions are monotonic: once settled
(fulfilled or rejected), a Promise stays in that state and further
``resolve()`` / ``reject()`` calls are silent no-ops.

.. sq:function:: Promise()

    Construct a fresh pending promise. No arguments - there is no
    JS-style executor callback. Settle it later through ``.resolve`` /
    ``.reject``.

.. sq:function:: Promise.getState()

    Returns the current state as one of the strings
    ``"pending"`` / ``"fulfilled"`` / ``"rejected"``.

.. sq:function:: Promise.resolve(value)

    Settle a pending promise as fulfilled with ``value``. Idempotent on
    an already-settled promise: a Promise transitions from pending to
    settled exactly once; second and later calls to ``.resolve`` /
    ``.reject`` are silently ignored and leave the original state,
    value and reason unchanged.

    If ``value`` is itself a ``Promise``, the receiver adopts its
    settlement instead of storing the Promise as the value
    (chain-unwrap, matching JS). If the inner promise is already
    fulfilled the receiver settles immediately with the inner's value;
    if rejected, the receiver settles rejected with the same reason;
    if pending, the receiver stays pending until the inner settles and
    then mirrors it. Recursive: an inner Promise that itself wraps
    another Promise unwraps all the way to a non-Promise value.

    Adoption locks the receiver: once ``p.resolve(otherPromise)`` has
    been accepted, subsequent ``p.resolve(...)`` / ``p.reject(...)``
    calls are silent no-ops, exactly as on an already-settled promise.
    Only ``otherPromise``'s eventual settlement can complete ``p``.

    Two errors throw:

    * Resolving a promise with itself (``p.resolve(p)``) or with a
      Promise that chains back to ``p`` via any depth of adoption
      (``a.resolve(b); b.resolve(a)`` or longer chains): would form an
      uncollectable cycle, detected by walking the adoption chain.
    * Resolving the promise returned by an ``async`` function: a
      task-promise is owned by its generator and must settle through
      the body's return / throw, not externally.

.. sq:function:: Promise.reject(reason)

    Settle a pending promise as rejected with ``reason``. Same
    restrictions as ``resolve``: self-reject throws, and rejecting a
    task-promise from outside throws.

    The reject path does not chain-unwrap: ``p.reject(somePromise)``
    stores ``somePromise`` as the rejection reason verbatim. Mirrors the
    JS rule that only the resolve path adopts thenables.

Example::

    let p = Promise()
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
   unhandled-rejection diagnostic does not surface it.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Task-promises vs. manually constructed promises
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Two flavours of Promise exist at runtime:

* Manual / bare promise - created by ``Promise()`` from script (or
  ``sqasync::create_promise`` from C++). Settle it explicitly through
  ``.resolve`` / ``.reject``. This is what bindings hand to script when
  they begin an async operation.
* Task-promise - created implicitly when an ``async`` function is
  called. It is owned by the function's generator frame; calling
  ``.resolve`` or ``.reject`` on it from outside throws an error. The
  function's own ``return`` / ``throw`` is the only way to settle it.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Subclassing ``Promise``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Subclasses of ``Promise`` work as long as the subclass constructor
chains to ``super.constructor()``. Skipping it leaves the instance
without its internal state, in which case ``getState`` /
``resolve`` / ``reject`` throw ``"invalid 'this'"``.

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

Multiple async tasks may park on the same pending promise; when the
promise settles, every waiter is resumed in FIFO order with the same
settled value (or with the same rejection reason).

-------------------------------
Errors, exceptions, rejections
-------------------------------

Inside an async function, a ``throw`` (or any unhandled runtime error)
rejects the task-promise with the thrown value as the rejection
reason. ``await`` re-raises a rejection at the await site::

    async function rejecter() {
      throw "boom"
    }

    async function consumer() {
      try {
        await rejecter()
      } catch (e) {
        print("caught: " + e)       // "caught: boom"
      }
    }
