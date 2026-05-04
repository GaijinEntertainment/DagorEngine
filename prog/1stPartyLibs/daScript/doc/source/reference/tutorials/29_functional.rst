.. _tutorial_functional:

========================
Functional Programming
========================

.. index::
    single: Tutorial; Functional Programming
    single: Tutorial; Filter
    single: Tutorial; Map
    single: Tutorial; Reduce
    single: Tutorial; Fold
    single: Tutorial; Iterator Combinators

This tutorial covers ``daslib/functional`` — lazy iterator adapters
and higher-order combinators for daslang.

Unlike ``daslib/linq`` which uses blocks and focuses on query syntax,
``functional`` accepts lambdas and functions, emphasising composability.

filter and map
==============

``filter`` keeps elements matching a predicate.
``map`` transforms each element::

  var src <- [iterator for (x in range(8)); x]
  var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })

  var nums <- [iterator for (x in range(5)); x]
  var squared <- map(nums, @(x : int) : int { return x * x; })

Both return lazy iterators — elements are produced on demand.
They accept lambdas (``@``) or named functions (``@@is_even``).

.. note::

   ``filter``, ``map``, ``scan``, and ``tap`` return generators and
   only accept lambdas or functions — **not blocks** — because blocks
   cannot be captured into generators.

reduce and fold
===============

``reduce`` combines elements pairwise. ``fold`` adds an initial seed::

  var total = reduce(src) $(a, b : int) : int { return a + b }

  var product = fold(src, 1) $(acc, x : int) : int { return acc * x }

``reduce_or_default`` returns a fallback on empty input instead of panicking::

  var safe = reduce_or_default(src, @(a, b : int) : int { return a + b; }, -1)

scan — running fold
===================

``scan`` yields every intermediate accumulator value::

  var running <- scan(src, 0, @(acc, x : int) : int { return acc + x; })
  // seed=0, then 0+1=1, 1+3=4, ...

Iterator combinators
====================

- ``chain(a, b)`` — concatenate two iterators
- ``enumerate(src)`` — ``(index, element)`` tuples
- ``pairwise(src)`` — consecutive pairs: ``(a,b), (b,c), ...``
- ``islice(src, start, stop)`` — slice ``[start, stop)``
- ``flatten(src)`` — flatten nested iterators one level

::

  var en <- enumerate(names)
  for (v in en) { print("{v._0}: {v._1}") }

  var ca <- chain(first_half, second_half)

Generators
==========

- ``iterate(seed, fn)`` — ``seed, f(seed), f(f(seed)), ...`` infinitely
- ``repeat(val, n)`` — repeat ``val`` ``n`` times (or forever if ``n`` < 0)
- ``cycle(src)`` — endlessly repeat an iterator

::

  var powers <- iterate(1, @(x : int) : int { return x * 2; })
  // 1, 2, 4, 8, 16, ...

  var rep <- repeat(42, 3)   // 42, 42, 42

Aggregation
===========

- ``sum(src)`` — sum all elements
- ``any(src)`` — true if any element is truthy
- ``all(src)`` — true if all elements are truthy

Search and split
================

- ``find(src, fn, default)`` — first match or default value
- ``find_index(src, fn)`` — index of first match, or ``-1``
- ``partition(src, fn)`` — split into ``(matching, non_matching)`` arrays

::

  var first_even = find(src, @@is_even, -1)
  var parts = partition(src, @@is_even)  // parts._0 = evens, parts._1 = odds

Side-effects and debugging
==========================

- ``for_each(src, fn/block)`` — apply side-effect to every element
- ``tap(src, fn)`` — passthrough with side-effect (debug tool)
- ``echo(x)`` — print ``x`` and return it unchanged

::

  for_each(src) $(x : int) { total += x }

  var tapped <- tap(src, @(x : int) { print("debug: {x}\n"); })

Composition
===========

Chain these together to build data pipelines::

  var big <- filter(src, @(x : int) : bool { return x >= 5; })
  var doubled <- map(big, @(x : int) : int { return x * 2; })
  var result = fold(doubled, 0) $(acc, x : int) : int { return acc + x }

.. seealso::

   Full source: :download:`tutorials/language/29_functional.das <../../../../tutorials/language/29_functional.das>`

   Next tutorial: :ref:`tutorial_json`

   :ref:`LINQ tutorial <tutorial_linq>` for block-based query syntax.

   :ref:`Iterators and generators <iterators>` in the language reference.

   :ref:`Lambdas <lambdas>` in the language reference.
