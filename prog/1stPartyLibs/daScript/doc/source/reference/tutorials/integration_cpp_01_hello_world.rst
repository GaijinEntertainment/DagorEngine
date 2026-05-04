.. _tutorial_integration_cpp_hello_world:

.. index::
   single: Tutorial; C++ Integration; Hello World

====================================
 C++ Integration: Hello World
====================================

This tutorial shows how to embed daslang in a C++ application using the
native ``daScript.h`` API.  By the end you will have a standalone program that
compiles a ``.das`` script, finds a function, calls it, and prints
``Hello from daslang!``.

.. note::

   If you have already read the C integration series (starting at
   :ref:`tutorial_integration_c_hello_world`), you will notice that the
   C++ API is considerably more concise: there is no manual reference
   counting, strings are ``std::string``, and smart pointers manage
   lifetimes automatically.


Prerequisites
=============

* daslang built from source (``cmake --build build --config Release``).
  The build produces ``libDaScript`` which the tutorial links against.
* The ``daScript.h`` header — located in ``include/daScript/daScript.h``.
  This is the main C++ header that pulls in the full API (type system,
  compilation, contexts, module registration, etc.).


The daslang file
=================

Create a minimal script with a single exported function:

.. code-block:: das

   options gen2

   [export]
   def test() {
       print("Hello from daslang!\n")
   }

The ``[export]`` annotation makes the function visible to the host application
so that ``Context::findFunction`` can locate it by name.


The C++ program
===============

The program follows the same lifecycle as the C version, but each step uses
the native C++ API directly.


Step 1 — Initialize
--------------------

.. code-block:: cpp

   #include "daScript/daScript.h"

   using namespace das;

   int main(int, char * []) {
       NEED_ALL_DEFAULT_MODULES;
       Module::Initialize();

``NEED_ALL_DEFAULT_MODULES`` is a macro that ensures all built-in modules
(math, strings, etc.) are linked into the executable.  ``Module::Initialize``
activates them.  Both must be called **once** before any compilation.


Step 2 — Set up compilation infrastructure
-------------------------------------------

.. code-block:: cpp

   TextPrinter tout;
   ModuleGroup dummyLibGroup;
   auto fAccess = make_smart<FsFileAccess>();

======================  =====================================================
Object                  Purpose
======================  =====================================================
``TextPrinter``         Sends compiler/runtime messages to **stdout**.
                        For an in-memory buffer, use ``TextWriter`` instead.
``ModuleGroup``         Holds modules available during compilation.
``FsFileAccess``        Provides disk-based file I/O to the compiler.
                        Wrapped in a ``smart_ptr`` — freed automatically.
======================  =====================================================


Step 3 — Compile the script
----------------------------

.. code-block:: cpp

   auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                  fAccess, tout, dummyLibGroup);
   if (program->failed()) {
       for (auto & err : program->errors) {
           tout << reportError(err.at, err.what, err.extra,
                               err.fixme, err.cerr);
       }
       return;
   }

``compileDaScript`` reads the ``.das`` file, parses, type-checks, and returns
a ``ProgramPtr`` (a smart pointer to ``Program``).  ``getDasRoot()`` returns
the project root directory, so paths are always resolved correctly regardless
of the working directory.

Errors are stored in ``program->errors`` and can be formatted with
``reportError``.


Step 4 — Create a context and simulate
----------------------------------------

.. code-block:: cpp

   Context ctx(program->getContextStackSize());
   if (!program->simulate(ctx, tout)) {
       // handle errors ...
   }

A **Context** is the runtime environment — it owns the execution stack, global
variables, and heap.  Unlike the C API, the context is a stack-allocated
object here; there is no ``release`` call.

**Simulation** resolves function pointers, initializes globals, and prepares
everything for execution.  Always check for errors after ``simulate``.


Step 5 — Find and verify the function
---------------------------------------

