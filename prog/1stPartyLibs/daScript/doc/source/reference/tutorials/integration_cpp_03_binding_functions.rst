.. _tutorial_integration_cpp_binding_functions:

.. index::
   single: Tutorial; C++ Integration; Binding Functions

============================================
 C++ Integration: Binding Functions
============================================

This tutorial shows how to expose C++ functions to daslang scripts by
creating a **custom module**.  Topics covered:

* ``addExtern`` with ``DAS_BIND_FUN`` — binding free C++ functions
* ``SideEffects`` flags — telling the optimizer what a function does
* ``addConstant`` — exposing compile-time constants
* Context-aware functions — receiving ``Context *`` automatically


Prerequisites
=============

* Tutorial 02 completed (:ref:`tutorial_integration_cpp_calling_functions`).
* Understanding of ``cast<T>`` and the daslang calling convention.


Creating a module
=================

A module is a class derived from ``Module``.  Its constructor registers
types, functions, and constants that scripts can use via ``require``:

.. code-block:: cpp

   class Module_Tutorial03 : public Module {
   public:
       Module_Tutorial03() : Module("tutorial_03_cpp") {
           ModuleLibrary lib(this);
           lib.addBuiltInModule();
           // ... register functions and constants here ...
       }
   };

   REGISTER_MODULE(Module_Tutorial03);

``REGISTER_MODULE`` makes the module available via ``NEED_MODULE`` in the
host program.


Binding constants
=================

``addConstant`` exposes a C++ value as a compile-time constant:

.. code-block:: cpp

   addConstant(*this, "PI",    3.14159265358979323846f);
   addConstant(*this, "SQRT2", sqrtf(2.0f));

In the script:

.. code-block:: das

   print("PI = {PI}\n")       // 3.1415927
   print("SQRT2 = {SQRT2}\n") // 1.4142135


Binding functions with ``addExtern``
=====================================

``addExtern`` is the primary way to expose a C++ function to daslang.
The ``DAS_BIND_FUN`` macro generates the template machinery needed for
automatic argument marshalling:

.. code-block:: cpp

   float xmadd(float a, float b, float c, float d) {
       return a * b + c * d;
   }

   addExtern<DAS_BIND_FUN(xmadd)>(*this, lib, "xmadd",
       SideEffects::none, "xmadd");

Parameters:

1. ``DAS_BIND_FUN(xmadd)`` — the C++ function to bind
2. ``*this`` — the module being populated
3. ``lib`` — the module library (for type resolution)
4. ``"xmadd"`` — the name visible in daslang
5. ``SideEffects::none`` — optimizer hint (see below)
6. ``"xmadd"`` — C++ function name for AOT (used in generated code)


``SideEffects`` flags
=====================

The ``SideEffects`` enum tells the optimizer what observable effects a
function has.  This controls whether calls can be eliminated, reordered,
or folded:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Flag
     - Meaning
   * - ``none``
     - Pure function — no side effects.  Safe to eliminate if result is unused.
   * - ``modifyExternal``
     - Modifies external state (stdout, files, hardware, etc.)
   * - ``modifyArgument``
     - Modifies one or more of its arguments (passed by reference).
   * - ``accessGlobal``
     - Reads global/shared mutable state.
   * - ``invoke``
     - Calls a daslang function or lambda.
   * - ``worstDefault``
     - Combines ``modifyArgument | modifyExternal``.  Use when unsure.

Example — a function that prints to stdout needs ``modifyExternal``:

.. code-block:: cpp

   void greet(const char * name) {
       printf("Hello from C++, %s!\n", name);
   }

   addExtern<DAS_BIND_FUN(greet)>(*this, lib, "greet",
       SideEffects::modifyExternal, "greet");


Functions that modify arguments
================================

If a function takes a reference and modifies it, use
``SideEffects::modifyArgument``:

.. code-block:: cpp

   void double_it(int32_t & value) {
       value *= 2;
   }

   addExtern<DAS_BIND_FUN(double_it)>(*this, lib, "double_it",
       SideEffects::modifyArgument, "double_it");

In the script:

.. code-block:: das

   var val = 21
   double_it(val)
   print("{val}\n")    // 42


Context-aware functions
========================

If a C++ function takes ``Context *`` as its first parameter (or
``LineInfoArg *`` as its last), daslang injects them automatically — the
script caller does **not** see these parameters:

.. code-block:: cpp

   void print_stack_info(Context * ctx) {
       printf("Stack size: %d bytes\n", ctx->stack.size());
   }

   addExtern<DAS_BIND_FUN(print_stack_info)>(*this, lib, "print_stack_info",
       SideEffects::modifyExternal, "print_stack_info");

In the script the function takes zero arguments:

.. code-block:: das

   print_stack_info()   // prints "Stack size: 16384 bytes"


Activating the module in the host
==================================

The host program must request the module before ``Module::Initialize()``:

.. code-block:: cpp

   int main(int, char * []) {
       NEED_ALL_DEFAULT_MODULES;
       NEED_MODULE(Module_Tutorial03);
       Module::Initialize();
       // ... compile and run scripts ...
       Module::Shutdown();
       return 0;
   }

The script uses ``require`` to access the module:

.. code-block:: das

   require tutorial_03_cpp


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_03
   bin\Release\integration_cpp_03.exe

Expected output::

   PI    = 3.1415927
   SQRT2 = 1.4142135
   xmadd(SQRT2, SQRT2, 1.0, 1.0) = 3
   factorial(10) = 3628800
   Hello from C++, daslang!
   double_it(21) = 42
   Context stack size: 16384 bytes


.. seealso::

   Full source:
   :download:`03_binding_functions.cpp <../../../../tutorials/integration/cpp/03_binding_functions.cpp>`,
   :download:`03_binding_functions.das <../../../../tutorials/integration/cpp/03_binding_functions.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_calling_functions`

   Next tutorial: :ref:`tutorial_integration_cpp_binding_types`

   C API equivalent: :ref:`tutorial_integration_c_binding_types`
   (the C tutorials combine type and function binding in a single tutorial)
