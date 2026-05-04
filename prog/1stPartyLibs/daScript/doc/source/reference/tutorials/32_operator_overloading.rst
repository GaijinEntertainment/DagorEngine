.. _tutorial_operator_overloading:

========================
Operator Overloading
========================

.. index::
    single: Tutorial; Operator Overloading
    single: Tutorial; Operators
    single: Tutorial; Custom Operators
    single: Tutorial; Property Accessors

This tutorial covers operator overloading in daslang — defining custom behavior
for built-in operators when applied to your own types.

daslang operators are regular functions named ``def operator <op>`` and follow
standard overload‑resolution rules. They can be declared as free functions or
as struct methods.

Arithmetic operators
====================

Overload ``+``, ``-``, ``*``, ``/``, ``%`` to give your types arithmetic
behaviour. Each operator is a binary function that takes two values and returns
a result:

.. code-block:: das

    struct Vec2 {
        x : float
        y : float
    }

    def operator +(a, b : Vec2) : Vec2 {
        return Vec2(x = a.x + b.x, y = a.y + b.y)
    }

    def operator -(a, b : Vec2) : Vec2 {
        return Vec2(x = a.x - b.x, y = a.y - b.y)
    }

    def operator *(a : Vec2; s : float) : Vec2 {
        return Vec2(x = a.x * s, y = a.y * s)
    }

    def operator *(s : float; a : Vec2) : Vec2 {
        return a * s
    }

    def operator /(a : Vec2; s : float) : Vec2 {
        return Vec2(x = a.x / s, y = a.y / s)
    }

Usage::

    let a = Vec2(x = 1.0, y = 2.0)
    let b = Vec2(x = 3.0, y = 4.0)
    print("{(a + b).x}, {(a + b).y}\n")  // 4, 6
    print("{(a * 3.0).x}\n")             // 3

Note that ``operator *`` is defined twice — once for ``Vec2 * float`` and once
for ``float * Vec2`` — because daslang does not automatically commute
arguments.

Comparison operators
====================

Overload ``==``, ``!=``, ``<``, ``>``, ``<=``, ``>=`` for custom comparisons.
Returning ``bool`` is required:

.. code-block:: das

    def operator ==(a, b : Vec2) : bool {
        return a.x == b.x && a.y == b.y
    }

    def operator !=(a, b : Vec2) : bool {
        return !(a == b)
    }

    // Order by squared magnitude (avoids sqrt)
    def operator <(a, b : Vec2) : bool {
        return (a.x * a.x + a.y * a.y) < (b.x * b.x + b.y * b.y)
    }

Usage::

    let a = Vec2(x = 1.0, y = 2.0)
    let c = Vec2(x = 3.0, y = 4.0)
    print("a < c: {a < c}\n")   // true

.. note::

    daslang does **not** auto-generate ``!=`` from ``==``, or ``>`` from ``<``.
    Each comparison operator must be defined explicitly if needed.

Unary operators
===============

Overload unary ``-`` (negate), ``~`` (bitwise complement), ``!`` (logical not),
and prefix/postfix ``++``/``--``:

.. code-block:: das

    def operator -(a : Vec2) : Vec2 {
        return Vec2(x = -a.x, y = -a.y)
    }

Usage::

    let a = Vec2(x = 1.0, y = 2.0)
    let neg = -a
    print("-a = ({neg.x}, {neg.y})\n")   // -a = (-1, -2)

For increment/decrement, ``++operator`` is prefix (modify-then-use) while
``operator ++`` is postfix (use-then-modify).

Compound assignment operators
=============================

Overload ``+=``, ``-=``, ``*=``, ``/=``, ``%=``, ``&=``, ``|=``, ``^=``,
``<<=``, ``>>=`` and others for in-place modification. The first argument must
be ``var ... &`` (mutable reference):

.. code-block:: das

    def operator +=(var a : Vec2&; b : Vec2) {
        a.x += b.x
        a.y += b.y
    }

    def operator -=(var a : Vec2&; b : Vec2) {
        a.x -= b.x
        a.y -= b.y
    }

    def operator *=(var a : Vec2&; s : float) {
        a.x *= s
        a.y *= s
    }

Usage::

    var a = Vec2(x = 1.0, y = 2.0)
    a += Vec2(x = 0.5, y = 0.5)
    // a is now (1.5, 2.5)

Index operators
===============

Index operators control how ``[]`` behaves on your types:

=================  ==========================================
Operator           Purpose
=================  ==========================================
``operator []``    Read access — ``v = obj[i]``
``operator []=``   Write access — ``obj[i] = v``
``operator []+=``  Compound index — ``obj[i] += v``
``operator []-=``  Compound index — ``obj[i] -= v``
``operator []<-``  Move into index — ``obj[i] <- v``
``operator []:=``  Clone into index — ``obj[i] := v``
=================  ==========================================

Example as free functions:

.. code-block:: das

    struct Matrix2x2 {
        data : float[4]
    }

    def operator [](m : Matrix2x2; i : int) : float {
        return m.data[i]
    }

    def operator []=(var m : Matrix2x2&; i : int; v : float) {
        m.data[i] = v
    }

    def operator []+=(var m : Matrix2x2&; i : int; v : float) {
        m.data[i] += v
    }

Usage::

    var m : Matrix2x2
    m.data[0] = 1.0
    m[0] = 10.0       // calls operator []=
    m[0] += 5.0        // calls operator []+=
    print("{m[0]}\n")  // 15

The safe index operator ``?[]`` returns a default value when the index is out of
range, following the same pattern.

