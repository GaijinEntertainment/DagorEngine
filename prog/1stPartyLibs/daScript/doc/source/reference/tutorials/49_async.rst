.. _tutorial_async:

==========================================
Async / Await
==========================================

.. index::
    single: Tutorial; Async
    single: Tutorial; Await
    single: Tutorial; Async Generators
    single: Tutorial; Cooperative Multitasking

This tutorial covers ``daslib/async_boost`` — an async/await framework
built on top of daslang generators.  Every ``[async]`` function is
transformed at compile time into a state-machine generator.  No threads,
channels, or job queues are involved — everything is single-threaded
cooperative multitasking.

Prerequisites: Tutorial 15 (Iterators and Generators),
Tutorial 40 (Coroutines).

.. code-block:: das

    options gen2
    options no_unused_function_arguments = false

    require daslib/async_boost
    require daslib/coroutines


Void async — the simplest form
================================

An ``[async]`` function with ``: void`` return type becomes a
``generator<bool>`` state machine, just like a ``[coroutine]``.
Call ``await_next_frame()`` to suspend until the next step:

.. code-block:: das

    [async]
    def greet(name : string) : void {
        print("  hello, ")
        await_next_frame()
        print("{name}!\n")
    }

    def demo_void_async() {
        print("=== void async ===\n")
        var it <- greet("world")
        var step = 1
        for (running in it) {
            print("  -- step {step} --\n")
            step ++
        }
        print("  -- done --\n")
    }

Each iteration of the ``for`` loop advances the generator by one
step.  The function body resumes after the last ``await_next_frame()``
and runs until the next one (or until it returns).


Typed async — yielding values
===============================

An ``[async]`` function with a non-void return type yields values
wrapped in ``variant<res:T; wait:bool>``.  Each ``await_next_frame()``
yields ``variant(wait=true)``; each ``yield value`` yields
``variant(res=value)``:

.. code-block:: das

    [async]
    def compute(x : int) : int {
        await_next_frame()  // simulate one frame of work
        yield x * 2
    }

    def demo_typed_async() {
        for (v in compute(21)) {
            if (v is wait) {
                print("  (waiting...)\n")
            } elif (v is res) {
                print("  result = {v as res}\n")  // 42
            }
        }
    }

The consumer checks ``v is wait`` vs ``v is res`` to distinguish
suspension frames from actual results.


Await — waiting for an async result
=====================================

Inside an ``[async]`` function you can ``await`` another async call.
``await`` suspends the parent until the child completes and extracts
the result:

.. code-block:: das

    [async]
    def add_one(x : int) : int {
        await_next_frame()
        yield x + 1
    }

    [async]
    def chained_math() : void {
        var a = 0
        a = await <| add_one(0)     // copy-assign:  a = 1
        a <- await <| add_one(a)    // move-assign:  a = 2
        let b <- await <| add_one(a) // let-bind:    b = 3
        print("  a={a}, b={b}\n")
    }

Three forms of ``await``:

- ``a = await <| fn(args)`` — copy-assign the result
- ``a <- await <| fn(args)`` — move-assign the result
- ``let b <- await <| fn(args)`` — bind to a new variable


Struct return with move semantics
==================================

Async functions can yield structs.  Use ``yield <-`` to move the
result out (useful for non-copyable data):

.. code-block:: das

    struct Measurement {
        sensor_id : int
        value : float
        tag : string
    }

    [async]
    def read_sensor(id : int) : Measurement {
        await_next_frame()
        var m : Measurement
        m.sensor_id = id
        m.value = 3.14
        m.tag = "temperature"
        yield <- m
    }

    [async]
    def process_sensors() : void {
        let m <- await <| read_sensor(1)
        print("  sensor {m.sensor_id}: {m.value} ({m.tag})\n")
    }


Iterating async generators
============================

A typed async function that yields multiple values acts as an
asynchronous generator.  Consumers iterate and check ``v is res``
to extract each value:

.. code-block:: das

    [async]
    def fibonacci_async(n : int) : int {
        var a = 0
        var b = 1
        for (i in range(n)) {
            yield a
            let next_val = a + b
            a = b
            b = next_val
            await_next_frame()
        }
    }

    def demo_async_iteration() {
        print("  fibonacci: ")
        for (v in fibonacci_async(8)) {
            if (v is res) {
                print("{v as res} ")
            }
        }
        print("\n")
    }

Output: ``0 1 1 2 3 5 8 13``


Running tasks
==============

``async_boost`` provides four task runners:

- ``async_run(it)`` — drive a single task to completion
- ``async_run_all(tasks)`` — drive all tasks cooperatively,
  round-robin one step per task per frame
- ``async_timeout(it, max_frames)`` — drive a task for at most
  *max_frames* steps; returns ``true`` if it completed in time
- ``async_race(a, b)`` — drive two tasks cooperatively; returns
  ``0`` if *a* finishes first, ``1`` if *b* finishes first

.. code-block:: das

    [async]
    def worker(name : string; frames : int) : void {
        for (i in range(frames)) {
            print("  {name}: frame {i + 1}/{frames}\n")
            await_next_frame()
        }
    }

``async_run`` steps through a single task:

.. code-block:: das

    var single <- worker("solo", 3)
    async_run(single)

``async_run_all`` interleaves multiple tasks:

.. code-block:: das

    var tasks : array<iterator<bool>>
    tasks |> emplace <| worker("alpha", 2)
    tasks |> emplace <| worker("beta", 3)
    async_run_all(tasks)

``async_timeout`` enforces a deadline:

.. code-block:: das

    var fast <- worker("fast", 2)
    let completed = async_timeout(fast, 10)   // true
    var slow <- worker("slow", 100)
    let timed_out = async_timeout(slow, 3)    // false (timeout)

``async_race`` runs two tasks and returns which finishes first:

.. code-block:: das

    var racer_a <- worker("A", 2)
    var racer_b <- worker("B", 5)
    let winner = async_race(racer_a, racer_b)  // 0 (A wins)


Mixing async with coroutines
==============================

An ``[async]`` function can ``await`` a ``[coroutine]``.  This lets
you compose low-level coroutine logic with high-level async
orchestration:

.. code-block:: das

    [coroutine]
    def tick_counter(n : int) {
        for (i in range(n)) {
            print("  tick {i + 1}\n")
            co_continue()
        }
    }

    [async]
    def orchestrator() : void {
        print("  orchestrator: start\n")
        await_next_frame()
        print("  orchestrator: awaiting coroutine\n")
        await <| tick_counter(3)
        print("  orchestrator: coroutine done\n")
        await_next_frame()
        print("  orchestrator: finish\n")
    }


Full source
============

The complete tutorial source is in
``tutorials/language/49_async.das``.

Run it with::

    daslang.exe tutorials/language/49_async.das


.. seealso::

   Full source: :download:`tutorials/language/49_async.das <../../../../tutorials/language/49_async.das>`

   :ref:`stdlib_async_boost` — ``async_boost`` module reference.

   :ref:`stdlib_coroutines` — coroutines module reference
   (underlying generator framework).

   :ref:`tutorial_coroutines` — Tutorial 40: Coroutines
   (prerequisite).

   Previous tutorial: :ref:`tutorial_apply`

   Next tutorial: :ref:`tutorial_soa`

