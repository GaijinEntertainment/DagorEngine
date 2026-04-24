.. _tutorial_macro_block_macro:

.. index::
   single: Tutorial; Macros; Block Macro
   single: Tutorial; Macros; block_macro
   single: Tutorial; Macros; AstBlockAnnotation
   single: Tutorial; Macros; traced
   single: Tutorial; Macros; finalList

====================================================
 Macro Tutorial 7: Block Macros
====================================================

Tutorials 3–6 transformed *functions* and *structs*.  Block macros
operate on **block closures** instead — the ``$ { ... }`` expressions
that are passed to functions as arguments.

``[block_macro(name="X")]`` registers a class that extends
``AstBlockAnnotation``.  When a block is annotated with ``[X]``, the
compiler calls the macro's methods at two stages of the compilation
pipeline:

+---------------------+----------------------------------------------+
| Method              | When it runs                                 |
+=====================+==============================================+
| ``apply()``         | During parsing, **before** inference.        |
|                     | Can modify the block's statement list and    |
|                     | finally list, validate annotation arguments, |
|                     | and inject code.                             |
+---------------------+----------------------------------------------+
| ``finish()``        | After all inference and optimization.        |
|                     | Read-only — argument types are fully         |
|                     | resolved, useful for diagnostics.            |
+---------------------+----------------------------------------------+

This tutorial builds a ``[traced(tag="X")]`` annotation that:

1. Prepends an enter-message and appends an exit-message (via
   ``finalList``) to the block body — the exit message runs even on
   early return (``apply``).
2. Prints a compile-time summary of each block's typed arguments and
   statement count (``finish``).


.. code-block:: das

   run_block() $ [traced(tag="setup")] {
       print("  initializing\n")
   }

After compilation the block body gains ``print(">> setup\n")`` at the
start and ``print("<< setup\n")`` in its finally section, so every
invocation prints entry and exit markers around the user code.


Block annotation syntax
=======================

Block annotations are placed between the ``$`` sigil and the parameter
list (or body, for parameterless blocks):

.. code-block:: das

   // Parameterless block
   $ [annotation(args)] { body }

   // Block with parameters
   $ [annotation(args)] (params) { body }

Multiple annotations can be comma-separated inside the brackets, just
like function annotations:

.. code-block:: das

   $ [traced(tag="x"), REQUIRE(hp)] (v : int) { ... }


Why only two methods?
=====================

Structure macros have three methods (``apply``, ``patch``, ``finish``)
because structs often need type-aware code generation in ``patch()``
after inference resolves field types.  Block macros skip ``patch()``
entirely:

* **apply()** — The block is parsed but types may not be resolved.  You
  can modify ``blk.list`` (the statement list) and ``blk.finalList``
  (the finally section).  Validation and code injection happen here.

* **finish()** — Everything is final: argument types are resolved, code
  is optimized.  No modifications allowed.  Use it for diagnostics,
  compile-time reporting, or verifying block structure.

Blocks are self-contained expressions — simpler than functions or
structs — so two methods are sufficient.  If you need post-inference
code generation, use a function macro or structure macro instead.


The module: ``block_macro_mod.das``
===================================

Registration
------------

.. code-block:: das

   [block_macro(name="traced")]
   class TracedBlockMacro : AstBlockAnnotation {
       ...
   }

``[block_macro(name="traced")]`` tells the compiler:

   *When a block is annotated with* ``[traced]``, *call this class's
   methods during compilation.*

Block annotations are registered as function annotations under the
hood — ``add_new_block_annotation`` internally calls
``add_function_annotation``.  This is why block annotations share the
``AnnotationArgumentList`` parameter type with function macros.


Inside ``apply()``
------------------

The method receives the block expression, annotation arguments, and
an error string.  It runs during parsing, before inference.


Step 1 — Validate arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   let labelArg = find_arg(args, "tag")
   if (!(labelArg is tString)) {
       errors := "[traced] requires a 'tag' string argument"
       return false
   }
   let lbl = labelArg as tString

We use ``find_arg`` (from ``daslib/ast_boost``) to look up the
``tag`` argument by name.  It returns an ``RttiValue`` variant —
we check ``is tString`` and cast with ``as tString``.  Returning
``false`` aborts compilation with the error message.


Step 2 — Prepend enter-print
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   var inscope enterExpr <- qmacro(print($v(">> {lbl}\n")))
   blk.list |> emplace(enterExpr, 0)

``blk.list`` is the block's statement array.  ``emplace(vec, val, 0)``
inserts at position 0, pushing existing statements down.  This places
the enter-print **before** any user code.

``$v(">> {lbl}\n")`` splices the compile-time string (with the tag
value baked in) as a constant expression in the generated code.


Step 3 — Append exit-print to ``finalList``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   var inscope exitExpr <- qmacro(print($v("<< {lbl}\n")))
   blk.finalList |> emplace(exitExpr)

``blk.finalList`` is the block's **finally** section — statements here
run when the block exits, regardless of how it exits.  This guarantees
the exit message prints even if the block has an early return.

