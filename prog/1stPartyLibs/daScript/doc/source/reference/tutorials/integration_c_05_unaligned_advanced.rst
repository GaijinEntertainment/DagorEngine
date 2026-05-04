.. _tutorial_integration_c_unaligned_advanced:

.. index::
   single: Tutorial; C Integration; Unaligned ABI
   single: Tutorial; C Integration; Inline Source
   single: Tutorial; C Integration; Error Reporting

=============================================
 C Integration: Unaligned ABI & Advanced
=============================================

This tutorial covers three advanced topics: the **unaligned calling
convention**, **inline script source**, and **detailed error reporting**.


The unaligned ABI
=================

The default (aligned) ABI uses ``vec4f`` — a 128-bit SSE type that **must**
be 16-byte aligned.  This is fast but requires SSE support and aligned
memory.

The **unaligned** ABI uses ``vec4f_unaligned`` — a plain struct of four
floats with no alignment requirement:

.. code-block:: c

   typedef struct { float x, y, z, w; } vec4f_unaligned;

This makes it portable to platforms without SSE (ARM, WASM, etc.).

====================================  ==========================================
Aligned                               Unaligned
====================================  ==========================================
``das_interop_function``              ``das_interop_function_unaligned``
``das_context_eval_with_catch``       ``das_context_eval_with_catch_unaligned``
``das_argument_int(args[i])``         ``das_argument_int_unaligned(args + i)``
``das_result_int(value)``             ``das_result_int_unaligned(result, value)``
``das_module_bind_interop_function``  ``das_module_bind_interop_function_unaligned``
====================================  ==========================================

Unaligned interop function
--------------------------

.. code-block:: c

   // Signature differs: returns void, result is an out-parameter.
   void c_greet(das_context * ctx, das_node * node,
                vec4f_unaligned * args, vec4f_unaligned * result) {
       int n = das_argument_int_unaligned(args + 0);
       printf("greet(%d)\n", n);
       das_result_void_unaligned(result);
   }

   // Bind with the _unaligned variant.
   das_module_bind_interop_function_unaligned(mod, lib, &c_greet,
       "c_greet", "c_greet", SIDEEFFECTS_modifyExternal, "v i");

Calling with unaligned eval
---------------------------

.. code-block:: c

   vec4f_unaligned args[2];
   das_result_int_unaligned(args + 0, 17);
   das_result_int_unaligned(args + 1, 25);

   vec4f_unaligned result;
   das_context_eval_with_catch_unaligned(ctx, fn_add, args, 2, &result);
   int sum = das_argument_int_unaligned(&result);

Note the extra ``narguments`` parameter (``2``) — the unaligned API needs
the argument count explicitly.


Inline script source
====================

Instead of loading a ``.das`` file from disk, you can embed the script as
a C string and register it as a virtual file:

.. code-block:: c

   static const char * SCRIPT =
       "options gen2\n"
       "[export]\n"
       "def test() {\n"
       "    print(\"Hello from inline script!\\n\")\n"
       "}\n";

   das_file_access * fa = das_fileaccess_make_default();
   das_fileaccess_introduce_file(fa, "inline.das", SCRIPT, 0);

   das_program * prog = das_program_compile("inline.das", fa, tout, lib);

The virtual file name (``"inline.das"``) can be anything — it never touches
the file system.  This is useful for:

- Embedded applications with no file system
- Testing and prototyping
- Configuration scripts shipped as resource data


Error reporting
===============

When compilation fails, iterate the errors with ``das_program_get_error``
and extract human-readable messages:

.. code-block:: c

   int err_count = das_program_err_count(program);
   for (int i = 0; i < err_count; i++) {
       das_error * error = das_program_get_error(program, i);

       // Option A: print to a text writer (stdout or string)
       das_error_output(error, tout);

       // Option B: extract into a C buffer
       char buf[1024];
       das_error_report(error, buf, sizeof(buf));
       printf("error: %s\n", buf);
   }


Building and running
====================

::

   cmake --build build --config Release --target integration_c_05
   bin\Release\integration_c_05.exe

Expected output (Part 1 — inline script + unaligned ABI)::

   === Part 1: Inline script with unaligned ABI ===

   Hello from inline script!
   [C unaligned] greet called with n=42
   add(17, 25) = 42

Expected output (Part 2 — error reporting)::

   === Part 2: Error reporting ===

   Compilation produced 1 error(s):
     error 0: bad_script.das:4:19: ...


.. seealso::

   Full source:
   :download:`05_unaligned_advanced.c <../../../../tutorials/integration/c/05_unaligned_advanced.c>`,
   :download:`05_unaligned_advanced.das <../../../../tutorials/integration/c/05_unaligned_advanced.das>`

   Previous tutorial: :ref:`tutorial_integration_c_callbacks`

   Next tutorial: :ref:`tutorial_integration_c_sandbox`

   :ref:`type_mangling` — complete type mangling reference

   daScriptC.h API header: ``include/daScript/daScriptC.h``