.. code-block:: cpp

   auto fnTest = ctx.findFunction("test");
   if (!fnTest) {
       tout << "function 'test' not found\n";
       return;
   }
   if (!verifyCall<void>(fnTest->debugInfo, dummyLibGroup)) {
       tout << "wrong signature\n";
       return;
   }

``findFunction`` returns a ``SimFunction *`` — valid for the lifetime of the
context.

``verifyCall<ReturnType, ArgTypes...>`` is a compile-time-safe signature
check.  It is slow (it walks debug info), so do it **once** during setup —
not on every call in a hot loop.


Step 6 — Call the function
---------------------------

.. code-block:: cpp

   ctx.evalWithCatch(fnTest, nullptr);
   if (auto ex = ctx.getException()) {
       tout << "exception: " << ex << "\n";
   }

``evalWithCatch`` runs the function inside a C++ try/catch so that a daslang
``panic()`` does not crash the host.  The second argument is an array of
``vec4f`` arguments — ``nullptr`` here because ``test`` takes none.


Step 7 — Shut down
--------------------

.. code-block:: cpp

       Module::Shutdown();
       return 0;
   }

``Module::Shutdown`` frees all global state.  No daslang API calls are
allowed after it.  Note that unlike the C API, there is no need to manually
release the program, module group, or text printer — these are either
stack-allocated or reference-counted smart pointers.


C++ vs. C API comparison
=========================

================================================================  ============================================
C API (``daScriptC.h``)                                           C++ API (``daScript.h``)
================================================================  ============================================
``das_initialize()``                                              ``NEED_ALL_DEFAULT_MODULES; Module::Initialize()``
``das_text_make_printer()`` / ``das_text_release()``              ``TextPrinter tout;`` (stack)
``das_fileaccess_make_default()`` / ``das_fileaccess_release()``  ``make_smart<FsFileAccess>()`` (ref-counted)
``das_program_compile()`` / ``das_program_release()``             ``compileDaScript()`` (returns ``ProgramPtr``)
``das_context_make()`` / ``das_context_release()``                ``Context ctx(stackSize);`` (stack)
``das_context_find_function()``                                   ``ctx.findFunction()``
``das_context_eval_with_catch()``                                 ``ctx.evalWithCatch()``
``das_shutdown()``                                                ``Module::Shutdown()``
================================================================  ============================================


Key concepts
============

Smart pointers
   The C++ API uses ``smart_ptr<T>`` (daslang's own reference-counted
   smart pointer) and ``make_smart<T>()`` for heap-allocated objects.
   ``ProgramPtr`` returned by ``compileDaScript`` is one example.

Stack-allocated objects
   ``TextPrinter``, ``ModuleGroup``, and ``Context`` can all live on the
   stack.  Their destructors handle cleanup automatically.

``getDasRoot``
   Returns the root of the daslang installation as a ``string``.  Use it
   to build paths to scripts so they resolve regardless of working directory.

``verifyCall``
   Template function that validates a ``SimFunction``'s signature against
   expected C++ types.  Use it as a development-time safety net.


Building and running
====================

The tutorial is built automatically by CMake as part of the daslang project::

   cmake --build build --config Release --target integration_cpp_01

Run from the project root so that the script path resolves correctly::

   bin\Release\integration_cpp_01.exe

Expected output::

   Hello from daslang!

You can also build all C++ integration tutorials from the installed SDK
— see :ref:`tutorial_building_from_sdk`.


.. seealso::

   Full source:
   :download:`01_hello_world.cpp <../../../../tutorials/integration/cpp/01_hello_world.cpp>`,
   :download:`01_hello_world.das <../../../../tutorials/integration/cpp/01_hello_world.das>`

   Building from the SDK: :ref:`tutorial_building_from_sdk`

   Next tutorial: :ref:`tutorial_integration_cpp_calling_functions`

   C API equivalent: :ref:`tutorial_integration_c_hello_world`
