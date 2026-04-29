.. _tutorial_error_handling:

========================
Error Handling
========================

.. index::
    single: Tutorial; Error Handling
    single: Tutorial; Panic
    single: Tutorial; Try/Recover
    single: Tutorial; Assert
    single: Tutorial; Defer

This tutorial covers daslang's error-handling mechanisms: ``panic`` for
fatal errors, ``try``/``recover`` for catching panics, assertions for
invariants, and ``defer`` for cleanup actions.

panic
=====

``panic("message")`` halts execution immediately with a runtime error.
Use it for unrecoverable situations::

  panic("something went horribly wrong")

Certain operations (null dereference, out-of-bounds access) cause
implicit panics.

try / recover
=============

``try``/``recover`` catches panics and continues execution. It is **not**
general exception handling — only panics are caught::

  try {
      risky_operation()
  } recover {
      print("recovered from panic\n")
  }

.. note::

   You cannot ``return`` from inside ``try`` or ``recover`` blocks.
   Assign results to a variable instead.

assert and verify
=================

``assert(condition, "message")`` checks a condition at runtime. It may
be stripped in release builds::

  assert(x > 0, "x must be positive")

``verify(condition, "message")`` is never stripped — side effects in the
condition are always evaluated::

  verify(initialize(), "init failed")

static_assert and concept_assert
=================================

``static_assert`` checks conditions at compile time::

  static_assert(typeinfo sizeof(type<int>) == 4, "expected 4-byte int")

``concept_assert`` works like ``static_assert`` but reports the error
at the **caller's** line — useful inside generic functions::

  def sum(arr : auto(TT)[]) {
      concept_assert(typeinfo is_numeric(type<TT>), "need numeric type")
  }

finally and defer
=================

``finally`` runs cleanup code when a block exits::

  for (i in range(3)) {
      total += i
  } finally {
      print("loop done\n")
  }

``defer`` (from ``daslib/defer``) schedules cleanup that runs when the
enclosing scope exits. Multiple defers run in LIFO order::

  require daslib/defer

  def example {
      defer() { print("third\n") }
      defer() { print("second\n") }
      defer() { print("first\n") }
  }
  // prints: first, second, third (reverse order)

.. seealso::

   :ref:`Statements <statements>`, :ref:`Unsafe <unsafe>` in the
   language reference.

   Full source: :download:`tutorials/language/21_error_handling.das <../../../../tutorials/language/21_error_handling.das>`

   Next tutorial: :ref:`tutorial_unsafe`
