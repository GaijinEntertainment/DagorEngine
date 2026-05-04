.. _tutorial_control_flow:

========================
Control Flow
========================

.. index::
    single: Tutorial; Control Flow

This tutorial covers ``if``/``elif``/``else``, ``while`` loops, ``for`` loops
with ranges, ``break``, ``continue``, and iterating over multiple sequences in
parallel.

if / elif / else
================

In gen2 syntax, parentheses around the condition and braces around the body are
required::

  let temperature = 25
  if (temperature > 30) {
      print("It's hot!\n")
  } elif (temperature > 20) {
      print("It's pleasant.\n")
  } elif (temperature > 10) {
      print("It's cool.\n")
  } else {
      print("It's cold!\n")
  }

while loop
==========

::

  var count = 5
  while (count > 0) {
      print("countdown: {count}\n")
      count--
  }

for loop with range
===================

``range(start, end)`` iterates from *start* to *end-1*.
The ``..`` operator is shorthand: ``0..5`` is the same as ``range(0, 5)``::

  for (i in range(0, 5)) {
      print("{i} ")
  }
  // 0 1 2 3 4

  for (i in 0..5) {
      print("{i} ")
  }
  // 0 1 2 3 4

Nested loops
============

::

  for (row in 1..4) {
      for (col in 1..4) {
          let product = row * col
          print("{product}\t")
      }
      print("\n")
  }

break and continue
===================

``break`` exits the innermost loop.  ``continue`` skips to the next
iteration::

  for (i in 0..10) {
      if (i == 3) {
          break
      }
      print("{i} ")
  }
  // 0 1 2

  for (i in 0..8) {
      if (i % 2 != 0) {
          continue
      }
      print("{i} ")
  }
  // 0 2 4 6

Multiple iterators
==================

``for`` can iterate over multiple sequences in parallel.  The loop stops when
the shortest sequence ends::

  let names = fixed_array("Alice", "Bob", "Carol")
  let scores = fixed_array(95, 87, 92)
  for (name, score in names, scores) {
      print("{name}: {score}\n")
  }

Pair a finite sequence with ``count()`` to get an index::

  let fruits = fixed_array("apple", "banana", "cherry")
  for (idx, fruit in count(), fruits) {
      print("  [{idx}] {fruit}\n")
  }

``count()`` is a built-in infinite iterator that produces 0, 1, 2, ...
You can also pass a start and step: ``count(10, 2)`` produces 10, 12, 14, ...

Running the tutorial
====================

::

  daslang.exe tutorials/language/04_control_flow.das

Full source: :download:`tutorials/language/04_control_flow.das <../../../../tutorials/language/04_control_flow.das>`

See also
========

* **Next:** :ref:`tutorial_functions` — functions
* :ref:`Statements <statements>` — full statement reference
* :ref:`Iterators <iterators>` — iterator mechanics
