.. _tutorial_integration_c_binding_types:

.. index::
   single: Tutorial; C Integration; Binding Types

================================
 C Integration: Binding C Types
================================

This tutorial shows how to create a **custom module** in C and expose C types
(enumerations, structures, aliases) to daslang so that scripts can use them
as native types.


Creating a module
=================

A module is a named container for types and functions.  The script refers to
it with ``require``:

.. code-block:: c

   das_module * mod = das_module_create("tutorial_c_03");

The module must be registered **before** compiling any script that
``require``\ s it.  After ``das_initialize()``, call your registration
function before ``das_program_compile``.


Binding an enumeration
======================

.. code-block:: c

   // C enum
   typedef enum { Color_red = 0, Color_green = 1, Color_blue = 2 } Color;

   // Bind to daslang
   das_enumeration * en = das_enumeration_make("Color", "Color", 1);
   das_enumeration_add_value(en, "red",   "Color_red",   Color_red);
   das_enumeration_add_value(en, "green", "Color_green", Color_green);
   das_enumeration_add_value(en, "blue",  "Color_blue",  Color_blue);
   das_module_bind_enumeration(mod, en);

The third argument to ``das_enumeration_make`` selects the underlying
integer type: ``0`` = int8, ``1`` = int32, ``2`` = int16.

In daslang:

.. code-block:: das

   let color = Color.green
   print("color = {color}\n")


Binding a structure
===================

C structures are exposed as **handled types**.  You specify size, alignment,
and each field's offset and mangled type:

.. code-block:: c

   typedef struct { float x; float y; } Point2D;

   das_structure * st = das_structure_make(lib, "Point2D", "Point2D",
                                          sizeof(Point2D), _Alignof(Point2D));
   das_structure_add_field(st, mod, lib, "x", "x", offsetof(Point2D, x), "f");
   das_structure_add_field(st, mod, lib, "y", "y", offsetof(Point2D, y), "f");
   das_module_bind_structure(mod, st);

In daslang:

.. code-block:: das

   var p : Point2D
   p.x = 3.0
   p.y = 4.0


Type mangling reference
=======================

Every type in the C API is described by a compact string:

=================  ========================
Mangled string     Type
=================  ========================
``i``              ``int``
``u``              ``uint``
``f``              ``float``
``d``              ``double``
``b``              ``bool``
``s``              ``string``
``v``              ``void``
``1<i>A``          ``array<int>``
``1<f>?``          ``float?`` (pointer)
``H<Name>``        handled struct ``Name``
=================  ========================

See :ref:`type_mangling` for the complete specification.


Binding a type alias
====================

.. code-block:: c

   das_module_bind_alias(mod, lib, "IntArray", "1<i>A");

In daslang, ``IntArray`` is now a synonym for ``array<int>``.


Binding interop functions
=========================

Functions that operate on custom types use the standard interop pattern, but
their mangled signatures reference the handled type by name:

.. code-block:: c

   // def point_distance(p : Point2D) : float
   // Mangled: "f H<Point2D>"
   vec4f c_point_distance(das_context * ctx, das_node * node, vec4f * args) {
       Point2D * p = (Point2D *)das_argument_ptr(args[0]);
       float dist = sqrtf(p->x * p->x + p->y * p->y);
       return das_result_float(dist);
   }

   das_module_bind_interop_function(mod, lib, &c_point_distance,
       "point_distance", "c_point_distance",
       SIDEEFFECTS_none, "f H<Point2D>");

Note the use of ``das_argument_ptr`` — structures are passed as pointers.


Building and running
====================

::

   cmake --build build --config Release --target integration_c_03
   bin\Release\integration_c_03.exe

Expected output::

   color = green
   numbers = [10, 20, 30]
   point = (3, 4)
   distance from origin = 5
   as string: (3.00, 4.00)


.. seealso::

   Full source:
   :download:`03_binding_types.c <../../../../tutorials/integration/c/03_binding_types.c>`,
   :download:`03_binding_types.das <../../../../tutorials/integration/c/03_binding_types.das>`

   Previous tutorial: :ref:`tutorial_integration_c_calling_functions`

   Next tutorial: :ref:`tutorial_integration_c_callbacks`

   :ref:`type_mangling` — complete type mangling reference
