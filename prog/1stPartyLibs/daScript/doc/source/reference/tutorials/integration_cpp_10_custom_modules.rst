.. _tutorial_integration_cpp_custom_modules:

.. index::
   single: Tutorial; C++ Integration; Custom Modules

====================================
 C++ Integration: Custom Modules
====================================

This tutorial shows how to organize C++ bindings into multiple modules
with dependencies between them.  Topics covered:

* Splitting types and functions across separate modules
* ``Module::require()`` — looking up modules by name
* ``addBuiltinDependency`` — declaring module dependencies
* ``addConstant`` — exporting compile-time constants
* ``initDependencies()`` — deferred initialization for robust ordering
* ``NEED_MODULE`` ordering in the host program


Prerequisites
=============

* Tutorial 09 completed (:ref:`tutorial_integration_cpp_operators_and_properties`).
* Familiarity with ``ManagedStructureAnnotation`` and ``addExtern``.


Why multiple modules?
=====================

So far, every tutorial has used a single module.  In real projects,
types and functions often belong to separate logical groups.  Splitting
them into modules gives scripts fine-grained ``require`` control and
keeps each module focused.

In this tutorial we create two modules:

* **math_types** — defines the ``Vec2`` type and mathematical constants
* **math_utils** — utility functions that *depend on* ``Vec2``


Module A: types and constants
=============================

The first module registers a type and several constants:

.. code-block:: cpp

   class Module_MathTypes : public Module {
   public:
       Module_MathTypes() : Module("math_types") {
           ModuleLibrary lib(this);
           lib.addBuiltInModule();

           addAnnotation(make_smart<Vec2Annotation>(lib));

           addExtern<DAS_BIND_FUN(make_vec2),
                     SimNode_ExtFuncCallAndCopyOrMove>(
               *this, lib, "make_vec2",
               SideEffects::none, "make_vec2")
                   ->args({"x", "y"});

           // Constants — appear as `let` in daslang
           addConstant(*this, "PI",      (float)M_PI);
           addConstant(*this, "TWO_PI",  (float)(2.0 * M_PI));
           addConstant(*this, "HALF_PI", (float)(M_PI / 2.0));
           addConstant(*this, "ORIGIN",  "origin_marker");  // string
       }
   };
   REGISTER_MODULE(Module_MathTypes);

``addConstant`` creates a module-level ``let`` constant.  It supports
numeric types (``int``, ``float``, ``double``, ``uint``, etc.) and
strings.


Module B: depends on Module A — ``initDependencies()``
========================================================

The second module uses ``Vec2`` from ``math_types`` in its function
signatures.  Instead of registering everything in the constructor, it
uses the **``initDependencies()`` pattern** — the production standard
for modules with cross-module dependencies.

Why not the constructor?
~~~~~~~~~~~~~~~~~~~~~~~~

Modules are constructed during static initialization (triggered by
``REGISTER_MODULE``).  At that point, the dependency module may not
exist yet — its constructor may not have run.  ``Module::require()``
could return ``nullptr``, and the dependency's types would not be
registered.

``initDependencies()`` is called later by the engine, after all modules
are constructed.  This guarantees that ``Module::require()`` can find
every loaded module.

The pattern
~~~~~~~~~~~

.. code-block:: cpp

   class Module_MathUtils : public Module {
       bool initialized = false;  // guard against double-init
   public:
       Module_MathUtils() : Module("math_utils") {
           // Empty — all work deferred to initDependencies().
       }

       bool initDependencies() override {
           if (initialized) return true;          // already done

           // 1. Look up the dependency by name
           auto mod = Module::require("math_types");
           if (!mod) return false;                // not loaded

           // 2. Ensure it is fully initialized
           if (!mod->initDependencies()) return false;

           // 3. Mark ourselves as initialized (before registration
           //    to prevent re-entry from circular dependencies)
           initialized = true;

           // 4. Set up library with dependency
           ModuleLibrary lib(this);
           lib.addBuiltInModule();
           addBuiltinDependency(lib, mod);

           // 5. Register functions — Vec2 is now known
           addExtern<DAS_BIND_FUN(vec2_length)>(
               *this, lib, "length", ...);
           addExtern<DAS_BIND_FUN(vec2_lerp), ...>(
               *this, lib, "lerp", ...);
           // ...

           return true;
       }
   };
   REGISTER_MODULE(Module_MathUtils);

Key elements:

