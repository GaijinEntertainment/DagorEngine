.. _tutorial_pattern_matching:

========================
Pattern Matching
========================

.. index::
    single: Tutorial; Pattern Matching
    single: Tutorial; Match
    single: Tutorial; Variable Binding

This tutorial covers daslang's pattern matching with the ``match``
macro from ``daslib/match``.

Basic match
===========

``match(expr) { ... }`` dispatches on values. Each arm is an ``if``::

  require daslib/match

  def describe(n : int) : string {
      match (n) {
          if (0) { return "zero" }
          if (1) { return "one" }
          if (_) { return "other" }
      }
      return "unreachable"
  }

.. note::

   ``match`` works on functions that return values.

Wildcards and binding
=====================

- ``_`` — wildcard, matches anything
- ``$v(name)`` — bind matched value to a variable::

    match (point) {
        if (Point(x = 0, y = 0)) { return "origin" }
        if (Point(x = $v(x), y = $v(y))) { return "({x}, {y})" }
    }

Guards
======

Add ``&&`` after a pattern for extra conditions::

  match (shape) {
      if (Shape(size = $v(sz)) && sz > 100) {
          return "large"
      }
  }

OR patterns
===========

Match multiple alternatives with ``||``::

  match (color) {
      if (Color.red || Color.green || Color.blue) {
          return true
      }
  }

Variant matching
================

Use ``$v(name) as alternative`` to match variant alternatives::

  match (value) {
      if ($v(i) as i) { return "int" }
      if ($v(f) as f) { return "float" }
  }

Tuple matching
==============

Match tuple elements positionally::

  match (pair) {
      if ((0, "zero")) { return "exact" }
      if ((1, _)) { return "starts with one" }
      if (($v(n), "hello")) { return "hello #{n}" }
  }

Array matching
==============

Static arrays match element-by-element. Dynamic arrays match head
elements; use ``...`` to ignore the tail::

  match (sa) {
      if (fixed_array<int>(0, 0, 0)) { return "zeros" }
      if (fixed_array<int>(1, $v(b), $v(c))) { return "1,{b},{c}" }
  }

  match (da) {
      if (array<int>(0, 0, ...)) { return "starts with 0,0" }
      if (array<int>($v(x), $v(y), ...)) { return "starts with {x},{y}" }
  }

match_expr — computed patterns
===============================

``match_expr(expression)`` evaluates at runtime instead of matching
literally. Useful when the expected value depends on a captured variable::

  match (t) {
      if (($v(a), match_expr(a + 1), match_expr(a + 2))) {
          return true   // matches consecutive triples
      }
  }

Bool matching
=============

``match`` works on booleans::

  match (b) {
      if (true) { return "yes" }
      if (false) { return "no" }
  }

multi_match and static_match
=============================

- ``multi_match`` — all matching arms execute (not just first)
- ``static_match`` — silently drops type-mismatched arms (for generics)

::

  multi_match (n) {
      if ($v(a) && a > 0) { tags += " positive" }
      if ($v(a) && (a % 2 == 0)) { tags += " even" }
  }

.. seealso::

   :ref:`Pattern matching <pattern-matching>` in the language reference.

   Full source: :download:`tutorials/language/24_pattern_matching.das <../../../../tutorials/language/24_pattern_matching.das>`

   Next tutorial: :ref:`tutorial_annotations`
