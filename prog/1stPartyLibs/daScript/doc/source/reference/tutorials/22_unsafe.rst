.. _tutorial_unsafe:

========================
Unsafe and Pointers
========================

.. index::
    single: Tutorial; Unsafe
    single: Tutorial; Pointers
    single: Tutorial; Addr
    single: Tutorial; Reinterpret

This tutorial covers daslang's ``unsafe`` block for opt-in low-level
operations: raw pointers, ``addr``/``deref``, null handling, pointer
arithmetic, and ``reinterpret`` casts.

unsafe blocks
=============

``unsafe { ... }`` enables pointer operations and other low-level
features that are normally disallowed::

  var x = 42
  unsafe {
      var p : int? = addr(x)
      *p = 100
  }

Use ``unsafe(expr)`` for a single expression::

  let p = unsafe(addr(x))

addr and deref
==============

``addr(x)`` returns a pointer ``T?`` to a local variable.
``*p`` (or ``deref(p)``) follows the pointer::

  unsafe {
      var p = addr(x)
      print("{*p}\n")   // prints value of x
      *p = 999          // modifies x through pointer
  }

Null pointers
=============

Dereferencing a null pointer causes a panic inside ``try``/``recover``::

  var np : int? = null
  try {
      print("{*np}\n")
  } recover {
      print("caught null deref\n")
  }

Pointer arithmetic
==================

Pointers can index into contiguous memory (e.g., arrays)::

  unsafe {
      var arr = [10, 20, 30]
      var p = addr(arr[0])
      print("{p[0]}, {p[1]}, {p[2]}\n")  // 10, 20, 30
  }

reinterpret
===========

``reinterpret`` performs a raw bit cast between types of the same size::

  let f = 1.0
  let bits = unsafe(reinterpret<int> f)
  print("{bits:x}\n")  // 3f800000

.. warning::

   ``unsafe`` does not propagate into lambdas or generators.
   Each lambda/generator body needs its own ``unsafe`` block if needed.

.. seealso::

   :ref:`Unsafe <unsafe>` in the language reference.

   Full source: :download:`tutorials/language/22_unsafe.das <../../../../tutorials/language/22_unsafe.das>`

   Next tutorial: :ref:`tutorial_string_format`
