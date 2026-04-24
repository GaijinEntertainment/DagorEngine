.. _tutorial_integration_c_hello_world:

.. index::
   single: Tutorial; C Integration; Hello World

=================================
 C Integration: Hello World
=================================

This tutorial shows how to embed daslang in a C application using the
``daScriptC.h`` pure-C API.  By the end you will have a standalone C program
that compiles a ``.das`` script, finds a function, calls it, and prints
``Hello from daslang!``.

.. note::

   The C API uses opaque handle types (pointers you never dereference) and
   ``vec4f`` for passing arguments.  A higher-level C++ API is also available
   in ``daScript.h``; this tutorial series focuses on the C API only.


Prerequisites
=============

* daslang built from source (``cmake --build build --config Release``).
  The build produces ``libDaScript`` which the C tutorial links against.
* The ``daScriptC.h`` header — located in ``include/daScript/daScriptC.h``.


The daslang file
=================

Create a minimal script with a single exported function:

.. code-block:: das

   options gen2

   [export]
   def test() {
       print("Hello from daslang!\n")
   }

The ``[export]`` annotation makes the function visible to the host so that
``das_context_find_function`` can locate it by name.


The C program
=============

The program follows a strict lifecycle.  Each step maps to one or two API
calls.


Step 1 — Initialize
--------------------

.. code-block:: c

   #include "daScript/daScriptC.h"

   int main(int argc, char ** argv) {
       das_initialize();

``das_initialize`` must be called **once** before any other API call.  It
registers all built-in modules (math, strings, etc.).


Step 2 — Create supporting objects
-----------------------------------

.. code-block:: c

       das_text_writer  * tout         = das_text_make_printer();
       das_module_group * module_group = das_modulegroup_make();
       das_file_access  * file_access  = das_fileaccess_make_default();

=========================  ==================================================
Object                     Purpose
=========================  ==================================================
``das_text_writer``        Receives compiler/runtime messages (prints to stdout)
``das_module_group``       Holds modules available during compilation
``das_file_access``        Provides file I/O to the compiler (default = disk)
=========================  ==================================================


Step 3 — Compile the script
----------------------------

.. code-block:: c

       das_program * program = das_program_compile(
           script_path, file_access, tout, module_group);

       if (das_program_err_count(program)) {
           // handle errors ...
       }

``das_program_compile`` reads the ``.das`` file, parses, type-checks, and
produces a program object.  Errors (if any) are written to ``tout`` and can
also be iterated with ``das_program_get_error``.


Step 4 — Create a context and simulate
----------------------------------------

.. code-block:: c

       das_context * ctx = das_context_make(
           das_program_context_stack_size(program));

       if (!das_program_simulate(program, ctx, tout)) {
           // handle errors ...
       }

A **context** is the runtime environment — it owns the execution stack, global
variables, and heap.  **Simulation** resolves function pointers, initializes
globals, and prepares everything for execution.


Step 5 — Find the function
----------------------------

.. code-block:: c

       das_function * fn_test = das_context_find_function(ctx, "test");
       if (!fn_test) {
           printf("function 'test' not found\n");
       }

Any function marked ``[export]`` in the script can be found by name.


Step 6 — Call the function
---------------------------

.. code-block:: c

       das_context_eval_with_catch(ctx, fn_test, NULL);

       char * exception = das_context_get_exception(ctx);
       if (exception) {
           printf("exception: %s\n", exception);
       }

``das_context_eval_with_catch`` runs the function inside a try/catch so that
daslang exceptions do not crash the host application.  The third argument is
an array of ``vec4f`` arguments — ``NULL`` here because ``test`` takes no
parameters.


Step 7 — Clean up and shut down
---------------------------------

.. code-block:: c

       das_context_release(ctx);
       das_program_release(program);
       das_fileaccess_release(file_access);
       das_modulegroup_release(module_group);
       das_text_release(tout);

       das_shutdown();
       return 0;
   }

Release objects in reverse order of creation.  ``das_shutdown`` frees all
global state — no API calls are allowed after it.


Key concepts
============

Opaque handles
   Every daslang object (program, context, function, etc.) is an **opaque
   handle** — a pointer whose internal layout is hidden from C.  You create
   them with ``das_*_make`` / ``das_*_create`` and free them with
   ``das_*_release``.  Never cast or dereference them.

Lifecycle ownership
   The C host owns all handles it creates.  ``das_program_release`` frees the
   program; ``das_context_release`` frees the context.  Forgetting to release
   a handle leaks memory.

Error checking
   Always check ``das_program_err_count`` after compilation **and** after
   simulation.  Always check ``das_context_get_exception`` after every
   ``das_context_eval_*`` call.


Building and running
====================

The tutorial is built automatically by CMake as part of the daslang project::

   cmake --build build --config Release --target integration_c_01

Run from the project root so that the script path resolves correctly::

   bin\Release\integration_c_01.exe

Expected output::

   Hello from daslang!

You can also build all C integration tutorials from the installed SDK
— see :ref:`tutorial_building_from_sdk`.


.. seealso::

   Full source:
   :download:`01_hello_world.c <../../../../tutorials/integration/c/01_hello_world.c>`,
   :download:`01_hello_world.das <../../../../tutorials/integration/c/01_hello_world.das>`

   Next tutorial: :ref:`tutorial_integration_c_calling_functions`

   Building from the SDK: :ref:`tutorial_building_from_sdk`

   :ref:`type_mangling` — how types are encoded as strings in the C API

   daScriptC.h API header: ``include/daScript/daScriptC.h``
