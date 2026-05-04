.. _tutorial_integration_cpp_operators_and_properties:

.. index::
   single: Tutorial; C++ Integration; Operators and Properties

============================================
 C++ Integration: Operators and Properties
============================================

This tutorial shows how to bind C++ operators and property accessors
to daslang, making custom types feel native.  Topics covered:

* ``addEquNeq<T>`` — binding ``==`` and ``!=`` operators
* Custom arithmetic operators via ``addExtern`` with operator names
* ``addProperty<DAS_BIND_MANAGED_PROP(method)>`` — property accessors
* ``addPropertyExtConst`` — const/non-const property overloads
* ``addCtorAndUsing`` — exposing C++ constructors to daslang
* Overriding ``isPod()`` / ``hasNonTrivialCtor()`` for non-POD types
* Properties vs fields in ``ManagedStructureAnnotation``


Prerequisites
=============

* Tutorial 08 completed (:ref:`tutorial_integration_cpp_methods`).
* Familiarity with ``DAS_CALL_MEMBER`` and ``addExtern``.


Operators via ``addExtern``
=============================

daslang resolves operators by name.  To bind a C++ operator, register a
function with the operator symbol as its daslang name:

.. code-block:: cpp

   Vec3 vec3_add(const Vec3 & a, const Vec3 & b) {
       return { a.x + b.x, a.y + b.y, a.z + b.z };
   }

   Vec3 vec3_neg(const Vec3 & a) {
       return { -a.x, -a.y, -a.z };
   }

   // Binary "+"
   addExtern<DAS_BIND_FUN(vec3_add),
             SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "+",
       SideEffects::none, "vec3_add")
           ->args({"a", "b"});

   // Unary "-"
   addExtern<DAS_BIND_FUN(vec3_neg),
             SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "-",
       SideEffects::none, "vec3_neg")
           ->args({"a"});

Available operator names: ``+``, ``-``, ``*``, ``/``, ``%``, ``<<``,
``>>``, ``<``, ``>``, ``<=``, ``>=``, ``&``, ``|``, ``^``.

.. note::

   Functions returning handled types by value require
   ``SimNode_ExtFuncCallAndCopyOrMove`` as the second template argument.


``addEquNeq<T>``
==================

The helper ``addEquNeq<T>`` binds both ``==`` and ``!=`` operators in
one call.  It requires ``operator==`` and ``operator!=`` on the C++ type:

.. code-block:: cpp

   struct Vec3 {
       bool operator==(const Vec3 & o) const { ... }
       bool operator!=(const Vec3 & o) const { ... }
   };

   // In module constructor:
   addEquNeq<Vec3>(*this, lib);


Properties — ``addProperty``
===============================

Properties look like fields but call C++ methods under the hood.  Define
getter methods on the C++ type, then register them in the annotation:

.. code-block:: cpp

   struct Vec3 {
       float x, y, z;
       float length() const { return sqrtf(x*x + y*y + z*z); }
       float lengthSq() const { return x*x + y*y + z*z; }
       bool isZero() const { return x == 0 && y == 0 && z == 0; }
   };

.. code-block:: cpp

   struct Vec3Annotation : ManagedStructureAnnotation<Vec3, false> {
       Vec3Annotation(ModuleLibrary & ml)
           : ManagedStructureAnnotation("Vec3", ml)
       {
           // Regular fields
           addField<DAS_BIND_MANAGED_FIELD(x)>("x", "x");
           addField<DAS_BIND_MANAGED_FIELD(y)>("y", "y");
           addField<DAS_BIND_MANAGED_FIELD(z)>("z", "z");

           // Properties — method calls disguised as field access
           addProperty<DAS_BIND_MANAGED_PROP(length)>("length", "length");
           addProperty<DAS_BIND_MANAGED_PROP(lengthSq)>("lengthSq", "lengthSq");
           addProperty<DAS_BIND_MANAGED_PROP(isZero)>("isZero", "isZero");
       }
   };

In daslang, properties are accessed with dot syntax just like fields:

.. code-block:: das

   let v = make_vec3(3.0, 4.0, 0.0)
   print("{v.length}\n")     // 5.0 — calls Vec3::length()
   print("{v.isZero}\n")     // false — calls Vec3::isZero()
   print("{v.x}\n")          // 3.0 — direct field access


Const/non-const properties — ``addPropertyExtConst``
======================================================

When a C++ type has both const and non-const overloads of a method, use
``addPropertyExtConst`` to bind them as a single property.  daslang will
call the appropriate overload depending on whether the object is mutable
(``var``) or immutable (``let``):

.. code-block:: cpp

   struct Vec3 {
       // Non-const — called when the object is mutable
       bool editable() { return true; }
       // Const — called when the object is immutable
       bool editable() const { return false; }
   };

Register with explicit function-pointer types for both overloads:

.. code-block:: cpp

   // Template params: <NonConstSig, &Method, ConstSig, &Method>
   addPropertyExtConst<
       bool (Vec3::*)(),       &Vec3::editable,    // non-const
       bool (Vec3::*)() const, &Vec3::editable     // const
   >("editable", "editable");

In daslang, the property value depends on the variable's mutability:

.. code-block:: das

   let immutable_v = make_vec3(1.0, 2.0, 3.0)
   print("{immutable_v.editable}\n")     // false — const overload

   var mutable_v = make_vec3(1.0, 2.0, 3.0)
   print("{mutable_v.editable}\n")       // true — non-const overload


POD vs non-POD handled types
==============================

