.. _tutorial_integration_cpp_threading:

.. index::
   single: Tutorial; C++ Integration; Threading

==============================
 C++ Integration: Threading
==============================

This tutorial demonstrates how to use daslang contexts and compilation
pipelines across multiple threads in a C++ host application.

Topics covered:

* **Part A** — Running a compiled context on a worker thread
* **Part B** — Compiling a script from scratch on a worker thread
* ``daScriptEnvironment::getBound()`` / ``setBound()`` — thread-local
  environment binding
* ``ReuseCacheGuard`` — thread-local free-list cache management
* ``ContextCategory::thread_clone`` — context clone category for threads
* ``shared_ptr<Context>`` with ``sharedPtrContext = true``
* ``CodeOfPolicies::threadlock_context`` — context mutex for threads
* Per-thread ``Module::Initialize()`` / ``Module::Shutdown()``


Prerequisites
=============

* Tutorial 1 completed (:ref:`tutorial_integration_cpp_hello_world`) —
  basic compile → simulate → eval cycle.
* Understanding of ``Context`` and ``daScriptEnvironment`` from
  :ref:`tutorial_integration_cpp_standalone_contexts`.


Why threading matters
=====================

daslang uses **thread-local storage** (TLS) for its environment object
(``daScriptEnvironment``).  This object holds the module registry,
compilation state, and various global flags.  Every thread that touches
daslang API must have a valid environment bound.

There are two common patterns:

1. **Share the environment** — the worker thread binds the same
   environment object that the main thread uses.  This works when the
   worker only *executes* (not compiles) and the main thread is idle.
2. **Independent environment** — the worker thread creates its own
   module registry and compilation pipeline from scratch.  This is fully
   isolated and safe for concurrent compilation.


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
run it on a ``std::thread``.

.. code-block:: cpp

   // 1. Compile & simulate on main thread (standard boilerplate).
   CodeOfPolicies policies;
   policies.threadlock_context = true;  // context will run on another thread
   auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                  fAccess, tout, dummyLibGroup, policies);
   Context ctx(program->getContextStackSize());
   program->simulate(ctx, tout);
   auto fnCompute = ctx.findFunction("compute");

   // 2. Clone the context for the worker thread.
   shared_ptr<Context> threadCtx;
   threadCtx.reset(new Context(ctx, uint32_t(ContextCategory::thread_clone)));
   threadCtx->sharedPtrContext = true;

   auto fnComputeClone = threadCtx->findFunction("compute");

   // 3. Capture the current environment.
   auto bound = daScriptEnvironment::getBound();

   // 4. Launch the worker thread.
   int32_t result = 0;
   std::thread worker([&result, threadCtx, fnComputeClone, bound]() mutable {
       daScriptEnvironment::setBound(bound);
       vec4f res = threadCtx->evalWithCatch(fnComputeClone, nullptr);
       result = cast<int32_t>::to(res);
   });
   worker.join();

Key points:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Concept
     - Explanation
   * - ``daScriptEnvironment::getBound()``
     - Returns a pointer to the current thread's environment (TLS).
       Must be captured **before** launching the worker.
   * - ``daScriptEnvironment::setBound(env)``
     - Binds the environment on the worker thread.  Without this,
       any daslang API call will crash.
   * - ``ContextCategory::thread_clone``
     - Marks the context as a thread-owned clone.  The runtime can use
       this flag for diagnostics and thread-safety checks.
   * - ``shared_ptr<Context>``
     - Ensures the clone's lifetime extends until the worker is done.
       Set ``sharedPtrContext = true`` so the delete path is correct.
   * - ``threadlock_context``
     - Compile-time policy that gives the context a mutex.  Required
       when the context (or its clone) will be accessed from a
       non-main thread.
   * - ``findFunction`` on clone
     - Each context has its own function table.  Always look up
       functions on the context that will execute them.


Part B — Compile on a worker thread
====================================

Create a fully independent daslang environment on a new thread.
This is the pattern used in ``test_threads.cpp`` for concurrent
compilation benchmarks.

.. code-block:: cpp

   std::thread worker([&result]() {
       // 1. Thread-local free-list cache.
       ReuseCacheGuard guard;

       // 2. Register module factories.
       NEED_ALL_DEFAULT_MODULES;

       // 3. Initialize modules — creates a fresh environment in TLS.
       Module::Initialize();

       // 4. Standard compile → simulate → eval cycle.
       TextPrinter tout;
       ModuleGroup dummyLibGroup;
       CodeOfPolicies policies;
       policies.threadlock_context = true;  // context mutex for thread safety
       auto fAccess = make_smart<FsFileAccess>();
       auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                      fAccess, tout, dummyLibGroup, policies);
       Context ctx(program->getContextStackSize());
       program->simulate(ctx, tout);
       auto fn = ctx.findFunction("compute");
       vec4f res = ctx.evalWithCatch(fn, nullptr);
       result = cast<int32_t>::to(res);

       program.reset();

       // 5. Shut down this thread's modules.
       Module::Shutdown();
   });
   worker.join();

Key points:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Concept
     - Explanation
   * - ``ReuseCacheGuard``
     - Initializes and tears down per-thread free-list caches.
       Must be created **first** on any thread that uses daslang.
   * - ``NEED_ALL_DEFAULT_MODULES``
     - Registers module factory functions (not instances).  This
       is a macro that must run before ``Module::Initialize()``.
   * - ``Module::Initialize()``
     - Creates a new ``daScriptEnvironment`` in TLS and
       instantiates all registered modules.
   * - ``Module::Shutdown()``
     - Destroys this thread's modules and environment.  Must be
       called before the thread exits.
   * - ``threadlock_context``
     - Same policy as Part A — ensures the context has a mutex
       even when compiled on the worker thread.
   * - ``program.reset()``
     - Release the program before ``Module::Shutdown()`` to avoid
       dangling references to destroyed modules.


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


Build & run
===========

.. code-block:: bash

   cmake --build build --config Release --target integration_cpp_21
   bin/Release/integration_cpp_21

Expected output::

   === Part A: Run on a worker thread ===
     compute() on worker thread returned: 4950
     PASS

   === Part B: Compile on a worker thread ===
     compute() compiled + run on worker thread returned: 4950
     PASS


.. seealso::

   Full source:
   :download:`21_threading.cpp <../../../../tutorials/integration/cpp/21_threading.cpp>`,
   :download:`21_threading.das <../../../../tutorials/integration/cpp/21_threading.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_standalone_contexts`

   Related: :ref:`tutorial_integration_cpp_hello_world` (basic compile/simulate/eval)
