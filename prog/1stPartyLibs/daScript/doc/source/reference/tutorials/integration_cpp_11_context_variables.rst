.. _tutorial_integration_cpp_context_variables:

.. index::
   single: Tutorial; C++ Integration; Context Variables

==========================================
 C++ Integration: Context Variables
==========================================

This tutorial shows how to read and write daslang global variables
from C++ host code.  Topics covered:

* ``ctx.findVariable()`` — look up globals by name
* ``ctx.getVariable()`` — get a raw pointer to variable data
* Reading and writing scalar, string, and struct globals
* ``ctx.getTotalVariables()`` and ``ctx.getVariableInfo()`` — enumeration


Prerequisites
=============

* Tutorial 10 completed (:ref:`tutorial_integration_cpp_custom_modules`).
* Familiarity with ``ManagedStructureAnnotation``.


Global variable layout
========================

After ``program->simulate(ctx, tout)``,
global variables live in a contiguous memory buffer inside the
``Context``.  The ``Context`` provides methods to find variables by
name and access their raw data:

.. code-block:: cpp

   // Find by name → index (returns -1 if not found)
   int idx = ctx.findVariable("score");

   // Index → raw pointer to data
   void * ptr = ctx.getVariable(idx);

   // Cast and use
   int32_t value = *(int32_t *)ptr;    // read
   *(int32_t *)ptr = 9999;             // write — visible to daslang!


Reading scalar globals
========================

.. code-block:: cpp

   int idx_score = ctx.findVariable("score");
   if (idx_score >= 0) {
       int32_t * pScore = (int32_t *)ctx.getVariable(idx_score);
       printf("score = %d\n", *pScore);
   }

   int idx_name = ctx.findVariable("player_name");
   if (idx_name >= 0) {
       // String globals are stored as char*
       char ** pName = (char **)ctx.getVariable(idx_name);
       printf("player_name = %s\n", *pName);
   }


Reading struct globals
========================

If the script has a global of a handled type, the pointer points
directly to the C++ struct:

.. code-block:: cpp

   int idx = ctx.findVariable("config");
   if (idx >= 0) {
       GameConfig * p = (GameConfig *)ctx.getVariable(idx);
       printf("gravity = %.1f\n", p->gravity);
   }


Writing globals from C++
===========================

Any changes made through the raw pointer are immediately visible
to daslang code:

.. code-block:: cpp

   *(int32_t *)ctx.getVariable(idx_score) = 9999;

   // Call a daslang function — it will see score == 9999
   auto fn = ctx.findFunction("print_globals");
   ctx.evalWithCatch(fn, nullptr);


Enumerating all variables
===========================

``getTotalVariables()`` returns the count, ``getVariableInfo()``
returns a ``VarInfo *`` with name and size:

.. code-block:: cpp

   int total = ctx.getTotalVariables();
   for (int i = 0; i < total; i++) {
       auto * info = ctx.getVariableInfo(i);
       if (info) {
           printf("[%d] %s (size=%d)\n", i, info->name, info->size);
       }
   }


Initialization
================

``simulate()`` automatically calls ``runInitScript()`` internally,
executing global initializers and ``[init]`` functions.  No separate
initialization step is needed — globals have their initial values
immediately after simulation.


The daslang side
===================

.. code-block:: das

   options gen2
   require tutorial_11_cpp

   var score : int = 42
   var player_name : string = "Hero"
   var alive : bool = true
   var config = GameConfig(gravity = 9.8, speed = 5.0, max_enemies = 10)

   [export]
   def print_globals() {
       print("score = {score}\n")
       // C++ changes are visible here


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_11
   bin\Release\integration_cpp_11.exe

Expected output::

   === Reading globals from C++ ===
   score = 42
   player_name = Hero
   alive = true
   config.gravity     = 9.8
   config.speed       = 5.0
   config.max_enemies = 10

   === Writing globals from C++ ===
   C++ set score = 9999
   C++ set config.gravity = 20.0, max_enemies = 100

   === Calling daslang to verify ===
     score        = 9999
     player_name  = Hero
     alive        = true
     config.gravity     = 20
     config.speed       = 5
     config.max_enemies = 100

   === All global variables ===
     [0] score (size=4)
     [1] player_name (size=8)
     [2] alive (size=1)
     [3] config (size=12)


.. seealso::

   Full source:
   :download:`11_context_variables.cpp <../../../../tutorials/integration/cpp/11_context_variables.cpp>`,
   :download:`11_context_variables.das <../../../../tutorials/integration/cpp/11_context_variables.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_custom_modules`

   Next tutorial: :ref:`tutorial_integration_cpp_smart_pointers`