Whether a handled type requires ``unsafe`` for local variables depends on
the annotation's ``isLocal()`` method, which defaults to:

.. code-block:: cpp

   virtual bool isLocal() const {
       return isPod() && !hasNonTrivialCtor()
           && !hasNonTrivialDtor() && !hasNonTrivialCopy();
   }

* **POD types** (like ``Vec3`` — no constructors, no virtual functions):
  ``isLocal()`` returns ``true`` automatically.  ``let`` and ``var``
  work without ``unsafe``.
* **Non-POD types** (like a struct with a constructor):  ``isLocal()``
  returns ``false``, and any local variable — even ``let`` — gives:

  ::

     error[30108]: local variable of type ... requires unsafe

Consider a ``Color`` type with a non-trivial constructor:

.. code-block:: cpp

   struct Color {
       float r, g, b, a;
       Color() : r(0), g(0), b(0), a(1) {}  // makes it non-POD

       float brightness() const { return 0.299f*r + 0.587f*g + 0.114f*b; }
   };

Without annotation overrides, local Color variables need workarounds:

.. code-block:: das

   // ERROR: let c = Color()  → error[30108] without unsafe!

   // Way 1: `using` pattern — safe, constructs on stack via block
   using() $(var c : Color#) {
       c.r = 1.0
       c.g = 0.5
       print("{c.brightness}\n")   // 0.5925
   }

   // Way 2: `unsafe` block
   unsafe {
       let c = make_color(1.0, 0.5, 0.0, 1.0)
       print("{c.brightness}\n")   // 0.5925
   }


``addCtorAndUsing`` — exposing constructors
============================================

``addCtorAndUsing<T>`` registers two things:

1. A **constructor function** named after the type — calls placement new
   in daslang.
2. A **``using`` function** for block-based construction —
   ``using() $(var c : Type#) { ... }``.

.. code-block:: cpp

   // In the module constructor:
   addCtorAndUsing<Color>(*this, lib, "Color", "Color");

The ``using`` pattern is the safe way to work with non-POD types —
the variable lives inside a block, so no ``unsafe`` is required.
The ``#`` suffix in the block parameter type (``Color#``) marks it
as a temporary value.


Overriding ``isPod()`` — ``SafeColor``
=======================================

If you know a C++ type's non-trivial constructor is harmless (e.g., it
just zero-initializes fields), you can override the annotation to tell
daslang it is safe for local variables.  This avoids the need for
``unsafe`` or ``using``.

We define ``SafeColor`` as a separate struct with the same layout:

.. code-block:: cpp

   struct SafeColor {
       float r, g, b, a;
       SafeColor() : r(0), g(0), b(0), a(1) {}
       float brightness() const { ... }
   };

   struct SafeColorAnnotation
       : ManagedStructureAnnotation<SafeColor, false>
   {
       // ... fields, properties ...

       // Override: treat as POD-safe despite non-trivial ctor
       virtual bool isPod() const override { return true; }
       virtual bool isRawPod() const override { return false; }
       virtual bool hasNonTrivialCtor() const override { return false; }
   };

Now scripts can use SafeColor without ``unsafe``:

.. code-block:: das

   let sc = SafeColor()          // works — annotation says it's safe
   var sc2 = SafeColor()         // also works
   sc2.r = 1.0
   print("{sc2.brightness}\n")   // 0.299


Using operators in daslang
=============================

All operators work naturally:

.. code-block:: das

   options gen2
   require tutorial_09_cpp

   [export]
   def test() {
       let a = make_vec3(1.0, 2.0, 3.0)
       let b = make_vec3(4.0, 5.0, 6.0)

       let c = a + b          // calls vec3_add
       let d = a * 2.0        // calls vec3_mul_scalar
       let e = -a             // calls vec3_neg
       print("a == a: {a == a}\n")   // true
       print("a != b: {a != b}\n")   // true
       print("a.length = {a.length}\n")  // property access


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_09
   bin\Release\integration_cpp_09.exe

Expected output::

   === Vec3 basics ===
   a = (1, 2, 3)
   b = (4, 5, 6)

   === Properties ===
   a.length   = 3.7416575
   a.lengthSq = 14
   a.isZero   = false
   zero.isZero = true

   === Const/non-const property ===
   Vec3 (POD):
     let  v.editable = false
     var  v.editable = true
   Color (non-POD, using pattern):
     using: brightness = 0.5925
   Color (non-POD, unsafe):
     unsafe let: brightness = 0.5925
   SafeColor (overridden annotation, no unsafe needed):
     let sc = SafeColor() : brightness = 0
     var sc2.brightness    = 0.5925

   === Operators ===
   a + b = (5, 7, 9)
   b - a = (3, 3, 3)
   a * 2 = (2, 4, 6)
   -a    = (-1, -2, -3)

   === Equality ===
   a == a2: true
   a != b:  true
   a == b:  false

   === Utility functions ===
   dot(a, b) = 32
   cross(a, b) = (-3, 6, -3)
   normalize(a) = (0.26726124, 0.5345225, 0.8017837)
   normalize(a).length = 0.99999994


.. seealso::

   Full source:
   :download:`09_operators_and_properties.cpp <../../../../tutorials/integration/cpp/09_operators_and_properties.cpp>`,
   :download:`09_operators_and_properties.das <../../../../tutorials/integration/cpp/09_operators_and_properties.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_methods`
   Next tutorial: :ref:`tutorial_integration_cpp_custom_modules`
