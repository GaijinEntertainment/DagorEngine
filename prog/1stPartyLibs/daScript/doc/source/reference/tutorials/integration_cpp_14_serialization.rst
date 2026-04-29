.. _tutorial_integration_cpp_serialization:

.. index::
   single: Tutorial; C++ Integration; Serialization

========================================
 C++ Integration: Serialization
========================================

This tutorial shows how to serialize a compiled daslang program to
a binary blob and deserialize it, skipping recompilation on subsequent
runs.  Topics covered:

* ``AstSerializer`` — the serializer/deserializer
* ``SerializationStorageVector`` — in-memory serialization buffer
* ``Program::serialize()`` — serializing a compiled program
* The compile → serialize → deserialize → simulate workflow


Prerequisites
=============

* Tutorial 13 completed (:ref:`tutorial_integration_cpp_aot`).


Why serialize?
==============

Compiling a daslang program involves parsing, type inference, and
optimization — this can take noticeable time for large scripts.
Serialization saves the compiled program to a binary blob so that
future runs can skip compilation entirely:

1. **First run** — compile from source, serialize to file/buffer
2. **Subsequent runs** — deserialize from blob, simulate, run

This is useful for:

* Faster startup in production applications
* Distributing pre-compiled scripts
* Caching compiled programs in editors and build pipelines


Required header
===============

.. code-block:: cpp

   #include "daScript/ast/ast_serializer.h"


The serialization workflow
==========================

Stage 1 — Compile from source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto program = compileDaScript(scriptPath, fAccess, tout, libGroup);
   // Must simulate once before serializing
   Context ctx(program->getContextStackSize());
   program->simulate(ctx, tout);

Stage 2 — Serialize to memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto writeTo = make_unique<SerializationStorageVector>();
   {
       AstSerializer ser(writeTo.get(), true);  // writing = true
       program->serialize(ser);
       ser.moduleLibrary = nullptr;
   }
   size_t blobSize = writeTo->buffer.size();
   program.reset();  // release original program

``SerializationStorageVector`` wraps a ``vector<uint8_t>`` buffer.
After serialization, the buffer contains the entire compiled program.

Stage 3 — Deserialize
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto readFrom = make_unique<SerializationStorageVector>();
   readFrom->buffer = das::move(writeTo->buffer);
   {
       AstSerializer deser(readFrom.get(), false);  // writing = false
       program = make_smart<Program>();
       program->serialize(deser);
       deser.moduleLibrary = nullptr;
   }

Note that ``Program::serialize()`` is used for both directions —
the ``AstSerializer`` constructor's ``isWriting`` flag determines
the mode.

Stage 4 — Simulate and run
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   Context ctx(program->getContextStackSize());
   program->simulate(ctx, tout);
   auto fn = ctx.findFunction("test");
   ctx.evalWithCatch(fn, nullptr);

The deserialized program works identically to a freshly compiled one.


Saving to and loading from disk
==================================

To persist across application restarts, write the buffer to a file:

.. code-block:: cpp

   // Save
   FILE * f = fopen("script.cache", "wb");
   fwrite(writeTo->buffer.data(), 1, writeTo->buffer.size(), f);
   fclose(f);

   // Load
   auto readFrom = make_unique<SerializationStorageVector>();
   FILE * f2 = fopen("script.cache", "rb");
   fseek(f2, 0, SEEK_END);
   size_t sz = ftell(f2);
   fseek(f2, 0, SEEK_SET);
   readFrom->buffer.resize(sz);
   fread(readFrom->buffer.data(), 1, sz, f2);
   fclose(f2);


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_14
   bin\Release\integration_cpp_14.exe

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
   :download:`14_serialization.cpp <../../../../tutorials/integration/cpp/14_serialization.cpp>`,
   :download:`14_serialization.das <../../../../tutorials/integration/cpp/14_serialization.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_aot`

   Next tutorial: :ref:`tutorial_integration_cpp_custom_annotations`
