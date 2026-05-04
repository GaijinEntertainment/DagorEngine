.. _tutorial_linq:

===========================================
LINQ — Language-Integrated Query
===========================================

.. index::
    single: Tutorial; LINQ
    single: Tutorial; linq
    single: Tutorial; linq_boost
    single: Tutorial; Queries
    single: Tutorial; Pipelines

This tutorial covers daslang's LINQ module for C#-style query operations on
iterators and arrays, plus the ``linq_boost`` macros for composing pipelines.

Setup
=====

Import both modules::

  require daslib/linq
  require daslib/linq_boost

Most LINQ functions come in several flavors:

* ``func(iterator, ...)`` — returns a lazy iterator
* ``func(array, ...)`` — returns a new array
* ``func_to_array(iterator, ...)`` — materializes into an array
* ``func_inplace(var array, ...)`` — mutates in place

Filtering
=========

``where_`` filters elements by a predicate (the trailing underscore avoids
collision with the built-in keyword)::

  var numbers = [3, 1, 4, 1, 5, 9, 2, 6, 5]
  var evens = where_to_array(numbers.to_sequence(), $(x) => x % 2 == 0)
  // evens: [4, 2, 6]

The ``_where`` shorthand uses ``_`` as the element placeholder::

  var big = _where_to_array(numbers.to_sequence(), _ > 4)
  // big: [5, 9, 6, 5]

Projection
==========

``select`` transforms each element::

  var doubled = _select_to_array(numbers.to_sequence(), _ * 2)
  // doubled: [6, 2, 8, 2, 10, 18, 4, 12, 10]

``select_many`` flattens nested sequences into one::

  var nested = [
      ["a", "b", "c"].to_sequence(),
      ["d", "e", "f"].to_sequence()
  ]
  var flat = select_many_to_array(
      nested.to_sequence(),
      $(s : string) => "{s}!"
  )
  // flat: ["a!", "b!", "c!", "d!", "e!", "f!"]

Sorting
=======

``order`` sorts ascending, ``order_descending`` sorts descending::

  var unsorted = [5, 3, 8, 1, 4]
  var asc = order(unsorted)           // [1, 3, 4, 5, 8]
  var desc = order_descending(unsorted) // [8, 5, 4, 3, 1]

Sort structs by a field with ``_order_by``::

  var by_age = _order_by(team.to_sequence(), _.age)

Partitioning
============

* ``skip(arr, n)`` / ``take(arr, n)`` — from the front
* ``skip_last(arr, n)`` / ``take_last(arr, n)`` — from the end
* ``skip_while`` / ``take_while`` — by predicate
* ``chunk(iter, n)`` — split into groups of N

::

  var seq = [for (x in 0..8); x]
  skip(seq, 3)        // [3, 4, 5, 6, 7]
  take(seq, 3)        // [0, 1, 2]
  take_last(seq, 3)   // [5, 6, 7]
  skip_last(seq, 3)   // [0, 1, 2, 3, 4]

Aggregation
===========

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Function
     - Description
   * - ``count(arr)``
     - Number of elements
   * - ``count(arr, pred)``
     - Number of elements matching predicate
   * - ``sum(iter)``
     - Sum of elements
   * - ``average(iter)``
     - Arithmetic mean
   * - ``min(iter)`` / ``max(iter)``
     - Minimum / maximum
   * - ``aggregate(iter, seed, func)``
     - Custom fold with seed

::

  var vals = [10, 20, 30, 40, 50]
  count(vals)                          // 5
  count(vals, $(v) => v > 25)          // 3
  sum(vals.to_sequence())              // 150
  aggregate(vals.to_sequence(), 1, $(acc, v) => acc * v)  // 12000000

The ``_count`` shorthand works like ``_where``::

  _count(vals.to_sequence(), _ >= 30)  // 3

Element access
==============

* ``first`` / ``last`` — first or last element (panics if empty)
* ``first_or_default`` / ``last_or_default`` — returns default if empty
* ``element_at(iter, i)`` / ``element_at_or_default(iter, i)``
* ``single`` — returns the only element (panics if count ≠ 1)

Querying
========

::

  any(vals.to_sequence(), $(v) => v > 40)   // true
  all(vals.to_sequence(), $(v) => v > 5)    // true
  contains(vals.to_sequence(), 30)           // true

Set operations
==============

::

  var a = [1, 2, 2, 3, 3, 3]
  var b = [2, 3, 4, 5]
  distinct(a)     // [1, 2, 3]
  union(a, b)     // [1, 2, 3, 4, 5]
  except(a, b)    // [1]
  intersect(a, b) // [2, 3]

Zip
===

``zip`` merges two or three sequences into tuples::

  var names = ["Alice", "Bob", "Charlie"]
  var ages  = [25, 35, 30]
  var zipped = zip(names, ages)
  // zipped: [(Alice,25), (Bob,35), (Charlie,30)]

  var scores = [95, 87, 91]
  var zipped3 = zip(names, ages, scores)
  // zipped3: [(Alice,25,95), (Bob,35,87), (Charlie,30,91)]

The ``_fold`` pipeline macro
============================

``_fold`` composes multiple LINQ operations into an optimized pipeline
using dot-chaining::

  // Sum of squares of even numbers
  var result = (
      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      ._where(_ % 2 == 0)
      ._select(_ * _)
      .sum()
      ._fold()
  )
  // result: 220

  // Top 3 unique values, descending
  var top3 = (
      [5, 3, 8, 1, 4, 8, 3, 9, 2, 9]
      .order_descending()
      .distinct()
      .take(3)
      ._fold()
  )
  // top3: [9, 8, 5]

The ``_fold`` macro rewrites the chain into efficient imperative code at
compile time, avoiding intermediate allocations.

.. seealso::

   Full source: :download:`tutorials/language/28_linq.das <../../../../tutorials/language/28_linq.das>`

   Next tutorial: :ref:`tutorial_functional`
