.. _tutorial_operators:

========================
Operators
========================

.. index::
    single: Tutorial; Operators

This tutorial covers arithmetic, comparison, logical, bitwise, and assignment
operators, plus the ternary conditional and the distinction between copy
(``=``) and move (``<-``).

Arithmetic
==========

Both operands must be the same type — no implicit promotion::

  let a = 17
  let b = 5
  print("a + b = {a + b}\n")   // 22
  print("a / b = {a / b}\n")   // 3  (integer division)
  print("a % b = {a % b}\n")   // 2  (remainder)

  let x = 10.0
  let y = 3.0
  print("x / y = {x / y}\n")   // 3.3333333

Comparison and logical
======================

::

  print("5 == 5 is {5 == 5}\n")       // true
  print("5 != 3 is {5 != 3}\n")       // true

  let t = true
  let f = false
  print("true && false = {t && f}\n") // false
  print("true || false = {t || f}\n") // true
  print("!true = {!t}\n")             // false

Logical ``&&`` and ``||`` short-circuit: the right side is not evaluated if the
left side already determines the result.

Ternary operator
================

::

  let val = 42
  let desc = val > 0 ? "positive" : "non-positive"
  print("{val} is {desc}\n")

Increment, decrement, and compound assignment
==============================================

::

  var counter = 10
  counter++             // 11
  counter--             // 10

  var n = 100
  n += 10               // 110
  n -= 20               // 90
  n *= 2                // 180
  n /= 3                // 60
  n %= 7                // 4

Bitwise operators
=================

::

  let m = 0x0F
  let k = 0xF0
  print("0x0F & 0xF0 = {int(m & k)}\n")    // 0
  print("0x0F | 0xF0 = {int(m | k)}\n")    // 255
  print("0x0F ^ 0xF0 = {int(m ^ k)}\n")    // 255
  print("0x0F << 4 = {int(m << 4u)}\n")     // 240

Copy vs move — introduction
============================

``=`` is copy assignment — works for simple types and copyable values.
``<-`` is move assignment — transfers ownership and zeroes the source::

  var p = 10
  var q = p       // q is a copy of p
  q = 99          // p is unchanged

  var s = 42
  var r <- s      // r gets 42, s is zeroed

Move matters for arrays, tables, lambdas, and other heap-allocated types.
See :ref:`tutorial_arrays` and :ref:`Move, copy, clone <move_copy_clone>`.

Running the tutorial
====================

::

  daslang.exe tutorials/language/03_operators.das

Full source: :download:`tutorials/language/03_operators.das <../../../../tutorials/language/03_operators.das>`

See also
========

* **Next:** :ref:`tutorial_control_flow` — control flow
* :ref:`Expressions <expressions>` — full expression reference
