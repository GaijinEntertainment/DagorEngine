.. _tutorial_iterators_and_generators:

============================
Iterators and Generators
============================

.. index::
    single: Tutorial; Iterators
    single: Tutorial; Generators
    single: Tutorial; Yield

This tutorial covers built-in iterators (``range``, ``each``, ``keys``,
``values``), creating generators with ``yield``, and making custom types
iterable.

Built-in iterators
==================

Arrays, ranges, tables, and strings are all iterable::

  for (i in range(5)) { ... }
  for (v in my_array) { ... }
  for (k, v in keys(table), values(table)) { ... }

Generators
==========

A generator lazily produces values one at a time using ``yield``.
Generators use ``$`` (either ``$ { }`` or ``$() { }``)::

  var counter <- generator<int>() <| $ {
      for (i in range(5)) {
          yield i
      }
      return false
  }

  for (v in counter) {
      print("{v} ")       // 0 1 2 3 4
  }

Generators maintain state between yields — local variables keep their
values across calls.

Generator patterns
==================

**Filtering** — yield only matching values::

  var evens <- generator<int>() <| $ {
      for (i in range(10)) {
          if (i % 2 == 0) { yield i }
      }
      return false
  }

**Stateful** — maintain running computations::

  var fibs <- generator<int>() <| $ {
      var a = 0
      var b = 1
      while (a < 100) {
          yield a
          let next = a + b
          a = b
          b = next
      }
      return false
  }

Custom iterable types
=====================

Define an ``each()`` function to make any struct iterable::

  struct NumberRange {
      low : int
      high : int
  }

  def each(r : NumberRange) : iterator<int> {
      return <- generator<int>() <| $ {
          for (i in r.low .. r.high) {
              yield i
          }
          return false
      }
  }

  var r = NumberRange(low=3, high=8)
  for (i in r) {
      print("{i} ")       // 3 4 5 6 7
  }

.. note::

   Generators are **one-shot** — once exhausted they produce no more values.
   Create a new generator to iterate again.

.. seealso::

   :ref:`Iterators <iterators>`, :ref:`Generators <generators>` in the language reference.

   Full source: :download:`tutorials/language/15_iterators_and_generators.das <../../../../tutorials/language/15_iterators_and_generators.das>`

Next tutorial: :ref:`tutorial_modules`
