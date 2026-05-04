.. _tutorial_variables:

========================
Variables and Types
========================

.. index::
    single: Tutorial; Variables
    single: Tutorial; Types

This tutorial covers mutable and immutable variables, daslang's basic types,
type inference, explicit annotations, and the strict no-implicit-conversion
rule.

var vs let
==========

Use ``var`` for mutable variables and ``let`` for immutable ones::

  var score = 0
  score = 100        // OK — score is mutable

  let maxScore = 999
  // maxScore = 0    // ERROR — cannot modify a constant

Type inference
==============

The compiler infers the type from the right-hand side::

  var i = 42          // int
  var f = 3.14        // float
  var b = true        // bool
  var s = "hello"     // string

You can also state the type explicitly with ``: Type``::

  var x : int = 10
  var y : float = 2.5
  var z : double = 1.0lf     // 'lf' suffix for double literals
  var flag : bool = false

No implicit conversions
=======================

daslang is strict — you cannot mix ``int`` and ``float`` in arithmetic.
Both sides must match::

  var i = 42
  var f = 3.14
  // let bad = i + f       // ERROR: int + float
  let result = float(i) + f    // OK — explicit cast

Likewise, ``bool`` is not interchangeable with ``int``::

  // let wrong = bool(1)   // ERROR
  let right = 1 != 0       // use comparison instead

Hex literals and unsigned types
===============================

Hex literals like ``0xFF`` are ``uint`` by default.  Use ``int(0xFF)`` to get
an ``int``.  64-bit literals use the ``l`` and ``ul`` suffixes::

  let h = 0xFF                  // uint
  let hi = int(0xFF)            // int
  var bigInt : int64 = 9_000_000_000l
  var bigUint : uint64 = 18_000_000_000ul

Zero initialization
===================

Variables declared with a type but no initializer are zero-initialized::

  var zeroInt : int        // 0
  var zeroFloat : float    // 0.0
  var zeroBool : bool      // false
  var zeroString : string  // ""

Running the tutorial
====================

::

  daslang.exe tutorials/language/02_variables.das

Full source: :download:`tutorials/language/02_variables.das <../../../../tutorials/language/02_variables.das>`

See also
========

* **Next:** :ref:`tutorial_operators` — operators and expressions
* :ref:`Datatypes <datatypes_and_values>` — complete type reference
* :ref:`Constants and enumerations <enumerations>` — constant values
