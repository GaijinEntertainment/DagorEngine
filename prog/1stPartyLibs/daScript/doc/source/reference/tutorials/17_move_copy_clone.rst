.. _tutorial_move_copy_clone:

========================
Move, Copy, and Clone
========================

.. index::
    single: Tutorial; Move
    single: Tutorial; Copy
    single: Tutorial; Clone
    single: Tutorial; Assignment

This tutorial covers the three assignment operators: copy (``=``), move
(``<-``), and clone (``:=``), when to use each, and how types determine
operator availability.

Copy (=)
========

Bitwise copy. Source unchanged. Works for POD types (int, float, string)
and POD-only structs::

  var a = 42
  var b = a         // b is 42, a is still 42

Move (<-)
=========

Transfers ownership. Source is zeroed. Use for containers (array, table),
lambdas, and iterators::

  var nums <- [1, 2, 3]
  var other <- nums       // other owns the array, nums is empty

Functions returning containers must use ``return <-``::

  def make_data() : array<int> {
      var result : array<int>
      result |> push(1)
      return <- result
  }

Clone (:=)
==========

Deep copy. Both sides remain valid and independent::

  var original : array<int>
  original |> push(1)
  original |> push(2)

  var copy : array<int>
  copy := original        // deep copy
  copy |> push(999)       // does not affect original

Clone initialization::

  var another := original

Type compatibility
==================

.. list-table::
   :header-rows: 1
   :widths: 30 15 15 15

   * - Type
     - ``=``
     - ``<-``
     - ``:=``
   * - int, float, POD
     - Yes
     - Yes
     - Yes
   * - string
     - Yes
     - Yes
     - Yes
   * - array, table
     - No
     - Yes
     - Yes
   * - lambda
     - No
     - Yes
     - No
   * - iterator
     - No
     - Yes
     - No
   * - block
     - No
     - No
     - No

Relaxed assign
==============

By default, the compiler auto-promotes ``=`` to ``<-`` when the right side
is temporary (literal, function return). Disable with
``options relaxed_assign = false``.

Struct field initialization
===========================

Each field can use a different mode::

  var weapons : array<string>
  weapons |> push("bow")

  var loadout = Inventory(items <- weapons, weight = 5)
  // weapons is now empty after the move

Custom clone functions
======================

Override clone behavior by defining a ``clone`` function for your type::

  struct GameState {
      level : int
      score : int
  }

  def clone(var dst : GameState; src : GameState) {
      dst.level = src.level + 1000   // custom logic during clone
      dst.score = src.score
  }

  var original = GameState(level=5, score=100)
  var copy := original    // invokes custom clone
  // copy.level is 1005, not 5

.. seealso::

   :ref:`Move, Copy, and Clone <move_copy_clone>` in the language reference.

   Full source: :download:`tutorials/language/17_move_copy_clone.das <../../../../tutorials/language/17_move_copy_clone.das>`

Next tutorial: :ref:`tutorial_classes`
