.. _tutorial_coroutines:

==========================================
Coroutines
==========================================

.. index::
    single: Tutorial; Coroutines
    single: Tutorial; co_continue
    single: Tutorial; cr_run
    single: Tutorial; cr_run_all
    single: Tutorial; co_await
    single: Tutorial; yeild_from
    single: Tutorial; Cooperative Multitasking

This tutorial covers generator-based coroutines using the
``daslib/coroutines`` module.  Coroutines are functions that can suspend
and resume, enabling cooperative multitasking without threads.

Prerequisites: :ref:`tutorial_iterators_and_generators` (Tutorial 15).

.. code-block:: das

    require daslib/coroutines

Basic coroutines
================

A coroutine is a function annotated with ``[coroutine]``.  Under the hood it
becomes a ``generator<bool>`` state machine.  Use ``co_continue()`` to yield
control (signal "still running"):

.. code-block:: das

    [coroutine]
    def counting_coroutine() {
        print("count: 1\n")
        co_continue()
        print("count: 2\n")
        co_continue()
        print("count: 3\n")
        // Falls through — coroutine finishes
    }

``cr_run`` drives the coroutine to completion (consumes all yields):

.. code-block:: das

    var c <- counting_coroutine()
    cr_run(c)
    // output:
    // count: 1
    // count: 2
    // count: 3

Manual stepping
===============

Since a coroutine is just an ``iterator<bool>``, you can step through it
manually with a ``for`` loop:

.. code-block:: das

    [coroutine]
    def step_coroutine() {
        print("step A\n")
        co_continue()
        print("step B\n")
        co_continue()
        print("step C\n")
    }

    var c <- step_coroutine()
    for (running in c) {
        print("-- yielded (running={running}) --\n")
    }
    print("-- coroutine done --\n")

Each iteration of the ``for`` loop advances the coroutine to the next
``co_continue()`` or end.

Coroutines with state
=====================

Coroutines can have local variables that persist across yields.  Each
``co_continue()`` suspends, and on resume the locals are intact:

.. code-block:: das

    [coroutine]
    def countdown(n : int) {
        var i = n
        while (i > 0) {
            print("{i}...\n")
            i --
            if (i > 0) {
                co_continue()
            }
        }
    }

    var c <- countdown(4)
    cr_run(c)
    print("liftoff!\n")
    // output: 4... 3... 2... 1... liftoff!

cr_run_all — cooperative scheduling
====================================

``cr_run_all`` takes an array of ``Coroutine`` (``iterator<bool>``) and
drives them round-robin until all are done:

.. code-block:: das

    [coroutine]
    def worker(name : string; steps : int) {
        for (i in range(steps)) {
            print("{name}: step {i + 1}/{steps}\n")
            if (i < steps - 1) {
                co_continue()
            }
        }
    }

    var tasks : Coroutines
    tasks |> emplace <| worker("alpha", 3)
    tasks |> emplace <| worker("beta", 2)
    tasks |> emplace <| worker("gamma", 4)
    cr_run_all(tasks)

Each coroutine gets one step per round.  When a coroutine finishes, it is
removed from the array.

co_await — waiting for a sub-coroutine
=======================================

``co_await`` suspends the current coroutine until a sub-coroutine finishes.
This is useful for composing coroutines hierarchically:

.. code-block:: das

    [coroutine]
    def load_data() {
        print("loading data... (tick 1)\n")
        co_continue()
        print("loading data... (tick 2)\n")
        co_continue()
        print("data loaded!\n")
    }

    [coroutine]
    def process_pipeline() {
        print("pipeline: start\n")
        co_continue()
        print("pipeline: awaiting load_data\n")
        co_await <| load_data()
        print("pipeline: processing loaded data\n")
        co_continue()
        print("pipeline: done\n")
    }

yeild_from — delegating to a sub-generator
============================================

``yeild_from`` yields all values from a sub-iterator.  It works with any
generator, not just coroutines:

.. code-block:: das

    def numbers_gen() : iterator<int> {
        return <- generator<int>() <| $() {
            yield 10
            yield 20
            yield 30
            return false
        }
    }

    def combined_gen() : iterator<int> {
        return <- generator<int>() <| $() {
            yield 1
            yeild_from <| numbers_gen()
            yield 2
            return false
        }
    }

    for (v in combined_gen()) {
        print("{v} ")
    }
    // output: 1 10 20 30 2

Practical: cooperative batch processing
=======================================

Simulate processing items in batches, with multiple workers cooperatively
sharing time:

.. code-block:: das

    [coroutine]
    def batch_processor(name : string; total_items : int) {
        let batch_size = 2
        var pos = 0
        while (pos < total_items) {
            var end = min(pos + batch_size, total_items)
            print("{name}: batch [{pos}..{end - 1}]\n")
            pos = end
            if (pos < total_items) {
                co_continue()
            }
        }
    }

    var tasks : Coroutines
    tasks |> emplace <| batch_processor("worker-A", 5)
    tasks |> emplace <| batch_processor("worker-B", 3)
    cr_run_all(tasks)

Summary
=======

==============================  ================================================
Feature                         Description
==============================  ================================================
``[coroutine]``                 Annotation that transforms function to generator
``co_continue()``               Yield control (still running)
``cr_run(c)``                   Drive one coroutine to completion
``cr_run_all(tasks)``           Drive array of coroutines round-robin
``co_await <| sub()``           Suspend until sub-coroutine finishes
``yeild_from <| gen()``         Delegate to sub-generator
``Coroutines``                  Type alias for ``array<iterator<bool>>``
==============================  ================================================

.. seealso::

   :ref:`Iterators and Generators <iterators>` — generator language reference.

   Full source: :download:`tutorials/language/40_coroutines.das <../../../../tutorials/language/40_coroutines.das>`

   Previous tutorial: :ref:`tutorial_dynamic_type_checking`

   Next tutorial: :ref:`tutorial_serialization`
