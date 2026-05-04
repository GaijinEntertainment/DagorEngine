.. _tutorial_pointers:

========================
Pointers
========================

.. index::
    single: Tutorial; Pointers
    single: Tutorial; Addr
    single: Tutorial; Safe Navigation
    single: Tutorial; Void Pointer
    single: Tutorial; Pointer Arithmetic

This tutorial covers pointer types, creation, dereferencing, null safety, pointer
arithmetic, and low-level operations like ``reinterpret`` and ``intptr``.

Pointer types
=============

In daslang, ``T?`` is a nullable pointer to ``T``::

  var p : int?           // pointer to int — null by default
  var ps : Point?        // pointer to struct
  var vp : void?         // untyped (void) pointer

Pointers are used for heap-allocated data, optional references, and
low-level interop.  Unlike C/C++, field access auto-dereferences::

  p.x      // same as (*p).x — no -> operator needed

Creating pointers with new
==========================

``new`` allocates on the heap and returns a pointer::

  var p = new Point(x = 3.0, y = 4.0)  // p is Point?

Fields get default values when omitted::

  var q = new Point()     // x = 0.0, y = 0.0

Heap pointers must be freed explicitly with ``delete`` (requires ``unsafe``),
or use ``var inscope`` for automatic cleanup::

  var inscope pt = new Point(x = 1.0, y = 2.0)
  // pt is automatically deleted when it goes out of scope

addr and safe_addr
==================

``addr(x)`` returns a pointer to an existing variable.  It requires
``unsafe`` because the pointer could outlive the variable::

  var x = 42
  unsafe {
      var p = addr(x)    // p is int?
      *p = 100           // modifies x through the pointer
  }

``safe_addr`` from ``daslib/safe_addr`` returns a temporary pointer
(``T?#``) without requiring ``unsafe``::

  require daslib/safe_addr
  var a = 13
  var p = safe_addr(a)   // p is int?# — safe, no unsafe needed
  print("{*p}\n")

Dereferencing
=============

``*p`` and ``deref(p)`` follow the pointer to the value.
Both panic if the pointer is null::

  unsafe {
      var y = 7
      var p = addr(y)
      print("{*p}\n")         // 7
      print("{deref(p)}\n")   // 7
  }

For struct pointers, ``.`` auto-dereferences — no ``->`` operator::

  var inscope pt = new Point(x = 5.0, y = 6.0)
  print("{pt.x}\n")           // 5 — same as (*pt).x

Null safety
===========

Uninitialized pointers are null.  Dereferencing null panics::

  var np : int?       // null
  try {
      print("{*np}\n")
  } recover {
      print("null deref caught\n")
  }

Use null checks, safe navigation, or null coalescing to handle nullable pointers
safely:

.. code-block:: das

    options gen2

    struct Point {
        x : float
        y : float
    }

    def get_x(p : Point?) : float {
        return p?.x ?? -1.0    // -1.0 if p is null
    }

``?.`` returns null if the pointer is null (no panic).
``??`` provides a fallback value when the left side is null.

Passing pointers to functions
=============================

Pointers can be passed as function arguments.  Functions can read from and
write through them::

  def double_value(p : int?) {
      *p = *p + *p
  }

  var val = 21
  unsafe {
      double_value(addr(val))
  }
  print("{val}\n")  // 42

Struct pointer arguments auto-deref for field access::

  def move_point(p : Point?; dx : float; dy : float) {
      p.x += dx    // auto-deref
      p.y += dy
  }

Deletion
========

``delete`` frees heap memory and sets the pointer to null.
Requires ``unsafe``::

  var p = new Point(x = 1.0, y = 2.0)
  unsafe {
      delete p     // memory freed, p is now null
  }

Prefer ``var inscope`` over manual ``delete`` — it adds a ``finally``
block that automatically cleans up the pointer when the scope exits.

Pointer arithmetic
==================

Pointer indexing and arithmetic are unsafe.  They operate on raw memory
with no bounds checking::

  var arr <- [10, 20, 30, 40, 50]
  unsafe {
      var p = addr(arr[0])
      print("{p[0]}, {p[2]}\n")     // 10, 30

      ++ p            // advance by one element
      print("{*p}\n") // 20

      p += 2           // advance by two more
      print("{*p}\n") // 40
  }

.. warning::

   Pointer arithmetic can easily cause out-of-bounds access.
   No runtime bounds checking is performed.

void pointers
=============

``void?`` is an untyped pointer — equivalent to ``void*`` in C.
Used for C/C++ interop where the actual type is opaque.
Must ``reinterpret`` back to a typed pointer before use::

  unsafe {
      var x = 123
      var px = addr(x)
      var vp : void? = reinterpret<void?> px   // erase type
      var px2 = reinterpret<int?> vp           // restore type
      print("{*px2}\n")                        // 123
  }

intptr
======

``intptr(p)`` converts a pointer to a ``uint64`` integer representing its
memory address.  Useful for debugging, logging, or identity comparisons::

  unsafe {
      var x = 42
      var p = addr(x)
      print("address: {intptr(p)}\n")
  }

reinterpret
===========

``reinterpret<T>`` performs a raw bit cast between types of the same size.
Requires ``unsafe``::

  unsafe {
      let f = 1.0
      let bits = reinterpret<int> f    // IEEE 754: 0x3f800000
      let back = reinterpret<float> bits
      print("{back}\n")                // 1
  }

.. warning::

   ``reinterpret`` does not convert values — it reinterprets raw bits.
   The source and target types must have the same size.

.. seealso::

   :ref:`Pointers <pointers>` — pointer language reference.

   :ref:`Unsafe <unsafe>` — unsafe operations reference.

   :ref:`Values and Data Types <datatypes_and_values>` — all data types including smart pointers.

   Full source: :download:`tutorials/language/36_pointers.das <../../../../tutorials/language/36_pointers.das>`

   Previous tutorial: :ref:`tutorial_jobque`

   Next tutorial: :ref:`tutorial_utility_patterns`
