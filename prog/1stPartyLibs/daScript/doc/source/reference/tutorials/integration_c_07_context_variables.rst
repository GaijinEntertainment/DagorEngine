.. _tutorial_integration_c_context_variables:

.. index::
   single: Tutorial; C Integration; Context Variables
   single: Tutorial; C Integration; Global Variables
   single: Tutorial; C Integration; das_context_find_variable

=============================================
 C Integration: Context Variables
=============================================

This tutorial demonstrates how to **read, write, and enumerate** daslang
global variables from a C host using the ``daScriptC.h`` API.

After a program is compiled and simulated, its global variables live in
the context.  The C API provides five functions to access them:

=========================================  =========================================
Function                                   Purpose
=========================================  =========================================
``das_context_find_variable(ctx, name)``   Look up a global by name; returns an index
``das_context_get_variable(ctx, idx)``     Get a raw ``void*`` to the variable storage
``das_context_get_total_variables(ctx)``   Total number of globals
``das_context_get_variable_name(ctx, i)``  Name of the i-th global
``das_context_get_variable_size(ctx, i)``  Size (in bytes) of the i-th global
=========================================  =========================================


The daslang script
===================

The companion script declares four scalar globals:

.. code-block:: das

   options gen2

   var score : int = 42
   var speed : float = 3.14
   var player_name : string = "Hero"
   var alive : bool = true

   [export]
   def print_globals() {
       print("  score       = {score}\n")
       print("  speed       = {speed}\n")
       print("  player_name = {player_name}\n")
       print("  alive       = {alive}\n")
   }


Reading globals
===============

After simulation, look up a global by name and cast the returned pointer
to the appropriate C type:

.. code-block:: c

   int idx = das_context_find_variable(ctx, "score");
   if (idx >= 0) {
       int * p = (int *)das_context_get_variable(ctx, idx);
       printf("score = %d\n", *p);
   }

String globals are stored as ``char*`` internally:

.. code-block:: c

   int idx = das_context_find_variable(ctx, "player_name");
   if (idx >= 0) {
       char ** p = (char **)das_context_get_variable(ctx, idx);
       printf("player_name = %s\n", *p);
   }

``bool`` is stored as a single byte (not ``int``):

.. code-block:: c

   char * p = (char *)das_context_get_variable(ctx, idx_alive);
   printf("alive = %s\n", *p ? "true" : "false");


Writing globals
===============

The pointer returned by ``das_context_get_variable`` is **read-write**.
Modifications are immediately visible to daslang functions:

.. code-block:: c

   int * p = (int *)das_context_get_variable(ctx, idx_score);
   *p = 9999;    // next call to a daslang function sees score == 9999


Enumerating all globals
=======================

.. code-block:: c

   int total = das_context_get_total_variables(ctx);
   for (int i = 0; i < total; i++) {
       const char * name = das_context_get_variable_name(ctx, i);
       int sz = das_context_get_variable_size(ctx, i);
       printf("  [%d] %s (size=%d)\n", i, name, sz);
   }


Build & run
===========

Build::

   cmake --build build --config Release --target integration_c_07

Run::

   bin/Release/integration_c_07

Expected output::

   === Reading globals from C ===
   score = 42
   speed = 3.14
   player_name = Hero
   alive = true

   === Writing globals from C ===
   C set score = 9999
   C set speed = 99.5

   === Calling daslang to verify ===
     score       = 9999
     speed       = 99.5
     player_name = Hero
     alive       = true

   === All global variables ===
     [0] score (size=4)
     [1] speed (size=4)
     [2] player_name (size=8)
     [3] alive (size=1)


.. seealso::

   Full source:
   :download:`07_context_variables.c <../../../../tutorials/integration/c/07_context_variables.c>`,
   :download:`07_context_variables.das <../../../../tutorials/integration/c/07_context_variables.das>`

   Previous tutorial: :ref:`tutorial_integration_c_sandbox`

   Next tutorial: :ref:`tutorial_integration_c_serialization`

   C++ equivalent: :ref:`tutorial_integration_cpp_context_variables`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
