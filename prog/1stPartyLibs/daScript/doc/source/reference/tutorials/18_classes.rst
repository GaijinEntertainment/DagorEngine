.. _tutorial_classes:

============================
Classes and Inheritance
============================

.. index::
    single: Tutorial; Classes
    single: Tutorial; Inheritance
    single: Tutorial; OOP
    single: Tutorial; Virtual Methods
    single: Tutorial; Super

This tutorial covers defining classes, constructors, ``super()`` and
``super.method()``, abstract and virtual methods, inheritance with
``override``, polymorphism, private members, and static fields/methods.

Defining a class
================

Classes are like structs with virtual methods and inheritance::

  class Shape {
      name : string

      def Shape(n : string) {
          name = n
      }

      def abstract area : float

      def describe {
          print("{name}: area = {area()}\n")
      }
  }

Inheritance and super
=====================

Single-parent inheritance with ``class Derived : Base``.
Use ``super()`` to call the parent constructor::

  class Circle : Shape {
      radius : float

      def Circle(r : float) {
          super("Circle")       // calls Shape`Shape(self, "Circle")
          radius = r
      }

      def override area : float {
          return 3.14159 * radius * radius
      }
  }

Use ``super.method()`` to call the parent's version of a method,
bypassing virtual dispatch::

  class Rectangle : Shape {
      width : float
      height : float

      def Rectangle(w, h : float) {
          super("Rectangle")
          width = w
          height = h
      }

      def override describe {
          super.describe()      // calls Shape`describe(self)
          print("  (w={width}, h={height})\n")
      }
  }

The compiler rewrites ``super()`` to ``Parent`Constructor(self, ...)`` and
``super.method()`` to ``Parent`method(self, ...)``.

Creating instances
==================

Use ``new`` to create a class on the heap::

  var c = new Circle(5.0)
  c.describe()

Polymorphism
============

Base pointers hold any derived instance. Method calls dispatch virtually::

  var shapes : array<Shape?>
  shapes |> push(new Circle(3.0))
  shapes |> push(new Rectangle(2.0, 5.0))

  for (s in shapes) {
      s.describe()    // calls the correct area() for each type
  }

Private members
===============

::

  class Counter {
      private count : int = 0
      def increment { count += 1 }
      def get_count : int { return count }
  }

Static fields and methods
=========================

``static`` fields are shared across all instances.
``def static`` methods access static fields but not ``self``::

  class Tracker {
      static total : int = 0
      id : int

      def Tracker {
          total += 1
          id = total
      }

      def static getTotal : int {
          return total
      }

      def static reset {
          total = 0
      }
  }

Call static methods with backtick syntax::

  print("{Tracker`getTotal()}\n")
  Tracker`reset()

Key concepts
============

- ``abstract`` — subclasses **must** implement
- ``override`` — replaces a parent method
- ``sealed`` — prevents further overriding or inheritance
- ``self`` — implicit pointer to current instance
- ``super()`` — calls parent constructor
- ``super.method()`` — calls parent's version of a method
- ``def static`` — method without self, accesses static fields only
- ``ClassName`method()`` — call static methods from outside

.. note::

   Structs can also have methods via the ``[class_method]`` annotation
   from ``daslib/class_boost``, which adds an implicit ``self`` parameter
   to static functions on structs.

.. seealso::

   :ref:`Classes <classes>`, :ref:`Structs <structs>` in the language reference.

   Full source: :download:`tutorials/language/18_classes.das <../../../../tutorials/language/18_classes.das>`

Next tutorial: :ref:`tutorial_generics`
