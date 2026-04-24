.. _tutorial_integration_c_aot:

.. index::
   single: Tutorial; C Integration; AOT
   single: Tutorial; C Integration; Ahead-of-Time Compilation
   single: Tutorial; C Integration; das_function_is_aot
   single: Tutorial; C Integration; das_policies

=============================================
 C Integration: AOT
=============================================

This tutorial demonstrates **Ahead-of-Time compilation** via the C API.
AOT translates daslang functions into C++ source code at build time.
When linked into the host executable and enabled via policies, ``simulate()``
replaces interpreter nodes with direct native calls — near-C++ performance.


AOT workflow
============

1. **Generate** C++ from the script:
   ``daslang.exe utils/aot/main.das -- -aot script.das script.das.cpp``
2. **Compile** the generated ``.cpp`` into the host executable (CMake handles this)
3. **Set policies**: ``DAS_POLICY_AOT = 1`` before compilation
4. **Simulate**: the runtime links AOT functions automatically
5. **Verify**: ``das_function_is_aot(fn)`` returns 1


Using policies from C
=====================

The ``das_policies`` API lets you control ``CodeOfPolicies`` from C:

.. code-block:: c

   das_policies * pol = das_policies_make();
   das_policies_set_bool(pol, DAS_POLICY_AOT, 1);
   das_policies_set_bool(pol, DAS_POLICY_FAIL_ON_NO_AOT, 1);

   das_program * program = das_program_compile_policies(
       script, fa, tout, libgrp, pol);
   das_policies_release(pol);

Two enum types ensure type safety:

- ``das_bool_policy`` — boolean flags (``DAS_POLICY_AOT``,
  ``DAS_POLICY_NO_UNSAFE``, ``DAS_POLICY_NO_GLOBAL_VARIABLES``, etc.)
- ``das_int_policy`` — integer fields (``DAS_POLICY_STACK``,
  ``DAS_POLICY_MAX_HEAP_ALLOCATED``, etc.)

Using the wrong enum type will produce a compiler warning.


Checking AOT status
===================

After simulation, check whether a function was AOT-linked:

.. code-block:: c

   das_function * fn = das_context_find_function(ctx, "test");
   printf("test() is AOT: %s\n", das_function_is_aot(fn) ? "yes" : "no");


Available boolean policies
==========================

====================================  ===============================================
Enum value                            CodeOfPolicies field
====================================  ===============================================
``DAS_POLICY_AOT``                    ``aot`` — enable AOT linking
``DAS_POLICY_NO_UNSAFE``              ``no_unsafe`` — forbid unsafe blocks
``DAS_POLICY_NO_GLOBAL_VARIABLES``    ``no_global_variables`` — forbid module-level var
``DAS_POLICY_NO_GLOBAL_HEAP``         ``no_global_heap`` — forbid heap globals
``DAS_POLICY_NO_INIT``                ``no_init`` — forbid [init] functions
``DAS_POLICY_FAIL_ON_NO_AOT``         ``fail_on_no_aot`` — missing AOT is error
``DAS_POLICY_THREADLOCK_CONTEXT``     ``threadlock_context`` — context mutex
``DAS_POLICY_INTERN_STRINGS``         ``intern_strings`` — string interning
``DAS_POLICY_PERSISTENT_HEAP``        ``persistent_heap`` — no GC between calls
``DAS_POLICY_MULTIPLE_CONTEXTS``      ``multiple_contexts`` — context safety
``DAS_POLICY_STRICT_SMART_POINTERS``  ``strict_smart_pointers`` — var inscope rules
``DAS_POLICY_RTTI``                   ``rtti`` — extended RTTI
``DAS_POLICY_NO_OPTIMIZATIONS``       ``no_optimizations`` — disable all optimizations
====================================  ===============================================


Available integer policies
==========================

========================================  ===============================================
Enum value                                CodeOfPolicies field
========================================  ===============================================
``DAS_POLICY_STACK``                      ``stack`` — context stack size (bytes)
``DAS_POLICY_MAX_HEAP_ALLOCATED``         ``max_heap_allocated`` — max heap (0 = unlimited)
``DAS_POLICY_MAX_STRING_HEAP_ALLOCATED``  ``max_string_heap_allocated``
``DAS_POLICY_HEAP_SIZE_HINT``             ``heap_size_hint`` — initial heap size
``DAS_POLICY_STRING_HEAP_SIZE_HINT``      ``string_heap_size_hint``
========================================  ===============================================


Build & run
===========

From the source tree
--------------------

Build::

   cmake --build build --config Release --target integration_c_09

Run::

   bin/Release/integration_c_09

From the installed SDK
-----------------------

The installed SDK includes a standalone ``CMakeLists.txt`` for all C
integration tutorials.  Configure and build against the SDK:

.. code-block:: bash

   mkdir build_c && cd build_c
   cmake -DCMAKE_PREFIX_PATH=/path/to/daslang /path/to/daslang/tutorials/integration/c
   cmake --build . --config Release

On Windows:

.. code-block:: powershell

   mkdir build_c; cd build_c
   cmake -DCMAKE_PREFIX_PATH=D:\daslang D:\daslang\tutorials\integration\c
   cmake --build . --config Release

The standalone ``CMakeLists.txt`` handles AOT code generation automatically.
It reuses the same ``13_aot.das`` script from the C++ tutorials:

.. code-block:: cmake

   set(AOT_C09_SRC)
   DAS_TUTORIAL_AOT("${CPP_TUT_DIR}/13_aot.das" -aot AOT_C09_SRC integration_c_09)

   DAS_C_TUTORIAL(integration_c_09
       "${TUT_DIR}/09_aot.c"
       "${AOT_C09_SRC}"
   )

See :ref:`tutorial_building_from_sdk` for the full guide on building your own
projects with AOT support.


Expected output
----------------

::

   === Interpreter mode ===
     test() is AOT: no
   === AOT Tutorial ===
   fib(0) = 0
   fib(1) = 1
   ...
   fib(9) = 34
   sin(pi/4) = 0.70710677
   cos(pi/4) = 0.70710677
   sqrt(2)   = 1.4142135

   === AOT mode (policies.aot = true) ===
     test() is AOT: yes
   === AOT Tutorial ===
   fib(0) = 0
   ...
   fib(9) = 34


.. seealso::

   Full source:
   :download:`09_aot.c <../../../../tutorials/integration/c/09_aot.c>`

   This tutorial reuses the script from the C++ AOT tutorial:
   :download:`13_aot.das <../../../../tutorials/integration/cpp/13_aot.das>`

   Previous tutorial: :ref:`tutorial_integration_c_serialization`

   Next tutorial: :ref:`tutorial_integration_c_threading`

   C++ equivalent: :ref:`tutorial_integration_cpp_aot`

   Building from the SDK: :ref:`tutorial_building_from_sdk`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
