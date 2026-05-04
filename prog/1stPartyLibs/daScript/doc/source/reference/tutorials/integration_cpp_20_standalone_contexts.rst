.. _tutorial_integration_cpp_standalone_contexts:

.. index::
   single: Tutorial; C++ Integration; Standalone Contexts

==========================================
 C++ Integration: Standalone Contexts
==========================================

This tutorial shows how to use a **pre-compiled standalone context** —
a daslang program baked entirely into C++ with zero runtime compilation
overhead.

Normal daslang embedding compiles ``.das`` files at startup:
``compileDaScript`` → ``simulate`` → ``evalWithCatch``.  A standalone
context eliminates the compile and simulate steps entirely.  The AOT
tool generates a ``.das.h`` and ``.das.cpp`` that contain all function
code, type information, and initialization logic as C++ code.

Topics covered:

* ``DAS_AOT_CTX`` CMake macro — generating standalone context artifacts
* The generated ``Standalone`` class (extends ``Context``)
* Direct method calls — ``test()`` is a C++ function, no lookup needed
* Zero startup cost — no parsing, no type checking, no simulation


Prerequisites
=============

* Tutorial 13 completed (:ref:`tutorial_integration_cpp_aot`).
* Understanding of AOT compilation and ``Context``.


When to use standalone contexts
==================================

Standalone contexts are ideal when:

* **Startup time matters** — embedded systems, command-line tools, game
  loading screens where even milliseconds count.
* **No file system access** — the script is burned into the binary.
* **Maximum performance** — all functions are AOT-compiled with no
  fallback to the interpreter.

The trade-off is that the script is fixed at build time.


The daslang script
===================

A simple script that computes a sum:

.. code-block:: das

   options gen2

   [export]
   def test {
       var total = 0
       for (i in range(10)) {
           total += i
       }
       print("Sum of 0..9 = {total}\n")
   }


Build pipeline
================

The ``DAS_AOT_CTX`` CMake macro drives the generation:

.. code-block:: cmake

   SET(STANDALONE_CTX_GENERATED_SRC)
   DAS_AOT_CTX("tutorials/integration/cpp/standalone_context.das"
               STANDALONE_CTX_GENERATED_SRC
               integration_cpp_20_dasAotStubStandalone daslang)

   add_executable(integration_cpp_20
       20_standalone_context.cpp
       standalone_context.das
       ${STANDALONE_CTX_GENERATED_SRC})
   TARGET_LINK_LIBRARIES(integration_cpp_20 libDaScript)

This runs ``daslang`` with the ``-ctx`` flag, which invokes
``daslib/aot_standalone.das`` to generate:

* ``standalone_context.das.h`` — declares the ``Standalone`` class
* ``standalone_context.das.cpp`` — implements all AOT functions and
  context initialization

These files are placed in ``_standalone_ctx_generated/``.


The C++ host
==============

The host code is remarkably simple — no module initialization,
no compilation, no simulation:

.. code-block:: cpp

   #include "daScript/daScript.h"
   #include "_standalone_ctx_generated/standalone_context.das.h"

   using namespace das;

   int main(int, char * []) {
       TextPrinter tout;
       tout << "Creating standalone context...\n";

       auto ctx = standalone_context::Standalone();

       tout << "Calling test():\n";
       ctx.test();

       tout << "Done.\n";
       return 0;
   }

Key points:

* **No** ``NEED_ALL_DEFAULT_MODULES`` or ``Module::Initialize`` —
  the standalone context is entirely self-contained.
* ``standalone_context::Standalone()`` — the constructor sets up all
  functions, globals, and type info from pre-generated AOT data.
* ``ctx.test()`` — a direct C++ method call, not ``findFunction``
  followed by ``evalWithCatch``.


Generated namespace
=====================

The generated namespace is derived from the script file name:

* ``standalone_context.das`` → namespace ``das::standalone_context``
* Class name is always ``Standalone``
* Each ``[export]`` function becomes a method: ``ctx.test()``,
  ``ctx.compute(args...)``, etc.


Build & run
===========

.. code-block:: bash

   cmake --build build --config Release --target integration_cpp_20
   bin/Release/integration_cpp_20

Expected output::

   Creating standalone context (no runtime compilation)...
   Calling test():
   Sum of 0..9 = 45
   Done.


.. seealso::

   Full source:
   :download:`20_standalone_context.cpp <../../../../tutorials/integration/cpp/20_standalone_context.cpp>`,
   :download:`standalone_context.das <../../../../tutorials/integration/cpp/standalone_context.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_class_adapters`

   Next tutorial: :ref:`tutorial_integration_cpp_threading`

   Related: :ref:`tutorial_integration_cpp_aot`
