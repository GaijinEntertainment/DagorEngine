.. _tutorial_macro_for_loop_macro:

.. index::
   single: Tutorial; Macros; For-Loop Macro
   single: Tutorial; Macros; for_loop_macro
   single: Tutorial; Macros; AstForLoopMacro
   single: Tutorial; Macros; table iteration

====================================================
 Macro Tutorial 9: For-Loop Macros
====================================================

Previous tutorials transformed calls, functions, structures, blocks, and
variants.  For-loop macros operate on **for-loop expressions** — they
intercept ``for (... in ...)`` at compile time and can rewrite the entire
loop before type inference finalises.

``[for_loop_macro(name="X")]`` registers a class that extends
``AstForLoopMacro``.  The compiler calls the macro's single method
after each for-loop's sources have been type-checked:

``visitExprFor(prog, mod, expr)``
   Called after visiting the for-loop body, during type inference.
   Source types are resolved.  Return a replacement ``ExpressionPtr``
   to transform the loop;  return ``default<ExpressionPtr>`` to skip.


Motivation
==========

daslang tables (``table<K;V>``) are not directly iterable.  The standard
idiom to iterate a table requires the verbose ``keys()`` and ``values()``
built-in functions:

.. code-block:: das

   for (k, v in keys(tab), values(tab)) {
       print("{k} => {v}\n")
   }

This tutorial builds a for-loop macro that supports a much more natural
tuple-destructuring syntax:

.. code-block:: das

   for ((k,v) in tab) {       // rewrites to keys/values automatically
       print("{k} => {v}\n")
   }

The ``(k,v)`` syntax is **already supported by the parser** — it
produces a backtick-joined iterator name (``k`v``) and sets a
tuple-expansion flag.  All the macro needs to do is detect a table
source, split the name, and rewrite the loop.

.. note::

   Macros cannot be used in the module that defines them.  This tutorial
   has **two** source files: a *module* file containing the macro
   definition and a *usage* file that requires the module and exercises it.


The macro module
================

Prerequisites
-------------

::

  require daslib/ast                // AST node types (ExprFor, ExprCall, etc.)
  require daslib/ast_boost   // AstForLoopMacro base class, [for_loop_macro]
  require strings            // find, slice — for splitting the iterator name


Step 1 — Registration
---------------------

.. literalinclude:: ../../../../../tutorials/macros/for_loop_macro_mod.das
   :language: das
   :lines: 15-18

``[for_loop_macro(name=table_kv)]`` registers ``TableKVForLoop`` so the
compiler calls ``visitExprFor`` for every for-loop in modules that
``require`` this one.


Step 2 — Detect a table source with tuple expansion
----------------------------------------------------

Each for-loop's ``ExprFor`` node has several parallel vectors:

``sources``
   Source expressions (the ``in`` parts).
``iterators``
   Iterator variable names.
``iteratorsAt``
   Source locations for each iterator.
``iteratorsAka``
   Alias names (from ``aka``).
``iteratorsTags``
   Tag expressions.
``iteratorsTupleExpansion``
   ``uint8`` flag per iterator — nonzero if the parser saw ``(k,v)`` syntax.
``iteratorVariables``
   Resolved variables (populated by inference, cleared on rewrite).

The macro scans for a source where the tuple-expansion flag is set
**and** the source type is a table:

.. literalinclude:: ../../../../../tutorials/macros/for_loop_macro_mod.das
   :language: das
   :lines: 23-31


Step 3 — Split the backtick-joined name
----------------------------------------

When the user writes ``(k,v)``,  the parser stores the iterator name as
``"k`v"`` (joined with a backtick).  The macro splits this into
``key_name`` and ``val_name``:

.. literalinclude:: ../../../../../tutorials/macros/for_loop_macro_mod.das
   :language: das
   :lines: 36-42


Step 4 — Clone and rewrite
---------------------------

The transformation follows the same pattern as the standard library's
``SoaForLoop`` (in ``daslib/soa.das``):

1. **Clone** the entire ``ExprFor``.
2. **Erase** the table entry at ``tab_index`` from all parallel vectors.
3. **Add** two new sources — ``keys(tab)`` and ``values(tab)`` — with
   the split iterator names.
4. **Clear** ``iteratorVariables`` so inference rebuilds them on the
   next pass.

.. literalinclude:: ../../../../../tutorials/macros/for_loop_macro_mod.das
   :language: das
   :lines: 43-72

The helper ``make_kv_call`` builds an ``ExprCall`` node for ``keys()``
or ``values()``:

.. literalinclude:: ../../../../../tutorials/macros/for_loop_macro_mod.das
   :language: das
   :lines: 77-80


Using the macro
===============

Section 1 — Verbose (without macro)
------------------------------------

.. literalinclude:: ../../../../../tutorials/macros/09_for_loop_macro.das
   :language: das
   :lines: 16-21

This is the standard idiom: ``keys()`` and ``values()`` return separate
iterators that advance in lock-step.


Section 2 — Tuple destructuring
---------------------------------

.. literalinclude:: ../../../../../tutorials/macros/09_for_loop_macro.das
   :language: das
   :lines: 27-30

Exactly the same output, but the syntax is more natural.  The macro
rewrites this to the verbose form transparently.


Section 3 — Mixed sources
---------------------------

The macro handles the table source independently; other sources in the
same loop are preserved:

.. literalinclude:: ../../../../../tutorials/macros/09_for_loop_macro.das
   :language: das
   :lines: 37-40

Here ``(k,v)`` iterates over the table while ``idx`` counts from
``range(100)``.  All three advance in parallel.


Running the tutorial
====================

::

  daslang.exe tutorials/macros/09_for_loop_macro.das

Expected output::

  --- Section 1: table iteration without the macro ---
    one => 1
    three => 3
    two => 2

  --- Section 2: for ((k,v) in tab) with the macro ---
    one => 1
    three => 3
    two => 2

  --- Section 3: mixed sources ---
    [0] apple => 10
    [1] banana => 20
    [2] cherry => 30

(Table iteration order may vary.)


Real-world example
==================

The standard library module ``daslib/soa.das`` uses the same mechanism.
Its ``SoaForLoop`` macro rewrites ``for (it in soa_struct)`` to iterate
over the individual arrays of a structure-of-arrays layout — a more
complex transformation but the same ``AstForLoopMacro`` pattern.


.. seealso::

   Full source:
   :download:`09_for_loop_macro.das <../../../../../tutorials/macros/09_for_loop_macro.das>`,
   :download:`for_loop_macro_mod.das <../../../../../tutorials/macros/for_loop_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_variant_macro`

   Next tutorial: :ref:`tutorial_macro_capture_macro`

   Standard library: ``daslib/soa.das`` (``SoaForLoop`` macro)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
