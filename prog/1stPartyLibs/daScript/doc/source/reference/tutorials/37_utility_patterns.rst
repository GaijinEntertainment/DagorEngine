.. _tutorial_utility_patterns:

==========================================
Utility Patterns (defer + static_let)
==========================================

.. index::
    single: Tutorial; Utility Patterns
    single: Tutorial; defer
    single: Tutorial; static_let
    single: Tutorial; Scope Exit
    single: Tutorial; Persistent Variables

This tutorial covers two powerful utility macros: ``defer`` for Go-style
scope-exit cleanup, and ``static_let`` for C-style persistent local variables.

.. code-block:: das

    require daslib/defer
    require daslib/static_let

defer — scope-exit cleanup
==========================

``defer`` moves a block of code to the ``finally`` section of the enclosing
scope.  It runs when the scope exits — whether normally or via an early
return.  This is Go's ``defer`` pattern: set up a resource, then immediately
defer its cleanup, keeping the two related operations adjacent.

.. code-block:: das

    print("step 1\n")
    defer() {
        print("cleanup (deferred)\n")
    }
    print("step 2\n")
    print("step 3\n")
    // output:
    // step 1
    // step 2
    // step 3
    // cleanup (deferred)

LIFO ordering
-------------

When multiple defers exist in the same scope, they run in LIFO order — the
last deferred block runs first, just like Go:

.. code-block:: das

    defer() {
        print("first defer (runs last)\n")
    }
    defer() {
        print("second defer (runs first)\n")
    }
    print("main body\n")
    // output:
    // main body
    // second defer (runs first)
    // first defer (runs last)

Early return
------------

Deferred blocks run even when the function returns early.  This makes
``defer`` ideal for resource cleanup:

.. code-block:: das

    def defer_early_return(do_early : bool) {
        defer() {
            print("cleanup always runs\n")
        }
        if (do_early) {
            print("returning early\n")
            return
        }
        print("reached the end\n")
    }

Scope attachment
----------------

``defer`` attaches to the nearest enclosing scope — so inside an ``if``
block, it only runs when that ``if`` block exits, not the function scope:

.. code-block:: das

    print("before if\n")
    if (true) {
        defer() {
            print("deferred inside if\n")
        }
        print("inside if\n")
    }
    print("after if\n")
    // output:
    // before if
    // inside if
    // deferred inside if
    // after if

Practical: paired acquire/release
----------------------------------

The classic use case: acquire a resource, immediately defer its release:

.. code-block:: das

    acquire_resource("database")
    defer() {
        release_resource("database")
    }
    acquire_resource("file")
    defer() {
        release_resource("file")
    }
    // On scope exit: file released first (LIFO), then database

.. note::

   ``defer`` is transformed at compile time — the deferred block ALWAYS
   moves to ``finally``, regardless of whether execution "reached" the
   ``defer()`` call at runtime.  This differs from Go, where ``defer`` is
   runtime-conditional.

static_let — persistent locals
==============================

``static_let`` makes local variable declarations persistent across calls.
The variable is initialized once and retains its value, just like C's
``static`` local variables:

.. code-block:: das

    def call_counter() : int {
        static_let() {
            var count = 0
        }
        count ++
        return count
    }

    // call_counter() returns 1, 2, 3, ... on successive calls

Variables declared inside ``static_let`` are promoted to global scope
but remain accessible by name in the declaring function.

One-time initialization
-----------------------

Use ``static_let`` for expensive one-time setup — the initializer runs
once:

.. code-block:: das

    def get_lookup_table() : int {
        static_let() {
            var lookup <- [10, 20, 30, 40, 50]
        }
        var total = 0
        for (v in lookup) {
            total += v
        }
        return total
    }
    // The array is created once, reused on every call

Named static_let
-----------------

The name argument helps disambiguate when you need multiple ``static_let``
blocks in different functions that might otherwise collide:

.. code-block:: das

    def named_counter_a() : int {
        static_let("counter_a") {
            var n = 0
        }
        n ++
        return n
    }

Combining defer and static_let
==============================

A practical pattern: cache a result with ``static_let``, and use ``defer``
to log when the function exits:

.. code-block:: das

    def cached_computation(input : int) : int {
        static_let() {
            var last_input = -1
            var last_result = 0
        }
        defer() {
            print("  exiting cached_computation({input})\n")
        }
        if (input == last_input) {
            print("  cache hit for {input}\n")
            return last_result
        }
        last_result = input * input + input
        last_input = input
        print("  computed {input} -> {last_result}\n")
        return last_result
    }

Summary
=======

==============================  ====================================================
Feature                         Description
==============================  ====================================================
``defer``                       Moves block to scope's ``finally`` section (LIFO)
``static_let``                  Promotes local declarations to persistent globals
``static_let("name")``          Same, with a name prefix to avoid collisions
``static_let_finalize``         Same as ``static_let``, but deletes on shutdown
==============================  ====================================================

.. seealso::

   Full source: :download:`tutorials/language/37_utility_patterns.das <../../../../tutorials/language/37_utility_patterns.das>`

   Previous tutorial: :ref:`tutorial_pointers`

   Next tutorial: :ref:`tutorial_random`
