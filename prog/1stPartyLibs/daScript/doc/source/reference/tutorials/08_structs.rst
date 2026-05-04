.. _tutorial_structs:

========================
Structs
========================

.. index::
    single: Tutorial; Structs

This tutorial covers declaring structs, field defaults, gen2 construction
syntax, nested structs, heap allocation with ``new``, and the ``with`` block.

Declaring a struct
==================

A struct defines a named collection of typed fields.  Fields can have default
values::

  struct Point {
      x : float = 0.0
      y : float = 0.0
  }

  struct Color {
      r : uint8
      g : uint8
      b : uint8
      a : uint8 = uint8(255)
  }

Construction
============

In gen2 syntax, construct structs with ``StructName(field=value, ...)``::

  var p1 = Point()                // all defaults
  var p2 = Point(x=3.0, y=4.0)   // named fields

Partial initialization fills unspecified fields with their defaults::

  struct Config {
      width : int = 800
      height : int = 600
      title : string = "My App"
      fullscreen : bool = false
  }

  var cfg = Config(title="Tutorial")
  // width=800, height=600, fullscreen=false

Structs are pure data
=====================

daslang structs have no methods.  Write free functions that take a struct as a
parameter::

  def area(r : Rect) : float {
      let w = r.bottomRight.x - r.topLeft.x
      let h = r.bottomRight.y - r.topLeft.y
      return w * h
  }

Pass by mutable reference with ``var`` and ``&`` to modify a struct
in place::

  def translate(var p : Point&; dx, dy : float) {
      p.x += dx
      p.y += dy
  }

Nested structs
==============

Struct fields can be other structs::

  struct Rect {
      topLeft : Point = Point()
      bottomRight : Point = Point()
  }

The ``with`` block
==================

Inside a ``with`` block, you can access struct fields without repeating the
variable name::

  var hero = Point()
  with (hero) {
      x = 100.0
      y = 200.0
  }

This is especially convenient for structs with many fields.

Heap allocation
===============

``new`` creates a heap-allocated struct and returns a pointer (``Point?``)::

  var p3 = new Point(x=7.0, y=8.0)
  print("({p3.x}, {p3.y})\n")

  // p3 will be garbage collected when no longer referenced.
  // For explicit cleanup, see the tutorial on delete and inscope.

Running the tutorial
====================

::

  daslang.exe tutorials/language/08_structs.das

Full source: :download:`tutorials/language/08_structs.das <../../../../tutorials/language/08_structs.das>`

See also
========

* **Next:** :ref:`tutorial_enumerations` — enumerations and bitfields
* :ref:`Structs <structs>` — full struct reference
* :ref:`Classes <classes>` — structs with virtual methods
