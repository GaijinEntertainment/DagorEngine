.. _tutorial_integration_c_type_introspection:

.. index::
   single: Tutorial; C Integration; Type Introspection
   single: Tutorial; C Integration; Struct Layout
   single: Tutorial; C Integration; das_type_info
   single: Tutorial; C Integration; das_struct_info

=============================================
 C Integration: Type Introspection
=============================================

This tutorial demonstrates how to **inspect types, struct layouts,
enumerations, and function signatures** from a C host using the
``daScriptC.h`` type introspection API.

After simulation, all type metadata is available through opaque handles.
The key entry points are:

==================================================  ===============================================
Function                                            Purpose
==================================================  ===============================================
``das_context_get_variable_type(ctx, idx)``         Type info for a global variable
``das_type_info_get_type(info)``                    Base type classification (``das_base_type``)
``das_type_info_get_size(info)``                    Size in bytes
``das_type_info_get_first_type(info)``              Element/key type for arrays, pointers, tables
``das_type_info_get_struct(info)``                  Struct layout (``das_struct_info``)
``das_struct_info_get_field_name(si, idx)``         Field name
``das_struct_info_get_field_offset(si, idx)``       Field byte offset
``das_struct_info_get_field_type(si, idx)``         Field type info
``das_type_info_get_enum(info)``                    Enum values (``das_enum_info``)
``das_function_get_info(fn)``                       Function debug info (``das_func_info``)
==================================================  ===============================================


The daslang script
===================

The companion script defines a struct hierarchy, an enum, and exported
functions for the C host to inspect:

.. code-block:: das

   options gen2
   options rtti

   enum Direction {
       north
       south
       east
       west
   }

   struct Transform {
       x : float = 0.0
       y : float = 0.0
       z : float = 0.0
       scale : float = 1.0
   }

   struct Entity {
       name : string
       transform : Transform = Transform()
       health : int
       direction : Direction
   }

   var entities : array<Entity>

   [export]
   def spawn_entity(n : string; px, py, pz : float) {
       entities |> emplace(Entity(name=n, transform=Transform(x=px, y=py, z=pz)))
   }

   [export]
   def get_entity_count() : int {
       return length(entities)
   }


Struct introspection
====================

Navigate from a global variable to its element type, then to the struct
layout:

.. code-block:: c

   int idx = das_context_find_variable(ctx, "entities");
   das_type_info * ti = das_context_get_variable_type(ctx, idx);

   // array<Entity> -> Entity
   das_type_info * elem = das_type_info_get_first_type(ti);
   das_struct_info * si = das_type_info_get_struct(elem);

   int nfields = das_struct_info_get_field_count(si);
   int struct_size = das_struct_info_get_size(si);

   for (int f = 0; f < nfields; f++) {
       const char * name = das_struct_info_get_field_name(si, f);
       int offset = das_struct_info_get_field_offset(si, f);
       das_type_info * ft = das_struct_info_get_field_type(si, f);
       das_base_type bt = das_type_info_get_type(ft);
       int fsz = das_type_info_get_size(ft);
   }

For nested structs (like ``Entity.transform``), call
``das_type_info_get_struct`` recursively on the field's type info.


Enum introspection
==================

When a field has an enumeration type, retrieve its values:

.. code-block:: c

   das_enum_info * ei = das_type_info_get_enum(ftype);
   int nvals = das_enum_info_get_count(ei);
   for (int v = 0; v < nvals; v++) {
       printf("%s = %lld\n",
              das_enum_info_get_value_name(ei, v),
              (long long)das_enum_info_get_value(ei, v));
   }


Function introspection
======================

Enumerate all functions and inspect their signatures:

.. code-block:: c

   int total = das_context_get_total_functions(ctx);
   for (int i = 0; i < total; i++) {
       das_function * fn = das_context_get_function(ctx, i);
       das_func_info * fi = das_function_get_info(fn);

       int nargs = das_func_info_get_arg_count(fi);
       for (int a = 0; a < nargs; a++) {
           const char * name = das_func_info_get_arg_name(fi, a);
           das_type_info * atype = das_func_info_get_arg_type(fi, a);
       }
       das_type_info * ret = das_func_info_get_result(fi);
   }


Engine-side allocation
======================

With the struct size and field offsets known, the engine can allocate
and populate structs without any daslang-side knowledge:

.. code-block:: c

   int struct_size = das_struct_info_get_size(si);
   char * buf = (char *)calloc(count, struct_size);

   // Write to the 'health' field at its discovered offset.
   int off = das_struct_info_get_field_offset(si, health_idx);
   for (int e = 0; e < count; e++) {
       int * hp = (int *)(buf + e * struct_size + off);
       *hp = 100 + e;
   }


Build & run
===========

Build::

   cmake --build build --config Release --target integration_c_11

Run::

   bin/Release/integration_c_11

Expected output::

   === Struct introspection ===
   'entities' type: array<Entity>
   'entities' base type: array

   Entity struct layout (with nested Transform):
     Entity (32 bytes, 4 fields):
       +0   name                 string     8 bytes
       +8   transform            struct     16 bytes
           Transform (16 bytes, 4 fields):
             +0   x                    float      4 bytes
             +4   y                    float      4 bytes
             +8   z                    float      4 bytes
             +12  scale                float      4 bytes
       +24  health               int        4 bytes
       +28  direction            enum       4 bytes

   === Enum introspection ===
   Enum 'Direction' (module: <script>):
     north = 0
     south = 1
     east = 2
     west = 3

   === Function introspection ===
   def spawn_entity(n : string const; px : float const; py : float const; pz : float const) : void
   def get_entity_count() : int const
   def Transform() : Transform
   def Entity() : Entity

   === Engine-side allocation ===
   Allocated 3 entities (32 bytes each, 96 total)
     entity[0].health = 100 (at byte offset 24)
     entity[1].health = 101 (at byte offset 56)
     entity[2].health = 102 (at byte offset 88)


.. seealso::

   Full source:
   :download:`11_type_introspection.c <../../../../tutorials/integration/c/11_type_introspection.c>`,
   :download:`11_type_introspection.das <../../../../tutorials/integration/c/11_type_introspection.das>`

   Previous tutorial: :ref:`tutorial_integration_c_threading`

   Next tutorial: :ref:`tutorial_integration_c_ecs`

   C API reference: :ref:`embedding_c_api`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
