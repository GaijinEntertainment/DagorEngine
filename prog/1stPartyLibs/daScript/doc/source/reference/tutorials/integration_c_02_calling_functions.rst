.. _tutorial_integration_c_calling_functions:

.. index::
   single: Tutorial; C Integration; Calling Functions

==========================================
 C Integration: Calling daslang Functions
==========================================

This tutorial shows how to pass arguments to daslang functions from C and
retrieve their return values.  It covers all the common scalar types (int,
float, string, bool) and introduces the **complex result** (cmres) calling
convention for functions that return structures.


Arguments and return values
===========================

Every value crossing the C ↔ daslang boundary is carried in a ``vec4f`` —
a 128-bit SSE register.  The API provides symmetric helper pairs:

=============================  ==============================
Packing (C → daslang)          Unpacking (daslang → C)
=============================  ==============================
``das_result_int(value)``      ``das_argument_int(vec4f)``
``das_result_float(value)``    ``das_argument_float(vec4f)``
``das_result_string(value)``   ``das_argument_string(vec4f)``
``das_result_bool(value)``     ``das_argument_bool(vec4f)``
=============================  ==============================

Despite their names, the ``das_result_*`` helpers are also used to **pack
arguments** into a ``vec4f`` array before calling a daslang function.


Calling a function with arguments
=================================

.. code-block:: c

   // Find the function (must be [export] in the script).
   das_function * fn = das_context_find_function(ctx, "add");

   // Pack two int arguments into a vec4f array.
   vec4f args[2];
   args[0] = das_result_int(17);
   args[1] = das_result_int(25);

   // Call and unpack the int return value.
   vec4f ret = das_context_eval_with_catch(ctx, fn, args);
   int result = das_argument_int(ret);    // 42

The corresponding daslang function:

.. code-block:: das

   [export]
   def add(a : int; b : int) : int {
       return a + b
   }


String arguments and return values
===================================

Strings are passed as plain ``char *`` pointers.  When daslang **returns**
a string, it lives on the context's heap and remains valid until the context
is released:

.. code-block:: c

   vec4f args[1];
   args[0] = das_result_string("World");

   vec4f ret = das_context_eval_with_catch(ctx, fn_greet, args);
   char * greeting = das_argument_string(ret);
   printf("%s\n", greeting);   // "Hello, World!"


Returning structures (complex results)
=======================================

When a daslang function returns a struct, use
``das_context_eval_with_catch_cmres``.  You allocate the struct on the C side
and pass a pointer:

.. code-block:: c

   typedef struct { float x; float y; } Vec2;

   vec4f args[2];
   args[0] = das_result_float(1.5f);
   args[1] = das_result_float(2.5f);

   Vec2 v;
   das_context_eval_with_catch_cmres(ctx, fn_make_vec2, args, &v);
   printf("x=%.1f y=%.1f\n", v.x, v.y);   // x=1.5 y=2.5

The C struct layout **must** match the daslang struct exactly (same field
order, same types, same padding).


Building and running
====================

::

   cmake --build build --config Release --target integration_c_02
   bin\Release\integration_c_02.exe

Expected output::

   add(17, 25) = 42
   square(3.5) = 12.25
   greet("World") = "Hello, World!"
   int=42 float=3.14 string=test bool=true
   make_vec2(1.5, 2.5) = { x=1.5, y=2.5 }


.. seealso::

   Full source:
   :download:`02_calling_functions.c <../../../../tutorials/integration/c/02_calling_functions.c>`,
   :download:`02_calling_functions.das <../../../../tutorials/integration/c/02_calling_functions.das>`

   Previous tutorial: :ref:`tutorial_integration_c_hello_world`

   Next tutorial: :ref:`tutorial_integration_c_binding_types`

   :ref:`type_mangling` — complete type mangling reference
