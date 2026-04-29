.. _embedding_vm:

.. index::
   single: Embedding; Quick Start
   single: Embedding; Virtual Machine

=======================
 Quick Start
=======================

This page covers the minimum needed to run a daslang program from a C++
host application.  For a complete step-by-step tutorial, see
:ref:`tutorial_integration_cpp_hello_world`.


Virtual machine overview
========================

daslang uses a **tree-based interpreter** — the compiled program is a tree
of ``SimNode`` objects, each representing one operation (add, load, call,
etc.).  Interpretation walks the tree, calling each node's ``eval`` method.

Key properties of this design:

* **Statically typed, 128-bit words** — every value fits in ``vec4f`` (an
  SSE register), so operations are branchless type dispatches at compile
  time, not runtime.
* **Zero-copy C++ interop** — the in-memory representation matches C++
  ABI, so calling between daslang and C++ has no marshalling overhead.
* **Seamless AOT** — the same ``SimNode`` tree can be walked by the
  interpreter *or* compiled ahead-of-time into C++, with identical
  semantics (:ref:`aot`).
* **Value types vs reference types** — types that fit in 128 bits *may*
  be value types (passed in registers).  Whether a type is a reference
  type depends on its nature (e.g. handled types are always reference
  types regardless of size), not purely on size.

Execution context
-----------------

A ``Context`` is the runtime state for one daslang program: a call stack,
two heap allocators (string heap + heap), and a global variable segment.

* **One context per program** — globals, functions, and type info are
  per-context.
* **Multiple contexts per thread** — you can have multiple contexts in
  one thread, but only one executes at a time.  Contexts can make direct
  cross-context calls within a thread via ``pinvoke``.
  Contexts are not thread-safe — do not share a context across threads.
* **Cheap reset** — both heaps use bump allocation.  Resetting a
  stateless script is nearly free (pointer reset, no deallocation).
* **Garbage collection** — stop-the-world, triggered manually by calling
  ``context.collectHeap()`` or equivalent from the host.  There is
  currently no automatic GC triggered by heap pressure.


Minimal host program
====================

Every C++ host follows the same pattern:

.. code-block:: cpp

   #include "daScript/daScript.h"
   using namespace das;

   void run_script() {
       TextPrinter tout;                         // output sink
       ModuleGroup dummyLibGroup;                 // module group
       auto fAccess = make_smart<FsFileAccess>(); // file system access

       // 1. Compile the script
       auto program = compileDaScript(
           getDasRoot() + "/path/to/script.das",
           fAccess, tout, dummyLibGroup);
       if (program->failed()) {
           for (auto & err : program->errors) {
               tout << reportError(err.at, err.what, err.extra,
                                   err.fixme, err.cerr);
           }
           return;
       }

       // 2. Simulate (link + initialize)
       Context ctx(program->getContextStackSize());
       if (!program->simulate(ctx, tout)) {
           for (auto & err : program->errors) {
               tout << reportError(err.at, err.what, err.extra,
                                   err.fixme, err.cerr);
           }
           return;
       }

       // 3. Find and call a function
       auto fn = ctx.findFunction("main");
       if (fn) {
           ctx.evalWithCatch(fn, nullptr);
           if (auto ex = ctx.getException()) {
               tout << "Exception: " << ex << "\n";
           }
       }
   }

   int main(int, char * []) {
       NEED_ALL_DEFAULT_MODULES;     // register built-in modules
       Module::Initialize();         // initialize the module system
       run_script();
       Module::Shutdown();           // clean up
       return 0;
   }

The three-step flow is always the same:

1. **Compile** — ``compileDaScript`` parses and type-checks the ``.das``
   file, producing a ``ProgramPtr``.
2. **Simulate** — ``program->simulate(ctx, tout)`` generates the
   ``SimNode`` tree and initializes globals.
3. **Evaluate** — ``ctx.evalWithCatch(fn, nullptr)`` runs a function.


Key types
=========

``TextPrinter``
   Output sink that writes to stdout.  Any ``TextWriter`` subclass works.

``FsFileAccess``
   File system access for the compiler.  ``setFileInfo`` can register
   virtual files for compiling from strings
   (:ref:`tutorial_integration_cpp_dynamic_scripts`).

``ModuleGroup``
   Groups modules for shared compilation state.  For simple hosts, an
   empty ``dummyLibGroup`` suffices.

``Context``
   Runtime state — stack, heaps, globals.  Created with the stack size
   reported by the compiled program.

``SimFunction``
   Pointer to a compiled function.  Retrieved via
   ``ctx.findFunction("name")``.


Calling daslang functions with arguments
==========================================

To pass arguments and receive return values, use ``das_invoke_function``:

.. code-block:: cpp

   // For: def add(a, b : int) : int
   auto fn = ctx.findFunction("add");
   int32_t result = das_invoke_function<int32_t>::invoke(
       &ctx, nullptr, fn, 10, 20);

This handles argument marshalling automatically and is the preferred
approach.  See :ref:`tutorial_integration_cpp_calling_functions` for
a complete example.


Next steps
==========

* **Bind C++ functions** → :ref:`tutorial_integration_cpp_binding_functions`
* **Bind C++ types** → :ref:`tutorial_integration_cpp_binding_types`
* **Create custom modules** → :ref:`tutorial_integration_cpp_custom_modules`
* **External modules** → :ref:`embedding_external_modules`
* **Full API reference** → :ref:`embedding_modules`
