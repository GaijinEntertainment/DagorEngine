.. _tutorial_integration_cpp_dynamic_scripts:

.. index::
   single: Tutorial; C++ Integration; Dynamic Scripts

========================================
 C++ Integration: Dynamic Scripts
========================================

This tutorial shows how to build a daslang program **from a string**
at runtime, compile it from a **virtual file** (no disk I/O), and
interact with its global variables directly through context memory
pointers.

The example implements an expression calculator: the host defines
variables and a math expression, compiles them into a tiny daslang
program, and then evaluates the expression at near-native speed by
poking variable values directly into context memory.

Topics covered:

* ``TextWriter`` — building daslang source code as a C++ string
* ``TextFileInfo`` + ``FsFileAccess::setFileInfo`` — registering a virtual file
* ``compileDaScript`` on a virtual file name
* ``ctx.findVariable`` / ``ctx.getVariable`` — locating globals in context memory
* Direct pointer writes for maximum-performance variable updates
* ``shared_ptr<Context>`` for long-lived context reuse


Prerequisites
=============

* Tutorial 11 completed (:ref:`tutorial_integration_cpp_context_variables`).
* Understanding of ``compileDaScript`` and ``Context`` lifecycle.


Building source from C++ strings
===================================

``TextWriter`` is daslang's stream builder.  We use it to construct a
valid daslang program with global variables and an eval function:

.. code-block:: cpp

   TextWriter ss;
   ss << "options gen2\n";
   ss << "require math\n\n";
   for (auto & v : vars) {
       ss << "var " << v.first << " = 0.0f\n";
   }
   ss << "\n[export]\n"
      << "def eval() : float {\n"
      << "    return " << expr << "\n"
      << "}\n";

The generated source is a complete daslang module — it ``require``\ s
``math`` for trigonometric functions and declares each variable as a
module-level global.


Compiling from a virtual file
================================

Instead of writing the source to disk, we register it as a virtual file
using ``TextFileInfo`` and ``setFileInfo``:

.. code-block:: cpp

   auto fAccess = make_smart<FsFileAccess>();
   auto fileInfo = make_unique<TextFileInfo>(
       text.c_str(), uint32_t(text.length()), false);
   fAccess->setFileInfo("expr.das", das::move(fileInfo));

   ModuleGroup dummyLibGroup;
   auto program = compileDaScript("expr.das", fAccess, tout, dummyLibGroup);

``compileDaScript`` treats ``"expr.das"`` as a real file, but the file
access layer resolves it to our in-memory buffer.  This is useful for
procedurally generated code, REPL implementations, and expression
evaluators.


Direct context memory access
================================

After simulation, we locate each variable in context memory and
cache its pointer for fast writes:

.. code-block:: cpp

   for (auto & v : vars) {
       auto idx = ctx->findVariable(v.first.c_str());
       if (idx != -1) {
           variables[v.first] = (float *)ctx->getVariable(idx);
       }
   }

To evaluate the expression with new values, we write directly into
context memory — no function calls, no marshalling:

.. code-block:: cpp

   float ExprCalc::compute(ExprVars & vars) {
       for (auto & v : vars) {
           auto it = variables.find(v.first);
           if (it != variables.end()) {
               *(it->second) = v.second;
           }
       }
       auto res = ctx->evalWithCatch(fni, nullptr);
       return cast<float>::to(res);
   }


Build & run
===========

.. code-block:: bash

   cmake --build build --config Release --target integration_cpp_18
   bin/Release/integration_cpp_18

Expected output::

   a=1 b=2 c=3  =>  a*b+c = 5
   All 60 evaluations correct
   sin(0)+cos(0) = 1
   sin(pi)+cos(pi) = -1


.. seealso::

   Full source:
   :download:`18_dynamic_scripts.cpp <../../../../tutorials/integration/cpp/18_dynamic_scripts.cpp>`

   Previous tutorial: :ref:`tutorial_integration_cpp_coroutines`

   Next tutorial: :ref:`tutorial_integration_cpp_class_adapters`

   Related: :ref:`tutorial_integration_cpp_context_variables`
