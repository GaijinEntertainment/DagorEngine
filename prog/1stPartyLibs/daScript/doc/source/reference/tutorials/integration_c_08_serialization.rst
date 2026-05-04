.. _tutorial_integration_c_serialization:

.. index::
   single: Tutorial; C Integration; Serialization
   single: Tutorial; C Integration; das_program_serialize
   single: Tutorial; C Integration; das_program_deserialize

=============================================
 C Integration: Serialization
=============================================

This tutorial demonstrates how to **serialize** a compiled daslang program
to a binary blob and **deserialize** it back, skipping recompilation on
subsequent runs.

The workflow:

1. Compile a program normally with ``das_program_compile``
2. Simulate it at least once (resolves function pointers)
3. Serialize to an in-memory buffer
4. Release the original program
5. Deserialize from the buffer — no parsing, no type inference
6. Simulate and run as usual

This is useful for faster startup, distributing pre-compiled scripts,
and caching compiled programs in editors or build pipelines.


Serialization API
=================

Three functions make up the serialization API:

==================================================  ==========================================
Function                                            Purpose
==================================================  ==========================================
``das_program_serialize(prog, &data, &size)``       Serialize to binary; returns opaque handle
``das_program_deserialize(data, size)``              Deserialize from raw bytes; returns program
``das_serialized_data_release(blob)``               Free the serialization buffer
==================================================  ==========================================


Serializing
===========

.. code-block:: c

   const void * blob_data = NULL;
   int64_t blob_size = 0;
   das_serialized_data * blob = das_program_serialize(program, &blob_data, &blob_size);

   printf("Serialized size: %lld bytes\n", (long long)blob_size);

   // Save blob_data/blob_size to file for persistent caching if desired.

The program must have been simulated at least once before serialization.


Deserializing
=============

.. code-block:: c

   das_program * restored = das_program_deserialize(blob_data, blob_size);

   // The blob can be released after deserialization — the program is independent.
   das_serialized_data_release(blob);

   // Simulate and run as usual
   das_context * ctx = das_context_make(das_program_context_stack_size(restored));
   das_program_simulate(restored, ctx, tout);

   das_function * fn = das_context_find_function(ctx, "test");
   das_context_eval_with_catch(ctx, fn, NULL);


Build & run
===========

Build::

   cmake --build build --config Release --target integration_c_08

Run::

   bin/Release/integration_c_08

Expected output::

   === Stage 1: Compile from source ===
     Compiled successfully.
     Simulated successfully.

   === Stage 2: Serialize ===
     Serialized size: 5022 bytes
     Original program released.

   === Stage 3: Deserialize ===
     Deserialized successfully.

   === Stage 4: Simulate and run ===
   === Serialization Tutorial ===
   Hello, World!
   sum_to(10)  = 55
   sum_to(100) = 5050


.. seealso::

   Full source:
   :download:`08_serialization.c <../../../../tutorials/integration/c/08_serialization.c>`

   This tutorial reuses the script from the C++ serialization tutorial:
   :download:`14_serialization.das <../../../../tutorials/integration/cpp/14_serialization.das>`

   Previous tutorial: :ref:`tutorial_integration_c_context_variables`

   Next tutorial: :ref:`tutorial_integration_c_aot`

   C++ equivalent: :ref:`tutorial_integration_cpp_serialization`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