+---------------------------------+---------------------------------------------+
| ``initialized`` guard           | Prevents double-initialization              |
+---------------------------------+---------------------------------------------+
| ``Module::require("name")``     | Looks up module by registered name          |
+---------------------------------+---------------------------------------------+
| ``mod->initDependencies()``     | Ensures dependency is fully registered      |
+---------------------------------+---------------------------------------------+
| ``addBuiltinDependency``        | Makes dependency's types visible + records  |
|                                 | the relationship for the compiler           |
+---------------------------------+---------------------------------------------+
| ``return true/false``           | Signals success or failure to the engine    |
+---------------------------------+---------------------------------------------+


Real-world examples
~~~~~~~~~~~~~~~~~~~

This pattern appears throughout the daslang ecosystem:

**dasAudio** (requires ``rtti``):

.. code-block:: cpp

   class Module_Audio : public Module {
       bool initialized = false;
   public:
       Module_Audio() : Module("audio") {}
       bool initDependencies() override {
           if (initialized) return true;
           if (!Module::require("rtti_core")) return false;
           initialized = true;
           ModuleLibrary lib(this);
           lib.addBuiltInModule();
           addBuiltinDependency(lib, Module::require("rtti_core"));
           // ... register types, functions ...
           return true;
       }
   };

**dasIMGUI_NODE_EDITOR** (requires ``imgui``):

.. code-block:: cpp

   bool Module_dasIMGUI_NODE_EDITOR::initDependencies() {
       if (initialized) return true;
       auto mod_imgui = Module::require("imgui");
       if (!mod_imgui) return false;
       if (!mod_imgui->initDependencies()) return false;
       initialized = true;
       lib.addModule(this);
       lib.addBuiltInModule();
       lib.addModule(mod_imgui);
       // ... register types, functions ...
       return true;
   }

The ``mod->initDependencies()`` call is especially important — it
chains initialization so that modules initialize in the correct order
regardless of how ``NEED_MODULE`` lines are arranged.


Constructor vs ``initDependencies()`` — when to use which
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------------------------+-----------------------------------------+
| **Constructor**            | Leaf modules with no custom-module deps |
|                            | (like ``math_types`` above)             |
+----------------------------+-----------------------------------------+
| **``initDependencies()``** | Any module that depends on another      |
|                            | custom module — use this by default     |
+----------------------------+-----------------------------------------+


Host program — ``NEED_MODULE``
================================

In ``main()``, both modules must be listed.  With ``initDependencies()``,
explicit ordering is no longer critical — the chained
``mod->initDependencies()`` calls handle it automatically:

.. code-block:: cpp

   int main(int, char * []) {
       NEED_ALL_DEFAULT_MODULES;
       NEED_MODULE(Module_MathTypes);
       NEED_MODULE(Module_MathUtils);
       Module::Initialize();
       // ... compile & run ...
       Module::Shutdown();
       return 0;
   }

``NEED_MODULE`` forces the linker to pull in the module's translation
unit.  ``Module::require("name")`` finds a module by its registered
name string, returning ``nullptr`` if not found.


Using both modules from daslang
==================================

Scripts ``require`` each module independently:

.. code-block:: das

   options gen2
   require math_types
   require math_utils

   [export]
   def test() {
       // Constants from math_types
       print("PI = {PI}\n")

       // Type from math_types
       let a = make_vec2(3.0, 4.0)

       // Functions from math_utils (operate on Vec2)
       print("length(a) = {length(a)}\n")

       let mid = lerp(a, make_vec2(1.0, 0.0), 0.5)
       print("mid = ({mid.x}, {mid.y})\n")


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_10
   bin\Release\integration_cpp_10.exe

Expected output::

   === Constants ===
   PI      = 3.1415927
   TWO_PI  = 6.2831855
   HALF_PI = 1.5707964
   ORIGIN  = origin_marker

   === Vec2 ===
   a = (3, 4)
   b = (1, 0)

   === Utility functions ===
   length(a)     = 5
   dot(a, b)     = 3
   normalize(a)  = (0.6, 0.8)

   === Operators ===
   a + b = (4, 4)
   a * 2 = (6, 8)

   === Lerp ===
   lerp(a, b, 0.5) = (2, 2)
   lerp(a, b, 0.0) = (3, 4)
   lerp(a, b, 1.0) = (1, 0)


.. seealso::

   Full source:
   :download:`10_custom_modules.cpp <../../../../tutorials/integration/cpp/10_custom_modules.cpp>`,
   :download:`10_custom_modules.das <../../../../tutorials/integration/cpp/10_custom_modules.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_operators_and_properties`

   Next tutorial: :ref:`tutorial_integration_cpp_context_variables`
