.. _tutorial_integration_cpp_interop:

.. index::
   single: Tutorial; C++ Integration; Low-Level Interop

============================================
 C++ Integration: Low-Level Interop
============================================

This tutorial covers ``addInterop`` — the low-level alternative to
``addExtern`` for binding C++ functions to daslang.  Topics covered:

* ``addInterop`` vs ``addExtern`` — when to use each
* Accepting "any type" arguments (``vec4f`` template parameter)
* Runtime ``TypeInfo`` inspection via ``call->types[i]``
* Accessing call-site debug info via ``call->debugInfo``
* The ``TypeInfo`` union: ``structType`` / ``enumType`` / ``annotation_or_name``


Prerequisites
=============

* Tutorial 05 completed (:ref:`tutorial_integration_cpp_binding_enums`).
* Familiarity with ``addExtern`` and ``ManagedStructureAnnotation``.


When to use ``addInterop``
============================

``addExtern`` handles most function binding needs.  Use ``addInterop``
when you need capabilities that ``addExtern`` cannot provide:

* **"Any type" arguments** — accept values of any daslang type and
  inspect them at runtime
* **TypeInfo access** — inspect type metadata like field names, enum
  values, struct layout
* **Call-site debug info** — know the source file and line number of
  the caller

The built-in ``sprint``, ``hash``, and ``write`` functions all use
``addInterop`` internally.


The ``InteropFunction`` signature
===================================

Every interop function must match this signature:

.. code-block:: cpp

   vec4f my_function(Context & context,
                     SimNode_CallBase * call,
                     vec4f * args);

* ``context`` — the script execution context
* ``call`` — the call node, provides ``call->types[i]`` (TypeInfo)
  and ``call->debugInfo`` (source location)
* ``args`` — the raw simulation-level arguments as ``vec4f`` values


Inspecting ``TypeInfo``
========================

``call->types[i]`` returns a ``TypeInfo *`` for the *i*-th argument.
The ``type`` field tells you what kind of type was passed:

.. code-block:: cpp

   vec4f describe_type(Context & context,
                       SimNode_CallBase * call,
                       vec4f * args) {
       TypeInfo * ti = call->types[0];
       TextWriter tw;

       if (ti->type == Type::tHandle) {
           auto ann = ti->getAnnotation();
           tw << "type = handle, name = " << ann->name;
       } else if (ti->type == Type::tStructure && ti->structType) {
           tw << "type = struct, name = " << ti->structType->name;
           tw << ", fields = " << ti->structType->count;
       } else {
           tw << "type = " << das_to_string(ti->type);
           tw << ", size = " << getTypeSize(ti);
       }

       auto result = context.allocateString(tw.str(), &call->debugInfo);
       return cast<char *>::from(result);
   }


The ``TypeInfo`` union
========================

``TypeInfo`` contains a union of three pointers:

.. code-block:: cpp

   union {
       StructInfo *             structType;        // tStructure
       EnumInfo *               enumType;           // tEnumeration
       mutable TypeAnnotation * annotation_or_name; // tHandle
   };

.. warning::

   Accessing the wrong union member is **undefined behavior**.  Always
   check ``ti->type`` before accessing ``structType``, ``enumType``, or
   ``annotation_or_name``.

For handled types (``type == tHandle``), use ``ti->getAnnotation()``
to safely resolve the annotation — it handles tagged-pointer resolution
automatically.  ``das_to_string(Type::tHandle)`` returns an empty string;
use ``ti->getAnnotation()->name`` for the type name.


Registering interop functions
==============================

Use ``addInterop<FuncPtr, ReturnType, ArgTypes...>``:

.. code-block:: cpp

   addInterop<describe_type, char *, vec4f>(*this, lib, "describe_type",
       SideEffects::none, "describe_type")
           ->arg("value");

When an ``ArgType`` is ``vec4f``, it means "any daslang type."  Concrete
types like ``int32_t`` or ``const char *`` are also valid and work like
``addExtern``.


Accessing call-site debug info
================================

``call->debugInfo`` provides the source location of the call:

.. code-block:: cpp

   vec4f call_site_info(Context & context,
                        SimNode_CallBase * call,
                        vec4f *) {
       TextWriter tw;
       if (call->debugInfo.fileInfo) {
           tw << call->debugInfo.fileInfo->name
              << ":" << call->debugInfo.line;
       }
       auto result = context.allocateString(tw.str(), &call->debugInfo);
       return cast<char *>::from(result);
   }


Using interop functions from daslang
=======================================

The interop functions look like regular functions in daslang — the
"any type" argument accepts any value:

.. code-block:: das

   options gen2
   require tutorial_06_cpp

   struct MyPoint {
       x : float
       y : float
       tag : string
   }

   [export]
   def test() {
       // Primitives and handled types
       print("{describe_type(42)}\n")
       print("{describe_type(true)}\n")
       let p = make_particle(1.0, 2.0, 0.5, -0.3)
       print("{describe_type(p)}\n")

       // Pure daslang struct — has StructInfo with field metadata
       let pt = MyPoint(x = 10.0, y = 20.0, tag = "origin")
       print("{describe_type(pt)}\n")
       print("fields: {struct_field_names(pt)}\n")

       // Call-site info reports this script's file and line
       print("called from: {call_site_info()}\n")


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_06
   bin\Release\integration_cpp_06.exe

Expected output::

   === describe_type ===
   type = int, size = 4
   type = float, size = 4
   type = string, size = 8
   type = bool, size = 1
   type = handle, name = Particle, size = 16
   type = struct, name = MyPoint, fields = 3, size = 16

   === debug_print ===
   int:    42
   float:  3.1400001f
   string: "hello"
   bool:   true

   === any_hash ===
   hash(42)      = 0x...
   hash("hello") = 0x...
   hash(42) again = 0x...
   same hash? true

   === struct_field_names ===
   MyPoint fields: x, y, tag

   === call_site_info ===
   called from: .../06_interop.das:57


.. seealso::

   Full source:
   :download:`06_interop.cpp <../../../../tutorials/integration/cpp/06_interop.cpp>`,
   :download:`06_interop.das <../../../../tutorials/integration/cpp/06_interop.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_binding_enums`

   Next tutorial: :ref:`tutorial_integration_cpp_callbacks`
