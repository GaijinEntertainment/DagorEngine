.. _tutorial_tables:

========================
Tables
========================

.. index::
    single: Tutorial; Tables
    single: Tutorial; Dictionaries

This tutorial covers declaring tables, inserting and erasing entries, safe
lookup with ``?[]``, block-based access with ``get()``, iteration, table
comprehensions, and key-only tables (sets).

Declaring a table
=================

A table maps keys to values.  Note the **semicolon** between key and value
types::

  var ages : table<string; int>

Inserting values
================

Use ``insert`` with pipe syntax::

  ages |> insert("Alice", 30)
  ages |> insert("Bob", 25)
  ages |> insert("Carol", 35)

Inline construction uses ``{ key => value }`` with commas::

  var scores <- { "math" => 95, "english" => 87, "science" => 92 }

Safe lookup
===========

``?[]`` returns the value if the key exists, falling back to the value after
``??``. It does **not** insert missing keys::

  let age = ages?["Alice"] ?? -1     // 30
  let unknown = ages?["Dave"] ?? -1  // -1

Use ``key_exists`` to test for a key::

  key_exists(ages, "Alice")    // true
  key_exists(ages, "Dave")     // false

.. note::

   The plain ``[]`` operator on a table **inserts** a default entry if the key
   is missing and requires ``unsafe``.  Prefer ``?[]`` or ``get()`` for safe
   lookups.

Block-based lookup
==================

``get()`` runs a block only if the key is found::

  get(ages, "Bob") $(val) {
      print("Bob is {val} years old\n")
  }

Pass ``var`` in the block parameter to modify the value in-place::

  get(scores, "math") $(var val) {
      val = 100
  }

Erasing
=======

::

  ages |> erase("Bob")

Iteration
=========

Use ``keys()`` and ``values()`` as parallel iterators::

  for (subject, score in keys(scores), values(scores)) {
      total += score
  }

.. note::

   Tables are hash-based — **iteration order is not guaranteed**.

Table comprehensions
====================

Build a table from a ``for`` expression::

  var squareMap <- { for (x in 1..6); x => x * x }

Key-only tables (sets)
======================

A ``table<KeyType>`` with no value type acts like a set::

  var seen : table<string>
  seen |> insert("apple")
  seen |> insert("banana")
  seen |> insert("apple")    // no effect — already present

  key_exists(seen, "apple")  // true

Safety notes
============

1. ``tab[key]`` **inserts** a default value when the key is missing.  It
   requires ``unsafe``.  Use ``?[]`` or ``get()`` instead.

2. Never write ``tab[k1] = tab[k2]`` — the first ``[]`` may resize the table,
   invalidating the reference from the second ``[]``.

3. Tables cannot be copied with ``=`` — use ``<-`` (move) or ``:=`` (clone).

Running the tutorial
====================

::

  daslang.exe tutorials/language/10_tables.das

Full source: :download:`tutorials/language/10_tables.das <../../../../tutorials/language/10_tables.das>`

See also
========

* :ref:`Tables <tables>` — full table reference
* :ref:`Comprehensions <comprehensions>` — comprehension syntax

Next tutorial: :ref:`tutorial_tuples_and_variants`
