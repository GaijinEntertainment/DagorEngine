.. _tutorial_integration_c_callbacks:

.. index::
   single: Tutorial; C Integration; Callbacks

======================================
 C Integration: Callbacks and Closures
======================================

This tutorial shows how C code can **receive and invoke** daslang callable
types: function pointers, lambdas, and blocks.


Three callable types
====================

daslang has three callable types, each with different semantics:

=====================  ========  =============================================
Type                   Mangled   Characteristics
=====================  ========  =============================================
**function pointer**   ``@@``    No capture, cheapest, like a C function pointer
**lambda**             ``@``     Heap-allocated, captures variables by reference
**block**              ``$``     Stack-allocated, captures, cannot outlive scope
=====================  ========  =============================================

The mangled signature encodes the callable type after the argument list:

- ``0<i;f>@@`` — function pointer taking ``(int, float)``
- ``0<i;f>@``  — lambda taking ``(int, float)``
- ``0<i;f>$``  — block taking ``(int, float)``

A leading return type is prepended:
``"s 0<i;f>@@ i f"`` means *returns string, takes function<(int,float):string>,
int, float*.


Calling a function pointer
==========================

.. code-block:: c

   vec4f c_call_function(das_context * ctx, das_node * node, vec4f * args) {
       das_function * callback = das_argument_function(args[0]);
       int a    = das_argument_int(args[1]);
       float b  = das_argument_float(args[2]);

       vec4f cb_args[2];
       cb_args[0] = das_result_int(a);
       cb_args[1] = das_result_float(b);

       vec4f ret = das_context_eval_with_catch(ctx, callback, cb_args);
       return ret;
   }

From daslang, pass a function pointer with ``@@``:

.. code-block:: das

   def format_result(a : int; b : float) : string {
       return "formatted: {a} + {b}"
   }

   let r = c_call_function(@@format_result, 10, 2.5)


Calling a lambda
================

Lambdas carry captured state.  When calling from C, **the first argument
must be the lambda itself** (its capture block):

.. code-block:: c

   vec4f c_call_lambda(das_context * ctx, das_node * node, vec4f * args) {
       das_lambda * lambda = das_argument_lambda(args[0]);
       int a    = das_argument_int(args[1]);
       float b  = das_argument_float(args[2]);

       // First arg = lambda capture, then actual arguments.
       vec4f lmb_args[3];
       lmb_args[0] = das_result_lambda(lambda);
       lmb_args[1] = das_result_int(a);
       lmb_args[2] = das_result_float(b);

       vec4f ret = das_context_eval_lambda(ctx, lambda, lmb_args);
       return ret;
   }

.. important::

   ``das_context_eval_lambda`` requires the lambda as ``lmb_args[0]``.
   Omitting it will crash or produce garbage results.

From daslang:

.. code-block:: das

   var counter = 0
   var lmb <- @(a : int; b : float) : string {
       counter += a
       return "lambda: a={a} b={b} counter={counter}"
   }
   let r = c_call_lambda(lmb, 5, 1.5)


Calling a block
===============

Blocks are lighter than lambdas — they live on the stack and cannot outlive
the scope that created them.  Unlike lambdas, **block arguments do not
include the block itself**:

.. code-block:: c

   vec4f c_call_block(das_context * ctx, das_node * node, vec4f * args) {
       das_block * block = das_argument_block(args[0]);
       int    a = das_argument_int(args[1]);
       float  b = das_argument_float(args[2]);

       vec4f blk_args[2];
       blk_args[0] = das_result_int(a);
       blk_args[1] = das_result_float(b);

       vec4f ret = das_context_eval_block(ctx, block, blk_args);
       return ret;
   }

From daslang:

.. code-block:: das

   var blk <- $(a : int; b : float) : string {
       counter += a
       return "block: a={a} b={b} counter={counter}"
   }
   let r = c_call_block(blk, 7, 0.5)


Mangled signature cheat sheet
=============================

.. code-block:: text

   "s 0<i;f>@@ i f"   →  function pointer callback
   "s 0<i;f>@  i f"   →  lambda callback
   "s 0<i;f>$  i f"   →  block callback


Building and running
====================

::

   cmake --build build --config Release --target integration_c_04
   bin\Release\integration_c_04.exe

Expected output::

   --- function pointer callback ---
   [C] calling function pointer with (10, 2.50)
   [C] callback returned: "formatted: 10 + 2.5"
   C returned: formatted: 10 + 2.5

   --- lambda callback ---
   [C] calling lambda with (5, 1.50)
   [C] lambda returned: "lambda: a=5 b=1.5 counter=5"
   C returned: lambda: a=5 b=1.5 counter=5
   [C] calling lambda with (3, 0.50)
   [C] lambda returned: "lambda: a=3 b=0.5 counter=8"
   C returned: lambda: a=3 b=0.5 counter=8

   --- block callback ---
   [C] calling block with (7, 0.50)
   [C] block returned: "block: a=7 b=0.5 counter=7"
   C returned: block: a=7 b=0.5 counter=7
   [C] calling block with (3, 1.50)
   [C] block returned: "block: a=3 b=1.5 counter=10"
   C returned: block: a=3 b=1.5 counter=10


.. seealso::

   Full source:
   :download:`04_callbacks.c <../../../../tutorials/integration/c/04_callbacks.c>`,
   :download:`04_callbacks.das <../../../../tutorials/integration/c/04_callbacks.das>`

   Previous tutorial: :ref:`tutorial_integration_c_binding_types`

   Next tutorial: :ref:`tutorial_integration_c_unaligned_advanced`

   :ref:`type_mangling` — complete type mangling reference
