.. _tutorial_generics:

========================
Generic Programming
========================

.. index::
    single: Tutorial; Generics
    single: Tutorial; Templates
    single: Tutorial; typeinfo
    single: Tutorial; static_if
    single: Tutorial; Specialization

This tutorial covers generic functions (omitting types / using ``auto``),
named type variables with ``auto(T)``, using ``auto(T)`` for local
variables, specialization from generic to specific, compile-time type
inspection with ``typeinfo``, and conditional compilation with
``static_if``.

Generic functions
=================

Omit a type to make a function generic. It is instantiated for each
concrete type at the call site::

  def print_value(v) {
      print("value = {v}\n")
  }

  print_value(42)       // instantiated for int
  print_value(3.14)     // instantiated for float
  print_value("hello")  // instantiated for string

Named auto
==========

Use ``auto(T)`` to name a type variable. Multiple parameters sharing the
same name must have the same type::

  def add(a, b : auto(T)) : T {
      return a + b
  }

  add(10, 20)       // ok: both int
  add(1.5, 2.5)     // ok: both float
  // add(1, 2.0)    // error: int != float

auto(T) for local variables
============================

Use the named type ``T`` to declare local variables of the same type::

  def make_pair(a, b : auto(T)) {
      var first : T = a
      var second : T = b
      print("pair: {first}, {second}\n")
  }

  make_pair(10, 20)           // T = int
  make_pair("hello", "world") // T = string

default<T>
==========

Use ``default<T>`` to create a default-initialized value of any type.
This is useful in generic code where you don't know the concrete type::

  def sum_array(arr : array<auto(T)>) : T {
      var total = default<T>    // 0 for int, 0.0 for float, "" for string
      for (v in arr) {
          total += v
      }
      return total
  }

Specialization
==============

When multiple overloads match, the compiler picks the **most specific**
one. This lets you write a generic fallback and optimize for known types::

  // Most generic: takes anything
  def process(v) {
      print("anything: {v}\n")
  }

  // More specific: any array (T is the element type)
  def process(arr : array<auto(T)>) {
      print("array of {typeinfo typename(type<T>)}\n")
  }

  // Most specific: array<int> (exact match)
  def process(arr : array<int>) {
      var total = 0
      for (v in arr) { total += v }
      print("array<int>, sum={total}\n")
  }

Calling these::

  process(42)           // → anything
  process(["a", "b"])   // → array of string
  process([1, 2, 3])    // → array<int>

typeinfo
========

Query types at compile time::

  def describe_type(v : auto(T)) {
      let name = typeinfo typename(type<T>)
      let size = typeinfo sizeof(type<T>)
      print("type={name}, size={size}\n")
  }

Common typeinfo traits:

- ``typeinfo typename(expr)`` — human-readable type name
- ``typeinfo sizeof(expr)`` — size in bytes
- ``typeinfo is_numeric(expr)`` — true for int, float, etc.
- ``typeinfo is_string(expr)`` — true for string
- ``typeinfo is_array(expr)`` — true for ``array<T>``
- ``typeinfo has_field<name>(expr)`` — true if struct has field

static_if
=========

Select code paths based on type traits — resolved at compile time::

  def to_string_generic(v : auto(T)) : string {
      static_if (typeinfo is_numeric(type<T>)) {
          return "number: {v}"
      } else {
          return "other: {v}"
      }
  }

Struct introspection
====================

Use ``has_field`` to write functions that work on multiple struct types::

  def get_name(obj) : string {
      static_if (typeinfo has_field<name>(obj)) {
          return obj.name
      } else {
          return "unnamed"
      }
  }

.. seealso::

   :ref:`Generic Programming <generic_programming>` in the language reference.

   Full source: :download:`tutorials/language/19_generics.das <../../../../tutorials/language/19_generics.das>`

Next tutorial: :ref:`tutorial_lifetime`
