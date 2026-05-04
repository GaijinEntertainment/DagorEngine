.. _tutorial_lambdas:

========================
Lambdas and Closures
========================

.. index::
    single: Tutorial; Lambdas
    single: Tutorial; Closures
    single: Tutorial; Capture
    single: Tutorial; Local Functions

This tutorial covers lambda declarations with ``@()``, capture modes
(copy, move, clone), call syntax, storing lambdas, and how lambdas
differ from blocks and function pointers.

Declaring a lambda
==================

Lambdas are prefixed with ``@()`` and are heap-allocated::

  var mul <- @(x : int) : int { return x * 3 }
  print("{mul(10)}\n")    // 30

Simplified syntax with ``=>``::

  var add <- @(a, b : int) : int => a + b

Call syntax
===========

Lambda variables support **call syntax** — call them like regular
functions::

  var doubler <- @(x : int) : int => x * 2
  print("{doubler(7)}\n")    // 14

``invoke()`` is the explicit alternative — same behavior::

  print("{invoke(doubler, 7)}\n")    // 14

Prefer call syntax; use ``invoke()`` when you need it.

Capture modes
=============

By default, lambdas capture outer variables **by copy**::

  var multiplier = 10
  var fn <- @(x : int) : int { return x * multiplier }
  // changing multiplier here does not affect fn

Use ``capture()`` for other modes:

- ``capture(move(var))`` — transfers ownership; source is zeroed
- ``capture(clone(var))`` — deep copy; source unchanged
- ``capture(ref(var))`` — by reference (requires ``unsafe``)

::

  var data : array<int>
  data |> push(1)
  unsafe {
      var fn2 <- @capture(move(data)) () { print("{data}\n") }
  }

Storing lambdas
===============

Lambdas must be **moved** with ``<-``, not copied::

  var a <- @() { print("hello\n") }
  var b <- a          // a is now empty
  b()

Lambdas cannot be copied, but they **can** be stored in arrays using
``emplace``, which moves the lambda into the container::

  var callbacks : array<lambda<():void>>
  var greet <- @() { print("hello from callback\n") }
  callbacks |> emplace(greet)    // greet is now empty
  var farewell <- @() { print("goodbye from callback\n") }
  callbacks |> emplace(farewell)
  for (cb in callbacks) {
      invoke(cb)
  }

Blocks **cannot** be stored in arrays or variables — they live on the
stack and are only valid as function arguments.

Stateful lambda factory
========================

A function can create and return a lambda that captures state::

  def make_counter() : lambda<():int> {
      var count = 0
      return <- @capture(clone(count)) () : int {
          count += 1
          return count
      }
  }

Each call creates an independent counter.

Lambda lifecycle and finally blocks
=====================================

A lambda is a heap-allocated struct. The full lifecycle is:

1. **Capture** — outer variables are copied/moved/cloned into the struct
2. **Invoke** — the lambda body runs (may be called many times)
3. **Destroy** — when the lambda is deleted or garbage collected:
   a) The ``finally{}`` block runs (user cleanup code)
   b) Captured fields are finalized (compiler-generated ``delete``)
   c) The struct memory is freed

Captured fields are automatically deleted on destruction unless:

- The field was captured **by reference** (not owned)
- The field was captured **by move/clone** with ``doNotDelete``
- The field type is POD (int, float — no cleanup needed)

Finally block
~~~~~~~~~~~~~

A lambda's ``finally{}`` block runs **once** when the lambda is
**destroyed** — not after each invocation.  This is different from
a *block* ``finally{}``, which runs after every call::

  var demo <- @() {
      print("body\n")
  } finally {
      print("destroyed\n")    // runs once, on deletion
  }
  demo()     // prints "body"
  demo()     // prints "body"
  unsafe { delete demo; }   // prints "destroyed"

Use lambda ``finally{}`` for one-time destruction cleanup (releasing
resources, closing handles).  For per-call cleanup, use scoped
variables or ``defer`` inside the lambda body.

Lambda vs block vs function pointer
=====================================

.. list-table::
   :header-rows: 1
   :widths: 20 25 25 30

   * - Feature
     - ``function<>`` (@@)
     - ``lambda<>`` (@)
     - ``block<>`` ($)
   * - Allocation
     - None
     - Heap
     - Stack
   * - Capture
     - None
     - By copy (default)
     - By reference
   * - Storable
     - Yes
     - Yes (move only)
     - No
   * - Returnable
     - Yes
     - Yes
     - No

A ``function<>`` parameter accepts only ``@@`` values.
A ``lambda<>`` parameter accepts only ``@`` values.
A ``block<>`` parameter accepts any of them (most flexible).

See :ref:`tutorial_function_pointers` for ``@@`` and ``function<>`` details.

.. seealso::

   :ref:`Lambdas <lambdas>`, :ref:`Functions <functions>` in the language reference.

   Full source: :download:`tutorials/language/14_lambdas.das <../../../../tutorials/language/14_lambdas.das>`

Next tutorial: :ref:`tutorial_iterators_and_generators`
