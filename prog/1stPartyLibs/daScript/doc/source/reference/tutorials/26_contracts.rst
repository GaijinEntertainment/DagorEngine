.. _tutorial_contracts:

========================
Contracts
========================

.. index::
    single: Tutorial; Contracts
    single: Tutorial; Generic Constraints
    single: Tutorial; concept_assert

This tutorial covers contract annotations from ``daslib/contracts``
for constraining generic function arguments at compile time.

What are contracts?
===================

Contracts are compile-time constraints on generic function parameters.
When a generic function is called, the contract checks whether each
argument satisfies the type constraint. If not, the function is skipped
during overload resolution.

Basic usage
===========

::

  require daslib/contracts

  [expect_any_numeric(a), expect_any_numeric(b)]
  def safe_add(a, b) {
      return a + b
  }
  // safe_add(3, 4)       — works
  // safe_add("a", "b")   — no match, not numeric

Built-in contracts
==================

Always available (no import needed):

- ``[expect_ref(arg)]`` — must be a reference
- ``[expect_dim(arg)]`` — must be a fixed-size array
- ``[expect_any_vector(arg)]`` — must be a C++ vector type

Library contracts
=================

From ``daslib/contracts``:

- ``[expect_any_array(a)]`` — ``array<T>`` or ``T[N]``
- ``[expect_any_numeric(a)]`` — ``int``, ``float``, etc.
- ``[expect_any_enum(a)]`` — any enumeration
- ``[expect_any_struct(a)]`` — any struct (not class)
- ``[expect_any_tuple(a)]`` — any tuple
- ``[expect_any_variant(a)]`` — any variant
- ``[expect_any_function(a)]`` — any function type
- ``[expect_any_lambda(a)]`` — any lambda
- ``[expect_any_bitfield(a)]`` — any bitfield
- ``[expect_pointer(a)]`` — any pointer
- ``[expect_class(a)]`` — class pointer

Combining contracts
===================

Use logical operators inside the brackets::

  [expect_any_numeric(a) && expect_any_numeric(b)]   // AND
  [expect_any_tuple(x) || expect_any_variant(x)]     // OR
  [!expect_any_numeric(x)]                            // NOT
  [expect_any_tuple(a) ^^ expect_any_variant(b)]     // XOR

concept_assert
==============

``concept_assert(condition, "message")`` is a compile-time check that
reports errors at the **caller's** line (not inside the generic)::

  def sum_all(arr : array<auto(TT)>) : TT {
      concept_assert(typeinfo is_numeric(type<TT>),
          "sum_all requires numeric elements")
      // ...
  }

.. seealso::

   :ref:`Generic programming <generic_programming>` in the
   language reference.

   Full source: :download:`tutorials/language/26_contracts.das <../../../../tutorials/language/26_contracts.das>`

   Next tutorial: :ref:`tutorial_testing`