Dot operators / property accessors
==================================

Dot operators let you create computed properties that look like regular field
access.

``operator . name`` defines a getter, ``operator . name :=`` defines a setter:

.. code-block:: das

    struct Particle {
        pos_x : float
        pos_y : float
    }

    // getter — computed "speed" property
    def operator . speed(p : Particle) : float {
        return sqrt(p.pos_x * p.pos_x + p.pos_y * p.pos_y)
    }

    // setter — scales direction to target speed
    def operator . speed := (var p : Particle&; value : float) {
        let mag = sqrt(p.pos_x * p.pos_x + p.pos_y * p.pos_y)
        if (mag > 0.0) {
            let scale = value / mag
            p.pos_x *= scale
            p.pos_y *= scale
        }
    }

Usage::

    var p = Particle(pos_x = 3.0, pos_y = 4.0)
    print("{p.speed}\n")  // 5
    p.speed := 10.0
    // p is now (6, 8)

The generic ``operator .`` takes a string field name and intercepts all field
accesses. This is powerful but should be used sparingly::

    def operator .(t : MyType; name : string) : string {
        return "accessing: {name}"
    }

The ``". ."`` (dot-space-dot) syntax bypasses any overloaded dot operator to
access real struct fields::

    print("{p . . pos_x}\n")   // accesses the actual field

Clone and finalize
==================

``operator :=`` overloads clone behaviour (the ``:=`` operator) and
``operator delete`` (or defining a ``finalize`` function) overloads deletion:

.. code-block:: das

    struct Resource {
        name : string
        refcount : int
    }

    def operator :=(var dst : Resource&; src : Resource) {
        dst.name = src.name
        dst.refcount = src.refcount + 1
    }

Usage::

    var orig = Resource(name = "texture", refcount = 1)
    var copy : Resource
    copy := orig
    print("{copy.refcount}\n")   // 2 — clone incremented the count

.. seealso::

    :ref:`Move, copy, and clone <move_copy_clone>` for more details.

Struct method operators
=======================

Operators can be defined as struct methods instead of free functions. This is
useful for encapsulation — the operator lives with the type definition:

.. code-block:: das

    struct Stack {
        items : array<int>

        def const operator [](index : int) : int {
            return items[index]
        }

        def operator []=(index : int; value : int) {
            items[index] = value
        }
    }

The ``const`` qualifier on the getter means it does not modify the struct.
Write operators omit ``const`` because they mutate state.

Usage::

    var s : Stack
    s.items |> push(10)
    s.items |> push(20)
    print("{s[0]}\n")   // 10
    s[1] = 99
    print("{s[1]}\n")   // 99

Generic operators
=================

Operators (and operator-like functions) can be generic using ``auto`` types.
This lets one definition cover multiple types, as long as they have the required
fields:

.. code-block:: das

    struct Vec3 {
        x : float
        y : float
        z : float
    }

    def dot_2d(a, b : auto) : float {
        return a.x * b.x + a.y * b.y
    }

Both ``Vec2`` and ``Vec3`` have ``x`` and ``y`` fields, so ``dot_2d`` works
with either::

    let v2a = Vec2(x = 1.0, y = 0.0)
    let v2b = Vec2(x = 0.0, y = 1.0)
    print("{dot_2d(v2a, v2b)}\n")   // 0

    let v3a = Vec3(x = 3.0, y = 4.0, z = 0.0)
    let v3b = Vec3(x = 1.0, y = 0.0, z = 99.0)
    print("{dot_2d(v3a, v3b)}\n")   // 3

For more sophisticated constraints, use :ref:`contracts <tutorial_contracts>`.

Complete operator reference
===========================

The full list of overloadable operators in daslang:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Category
     - Operators
   * - Arithmetic
     - ``+``  ``-``  ``*``  ``/``  ``%``
   * - Comparison
     - ``==``  ``!=``  ``<``  ``>``  ``<=``  ``>=``
   * - Bitwise
     - ``&``  ``|``  ``^``  ``~``  ``<<``  ``>>``  ``<<<``  ``>>>``
   * - Logical
     - ``&&``  ``||``  ``^^``  ``!``
   * - Unary
     - ``-`` (negate)  ``~`` (complement)  ``++``  ``--``
   * - Compound assignment
     - ``+=``  ``-=``  ``*=``  ``/=``  ``%=``  ``&=``  ``|=``  ``^=``  ``<<=``  ``>>=``  ``<<<=``  ``>>>=``  ``&&=``  ``||=``  ``^^=``
   * - Index
     - ``[]``  ``[]=``  ``[]<-``  ``[]:=``  ``[]+=``  ``[]-=``  ``[]*=``  etc.
   * - Safe index
     - ``?[]``
   * - Dot
     - ``.``  ``?.``  ``. name``  ``. name :=``  ``. name +=``  etc.
   * - Type
     - ``:=`` (clone)  ``delete`` (finalize)  ``is``  ``as``  ``?as``
   * - Null coalesce
     - ``??``
   * - Interval
     - ``..``

.. seealso::

   Full source: :download:`tutorials/language/32_operator_overloading.das <../../../../tutorials/language/32_operator_overloading.das>`

   Next tutorial: :ref:`tutorial_algorithm`

   :ref:`Regular expressions tutorial <tutorial_regex>` (previous tutorial).

   :ref:`Functions <functions>` — function declaration and operator overloading reference.

   :ref:`Generic programming <generic_programming>` — generic operators with ``auto`` types.
