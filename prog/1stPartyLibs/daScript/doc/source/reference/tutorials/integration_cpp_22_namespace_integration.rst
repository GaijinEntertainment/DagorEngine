.. _tutorial_integration_cpp_namespace_integration:

.. index::
   single: Tutorial; C++ Integration; Namespace Integration

==========================================
 C++ Integration: Namespace Integration
==========================================

This tutorial shows how to initialize daslang modules when your code lives
inside a C++ namespace.

Topics covered:

* Why ``NEED_MODULE`` fails inside a namespace
* ``DECLARE_MODULE`` / ``PULL_MODULE`` — the namespace-safe alternative
* ``DECLARE_ALL_DEFAULT_MODULES`` / ``PULL_ALL_DEFAULT_MODULES``


Prerequisites
=============

* Tutorial 1 completed (:ref:`tutorial_integration_cpp_hello_world`) —
  basic compile → simulate → eval cycle.


The problem
===========

The ``NEED_MODULE`` macro expands to an ``extern`` declaration followed
by a call:

.. code-block:: cpp

   extern DAS_API das::Module * register_Module_BuiltIn();
   *das::ModuleKarma += unsigned(intptr_t(register_Module_BuiltIn()));

When this macro is used inside a C++ namespace, the ``extern``
declaration is placed in that namespace's scope.  The linker then looks
for ``MyApp::register_Module_BuiltIn()`` instead of the global
``::register_Module_BuiltIn()`` defined in the library:

.. code-block:: cpp

   namespace MyApp {
       void init() {
           NEED_ALL_DEFAULT_MODULES;   // FAILS: linker error!
           das::Module::Initialize();
       }
   }

The solution is to split the declaration and the call into two separate
macros.


``DECLARE_MODULE`` / ``PULL_MODULE``
====================================

``DECLARE_MODULE(ClassName)`` — forward-declares the global-scope
``register_*`` function.  Must be placed at file or global scope (outside
any namespace).

``PULL_MODULE(ClassName)`` — performs the registration call using the
``::`` prefix to explicitly reference the global-scope function.  Safe
inside any namespace, class, or function body.

Convenience wrappers ``DECLARE_ALL_DEFAULT_MODULES`` and
``PULL_ALL_DEFAULT_MODULES`` cover every built-in module.


The tutorial
============

The tutorial runs the same ``01_hello_world.das`` script as Tutorial 1,
but all daslang calls happen inside ``namespace MyApp``.

.. literalinclude:: ../../../../tutorials/integration/cpp/22_namespace_integration.cpp
   :language: cpp
   :caption: tutorials/integration/cpp/22_namespace_integration.cpp


How it works
============

1. ``DECLARE_ALL_DEFAULT_MODULES`` at file scope forward-declares every
   default module's ``register_*`` function as a global-scope symbol.

2. Inside ``MyApp::initialize()``, ``PULL_ALL_DEFAULT_MODULES`` calls
   each ``::register_Module_*()`` function — the ``::`` prefix bypasses
   the enclosing namespace.

3. ``Module::Initialize()`` and the rest of the daslang API continue to
   work normally inside the namespace — only module registration has the
   namespace restriction.


Custom modules
==============

For custom modules, use ``DECLARE_MODULE`` at file scope alongside the
convenience macros:

.. code-block:: cpp

   DECLARE_ALL_DEFAULT_MODULES;
   DECLARE_MODULE(Module_MyMod);

   namespace MyApp {
       void init() {
           PULL_ALL_DEFAULT_MODULES;
           PULL_MODULE(Module_MyMod);
           das::Module::Initialize();
       }
   }


External (plugin) modules
=========================

CMake generates three ``.inc`` files for every module registered with
``ADD_MODULE_CPP``:

``external_need.inc``
   Contains ``NEED_MODULE(...)`` — the traditional all-in-one macro
   (works only at global scope).

``external_declare.inc``
   Contains ``DECLARE_MODULE(...)`` — include at file scope.

``external_pull.inc``
   Contains ``PULL_MODULE(...)`` — include inside any namespace or
   function body.

For namespace-safe code, replace:

.. code-block:: cpp

   // old (global scope only)
   #include "modules/external_need.inc"

with:

.. code-block:: cpp

   // file scope
   #include "modules/external_declare.inc"

   namespace MyApp {
       void init() {
           // inside namespace
           #include "modules/external_pull.inc"
       }
   }


Build and run
=============

.. code-block:: bash

   cmake --build build --config Release --target integration_cpp_22

.. code-block::

   $ bin/Release/integration_cpp_22
   Hello, World!
