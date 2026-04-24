.. _tutorial_integration_c_threading:

.. index::
   single: Tutorial; C Integration; Threading
   single: Tutorial; C Integration; das_environment_get_bound
   single: Tutorial; C Integration; das_context_clone

==============================
 C Integration: Threading
==============================

This tutorial demonstrates how to use daslang contexts and compilation
pipelines across multiple threads from a pure C host application using
the ``daScriptC.h`` API.

Topics covered:

* **Part A** — Running a compiled context on a worker thread
* **Part B** — Compiling a script from scratch on a worker thread
* ``das_environment_get_bound()`` / ``das_environment_set_bound()`` —
  thread-local environment binding
* ``das_reuse_cache_guard_create()`` / ``das_reuse_cache_guard_release()``
  — thread-local free-list cache management
* ``das_context_clone()`` with ``DAS_CONTEXT_CATEGORY_THREAD_CLONE``
* ``DAS_POLICY_THREADLOCK_CONTEXT`` — context mutex for threads


Prerequisites
=============

* Tutorial 1 completed (:ref:`tutorial_integration_c_hello_world`) —
  basic compile → simulate → eval cycle.
* Familiarity with platform threading primitives
  (``_beginthreadex`` on Windows, ``pthread_create`` on POSIX).


Why threading matters
=====================

daslang uses **thread-local storage** (TLS) for its environment object.
Every thread that touches the daslang C API must have a valid environment
bound.  There are two common patterns:

1. **Share the environment** — the worker thread binds the same
   environment as the main thread via ``das_environment_set_bound()``.
   Use this when the worker only *executes* and the main thread is idle.
2. **Independent environment** — the worker thread calls
   ``das_initialize()`` / ``das_shutdown()`` to create and destroy its
   own module registry and compilation pipeline.


New C API functions for threading
=================================

These functions were added to ``daScriptC.h`` specifically for
multi-threaded host applications:

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - Function
     - Purpose
   * - ``das_environment_get_bound()``
     - Return the environment bound to the calling thread (TLS).
   * - ``das_environment_set_bound(env)``
     - Bind an environment on the calling thread.
   * - ``das_reuse_cache_guard_create()``
     - Create a thread-local free-list cache guard (call first on any
       worker thread).
   * - ``das_reuse_cache_guard_release(guard)``
     - Release the guard (call before thread exit).
   * - ``das_context_clone(ctx, category)``
     - Clone a context for use on another thread.
   * - ``DAS_CONTEXT_CATEGORY_THREAD_CLONE``
     - Category constant for thread-owned clones.


The daslang script
==================

A simple script that computes the sum 0 + 1 + ... + 99 = 4950:

.. code-block:: das

   options gen2

   [export]
   def compute() : int {
       var total = 0
       for (i in range(100)) {
           total += i
       }
       return total
   }


Part A — Run on a worker thread
================================

Compile and simulate on the main thread, then clone the context and
run it on a worker thread.

.. code-block:: c

   // Compile with threadlock_context enabled
   das_policies * pol = das_policies_make();
   das_policies_set_bool(pol, DAS_POLICY_THREADLOCK_CONTEXT, 1);
   das_program * prog = das_program_compile_policies(path, fac, tout, lib, pol);
   das_policies_release(pol);

   // Simulate on the main thread
   das_context * ctx = das_context_make(das_program_context_stack_size(prog));
   das_program_simulate(prog, ctx, tout);

   // Clone the context for the worker thread
   das_context * clone = das_context_clone(ctx,
                             DAS_CONTEXT_CATEGORY_THREAD_CLONE);

   // Capture the environment
   das_environment * env = das_environment_get_bound();

   // On the worker thread:
   das_environment_set_bound(env);
   das_function * fn = das_context_find_function(clone, "compute");
   vec4f res = das_context_eval_with_catch(clone, fn, NULL);
   int result = das_argument_int(res);

