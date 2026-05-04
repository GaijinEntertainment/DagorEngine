.. _tutorial_integration_cpp_binding_types:

.. index::
   single: Tutorial; C++ Integration; Binding Types

============================================
 C++ Integration: Binding Types
============================================

This tutorial shows how to expose C++ structs to daslang so that scripts
can create instances, access fields, and pass them to/from C++ functions.
Topics covered:

* ``MAKE_TYPE_FACTORY`` — registering a C++ type with the daslang type system
* ``ManagedStructureAnnotation`` — describing struct fields
* ``addAnnotation`` — plugging type metadata into a module
* ``SimNode_ExtFuncCallAndCopyOrMove`` — returning bound types by value
* Factory functions — creating instances without ``unsafe``


Prerequisites
=============

* Tutorial 03 completed (:ref:`tutorial_integration_cpp_binding_functions`).
* Familiarity with ``addExtern`` and ``SideEffects``.


Defining the C++ types
=======================

We define three simple structs.  They are intentionally **POD** (no default
member initializers, no virtual functions) so that the daslang type system
sees them as plain data:

.. code-block:: cpp

   struct Vec2 {
       float x;
       float y;
   };

   struct Color {
       uint8_t r;
       uint8_t g;
       uint8_t b;
       uint8_t a;
   };

   struct Rect {
       Vec2  pos;
       Vec2  size;
   };


``MAKE_TYPE_FACTORY``
======================

Before daslang can work with a C++ type, you must declare a
**type factory** at file scope.  ``MAKE_TYPE_FACTORY`` creates two things:
``typeFactory<CppType>`` (so ``addExtern`` resolves the type in function
signatures) and ``typeName<CppType>`` (the type's display name):

.. code-block:: cpp

   MAKE_TYPE_FACTORY(Vec2,  Vec2);
   MAKE_TYPE_FACTORY(Color, Color);
   MAKE_TYPE_FACTORY(Rect,  Rect);

The first argument is the daslang-visible name; the second is the C++
type.  They can differ when the C++ name lives in a namespace — e.g.
``MAKE_TYPE_FACTORY(Vec2, math::Vec2)``.


``ManagedStructureAnnotation``
================================

An **annotation** describes a type's layout to daslang.  For structs,
derive from ``ManagedStructureAnnotation<T>`` and call ``addField`` for
each member:

.. code-block:: cpp

   struct Vec2Annotation : ManagedStructureAnnotation<Vec2, false> {
       Vec2Annotation(ModuleLibrary & ml)
           : ManagedStructureAnnotation("Vec2", ml)
       {
           addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
           addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
       }
   };

The template parameters are ``<CppType, canNew, canDelete>``.  Passing
``false`` for ``canNew`` prevents scripts from calling ``new Vec2()``
directly (we provide factory functions instead).

``DAS_BIND_MANAGED_FIELD(member)`` resolves the offset and type of a
struct member at compile time.  The two string arguments are the daslang
field name and the C++ field name (used for AOT).

Nested bound types work naturally — ``Rect`` has ``Vec2`` fields:

.. code-block:: cpp

   struct RectAnnotation : ManagedStructureAnnotation<Rect, false> {
       RectAnnotation(ModuleLibrary & ml)
           : ManagedStructureAnnotation("Rect", ml)
       {
           addField<DAS_BIND_MANAGED_FIELD(pos)>("pos",   "pos");
           addField<DAS_BIND_MANAGED_FIELD(size)>("size", "size");
       }
   };


Registering annotations in the module
=======================================

Call ``addAnnotation`` in the module constructor.  **Order matters** —
if type B contains type A as a field, register A first:

.. code-block:: cpp

   addAnnotation(make_smart<Vec2Annotation>(lib));   // Vec2 first
   addAnnotation(make_smart<ColorAnnotation>(lib));
   addAnnotation(make_smart<RectAnnotation>(lib));   // Rect uses Vec2


Returning bound types by value
================================

When a C++ function returns a bound struct by value, ``addExtern`` needs
the ``SimNode_ExtFuncCallAndCopyOrMove`` sim-node so that the return
value is properly copied into daslang's stack:

.. code-block:: cpp

   Vec2 vec2_add(const Vec2 & a, const Vec2 & b) {
       return { a.x + b.x, a.y + b.y };
   }

   addExtern<DAS_BIND_FUN(vec2_add), SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "vec2_add",
       SideEffects::none, "vec2_add")
           ->args({"a", "b"});

Functions that return scalars (``float``, ``bool``, etc.) or take bound
types by ``const &`` do not need this — the default sim-node works:

.. code-block:: cpp

   float vec2_length(const Vec2 & v) {
       return sqrtf(v.x * v.x + v.y * v.y);
   }

   addExtern<DAS_BIND_FUN(vec2_length)>(*this, lib, "vec2_length",
       SideEffects::none, "vec2_length")
           ->args({"v"});


Factory functions
==================

Types bound via ``ManagedStructureAnnotation`` are **handled** (reference)
types.  Creating a mutable local variable of such a type requires an
``unsafe`` block.  To give scripts a safe and ergonomic API, provide
**factory functions** that return the type by value:

.. code-block:: cpp

   Vec2 make_vec2(float x, float y) {
       Vec2 v;  v.x = x;  v.y = y;
       return v;
   }

   addExtern<DAS_BIND_FUN(make_vec2), SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "make_vec2",
       SideEffects::none, "make_vec2")
           ->args({"x", "y"});


Using bound types in daslang
==============================

.. code-block:: das

   require tutorial_04_cpp

   [export]
   def test() {
       // Immutable locals — no `unsafe` needed
       let a = make_vec2(3.0, 4.0)
       print("length(a) = {vec2_length(a)}\n")

       let c = vec2_add(a, make_vec2(1.0, 2.0))
       print("a + b = ({c.x}, {c.y})\n")

       // Mutable local requires `unsafe`
       unsafe {
           var d = make_vec2(3.0, 4.0)
           vec2_normalize(d)
           print("normalize = ({d.x}, {d.y})\n")
       }

       // Nested types and field access
       let r = make_rect(10.0, 20.0, 100.0, 50.0)
       print("rect area = {rect_area(r)}\n")

Immutable locals created via ``let`` from factory functions work without
``unsafe``.  Use ``unsafe { var ... }`` only when the variable must be
mutated (e.g. passed to a ``modifyArgument`` function).


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_04
   bin\Release\integration_cpp_04.exe

Expected output::

   a = (3, 4)
   length(a) = 5
   a + b = (4, 6)
   a * 2 = (6, 8)
   dot(a, b) = 11
   normalize(3,4) = (0.6, 0.8)
   color = (255, 128, 0, 255)
   rect area = 5000
   contains(50,30) = true
   contains(200,200) = false


.. seealso::

   Full source:
   :download:`04_binding_types.cpp <../../../../tutorials/integration/cpp/04_binding_types.cpp>`,
   :download:`04_binding_types.das <../../../../tutorials/integration/cpp/04_binding_types.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_binding_functions`

   Next tutorial: :ref:`tutorial_integration_cpp_binding_enums`