.. note::

   ``finalList`` on loop blocks (``blockFlags.inTheLoop``) only runs
   once at loop exit, not per iteration.  For block closures passed to
   functions (like in this tutorial), ``finalList`` runs on every
   ``invoke``, which is correct for enter/exit tracing.


Inside ``finish()``
-------------------

.. code-block:: das

   def override finish(var blk : smart_ptr<ExprBlock>; var group : ModuleGroup;
                       args, progArgs : AnnotationArgumentList;
                       var errors : das_string) : bool {
       let labelArg = find_arg(args, "tag")
       var lbl = "?"
       if (labelArg is tString) {
           lbl = labelArg as tString
       }

       // Count original statements (subtract injected enter-print)
       var numStmts = 0
       for (s in blk.list) {
           numStmts++
       }
       if (numStmts > 0) {
           numStmts -= 1
       }

       print("[traced] \"{lbl}\": {numStmts} statement(s)")

       // Iterate typed arguments — types are now resolved
       var numArgs = 0
       for (a in blk.arguments) {
           numArgs++
       }
       if (numArgs > 0) {
           print(", args = (")
           var first = true
           for (arg in blk.arguments) {
               if (!first) {
                   print(", ")
               }
               print("{arg.name}:{describe(arg._type)}")
               first = false
           }
           print(")")
       }

       print("\n")
       return true
   }

``finish()`` runs after all inference and optimization.  The block is
in its final form — argument types are fully resolved.  We iterate
``blk.arguments`` (a managed vector of ``VariablePtr``) and call
``describe(arg._type)`` to get human-readable type names like
``int const``.

We count statements by iterating ``blk.list`` and subtract one to
account for the enter-print injected by ``apply()``.


The usage file
==============

.. code-block:: das

   options gen2
   require block_macro_mod

   def run_block(blk : block) {
       invoke(blk)
   }

   def apply_to(x : int; blk : block<(v : int) : void>) {
       invoke(blk, x)
   }

   [export]
   def main() {
       print("--- simple block ---\n")
       run_block() $ [traced(tag="setup")] {
           print("  initializing\n")
       }

       print("\n--- block with argument ---\n")
       apply_to(42) $ [traced(tag="process")] (v : int) {
           print("  received {v}\n")
       }
   }

The first block is parameterless — it demonstrates the basic
``[traced]`` syntax.  The second block takes an ``int`` parameter,
showing that ``finish()`` can report its fully-typed arguments.

Both blocks are invoked by helper functions via ``invoke(blk)`` /
``invoke(blk, x)``.  Each invocation executes the modified block body
(with the injected enter/exit prints).

Compile-time output (from ``finish``)::

   [traced] "setup": 1 statement(s)
   [traced] "process": 1 statement(s), args = (v:int const)

Runtime output::

   --- simple block ---
   >> setup
     initializing
   << setup

   --- block with argument ---
   >> process
     received 42
   << process


Compilation pipeline summary
=============================

The full sequence for a ``[traced]`` block:

.. code-block:: text

   parse block closure
     ↓
   apply() → validate tag, prepend enter-print, append exit-print to finalList
     ↓
   infer types (block arguments and body are inferred)
     ↓
   optimize
     ↓
   finish() → compile-time diagnostic (statement count, typed args)
     ↓
   simulate → invoke block at runtime (enter-print, user code, exit-print)

Unlike structure macros, there is no ``patch`` → ``re-infer`` cycle.
All code injection happens in ``apply()`` before the first inference
pass.


Key takeaways
=============

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - Concept
     - What it does
   * - ``[block_macro(name="X")]``
     - Registers a class as a block annotation
   * - ``AstBlockAnnotation``
     - Base class with ``apply`` and ``finish`` methods (no ``patch``)
   * - ``apply()``
     - Pre-inference: modify statement list, inject code, validate args
   * - ``finish()``
     - Final: read-only diagnostics and reporting with resolved types
   * - ``blk.list``
     - The block's statement array — modify to inject code
   * - ``blk.finalList``
     - The block's finally section — runs on exit, like a ``finally`` block
   * - ``blk.arguments``
     - The block's parameter list (``VariablePtr`` elements)
   * - ``emplace(vec, val, 0)``
     - Insert at a specific position in a managed vector
   * - ``find_arg``
     - Look up annotation argument values by name
   * - ``describe(arg._type)``
     - Human-readable type name from a ``TypeDeclPtr``
   * - ``$ [ann(args)] (params) { body }``
     - Block annotation syntax — annotation between ``$`` and parameters


.. seealso::

   Full source:
   :download:`block_macro_mod.das <../../../../../tutorials/macros/block_macro_mod.das>`,
   :download:`07_block_macro.das <../../../../../tutorials/macros/07_block_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_structure_macro`

   Next tutorial: :ref:`tutorial_macro_variant_macro`

   Standard library examples:
   ``daslib/decs_boost.das`` (``REQUIRE`` / ``REQUIRE_NOT`` — marker block annotations),
   ``daslib/defer.das`` (``finalList`` manipulation),
   ``daslib/heartbeat.das`` (``emplace(list, expr, 0)`` prepend pattern)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