Key points:

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Function
     - Explanation
   * - ``das_environment_get_bound()``
     - Returns the current thread's TLS environment pointer.
       Must be captured **before** launching the worker.
   * - ``das_environment_set_bound(env)``
     - Binds the environment on the worker thread.  Without this,
       any daslang C API call will crash.
   * - ``das_context_clone()``
     - Creates a new context that shares the simulated program.
       Release with ``das_context_release()``.
   * - ``DAS_CONTEXT_CATEGORY_THREAD_CLONE``
     - Marks the clone as thread-owned for diagnostics and safety.
   * - ``DAS_POLICY_THREADLOCK_CONTEXT``
     - Compile-time policy that gives the context a mutex.  Required
       when the context (or its clone) runs on a non-main thread.


Part B — Compile on a worker thread
====================================

Create a fully independent daslang environment on a new thread.

.. code-block:: c

   // On the worker thread:

   // 1. Thread-local free-list cache — must be first.
   das_reuse_cache_guard * guard = das_reuse_cache_guard_create();

   // 2+3. Register modules + create environment in TLS.
   das_initialize();

   // 4. Standard compile → simulate → eval cycle.
   das_policies * pol = das_policies_make();
   das_policies_set_bool(pol, DAS_POLICY_THREADLOCK_CONTEXT, 1);
   das_program * prog = das_program_compile_policies(path, fac, tout, lib, pol);
   das_policies_release(pol);
   das_context * ctx = das_context_make(das_program_context_stack_size(prog));
   das_program_simulate(prog, ctx, tout);
   das_function * fn = das_context_find_function(ctx, "compute");
   vec4f res = das_context_eval_with_catch(ctx, fn, NULL);
   int result = das_argument_int(res);

   // Cleanup
   das_context_release(ctx);
   das_program_release(prog);
   /* ... release fac, lib, tout ... */

   // 5. Shut down this thread's module system.
   das_shutdown();
   das_reuse_cache_guard_release(guard);

Key points:

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Function
     - Explanation
   * - ``das_reuse_cache_guard_create()``
     - Initializes per-thread free-list caches.
       Must be created **first** on any thread that uses daslang.
   * - ``das_initialize()``
     - Registers module factories and creates a new environment in TLS.
       Safe to call on multiple threads — module registration is
       idempotent.
   * - ``das_shutdown()``
     - Destroys this thread's modules and environment.  Must be
       called before the thread exits.
   * - ``das_reuse_cache_guard_release()``
     - Tears down thread-local caches.  Call **after**
       ``das_shutdown()``.
   * - ``DAS_POLICY_THREADLOCK_CONTEXT``
     - Same policy as Part A — ensures the context has a mutex.


Choosing the right pattern
==========================

.. list-table::
   :widths: 20 40 40
   :header-rows: 1

   * - Criteria
     - Part A (clone + run)
     - Part B (compile on thread)
   * - Compilation cost
     - Zero on worker (pre-compiled)
     - Full compilation on worker
   * - Isolation
     - Shares environment with main
     - Fully independent
   * - Concurrent compilation
     - Not safe (shared env)
     - Safe (separate env per thread)
   * - Use case
     - Game threads running pre-compiled AI/logic
     - Build servers, parallel test runners


Portable threading
==================

The tutorial uses ``_beginthreadex`` on Windows and ``pthread_create``
on POSIX, wrapped in a small helper.  The ``DAS_C_INTEGRATION_TUTORIAL``
CMake macro already links ``Threads::Threads``, so no extra setup is
needed for pthreads.


Build & run
===========

.. code-block:: bash

   cmake --build build --config Release --target integration_c_10
   bin/Release/integration_c_10

Expected output::

   === Part A: Run on a worker thread ===
     compute() on worker thread returned: 4950
     PASS

   === Part B: Compile on a worker thread ===
     compute() compiled + run on worker thread returned: 4950
     PASS


.. seealso::

   Full source:
   :download:`10_threading.c <../../../../tutorials/integration/c/10_threading.c>`,
   :download:`10_threading.das <../../../../tutorials/integration/c/10_threading.das>`

   Previous tutorial: :ref:`tutorial_integration_c_aot`

   C++ equivalent: :ref:`tutorial_integration_cpp_threading`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
