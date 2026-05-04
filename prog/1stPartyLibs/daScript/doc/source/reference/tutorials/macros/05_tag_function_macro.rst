.. _tutorial_macro_tag_function_macro:

.. index::
   single: Tutorial; Macros; Tag Function Macro
   single: Tutorial; Macros; tag_function
   single: Tutorial; Macros; tag_function_macro
   single: Tutorial; Macros; once

====================================================
 Macro Tutorial 5: Tag Function Macros
====================================================

In tutorials 3 and 4, ``[function_macro]`` required the macro class
to live in a separate module compiled *before* the usage file.  The
annotation needs the class to exist at parse time, so two files are
unavoidable.

daslang offers a lighter pattern — ``[tag_function]`` +
``[tag_function_macro]`` — that lets both the tagged function and its
macro class live in the **same module**.

This tutorial builds a ``once()`` macro that executes a block only on
the first call.  Each call site gets its own auto-generated global
boolean flag:

.. code-block:: das

   for (i in range(5)) {
       once() {
           print("runs exactly once\n")
       }
       print("  iteration {i}\n")
   }

Output::

   runs exactly once
     iteration 0
     iteration 1
     iteration 2
     iteration 3
     iteration 4


Why tag functions?
==================

``[function_macro(name="X")]`` expects the class ``X`` to already exist
when the annotated function definition is parsed.  This means the macro
class and the tagged function **cannot** appear in the same compilation
unit — you always need a two-file setup.

``[tag_function(tag_name)]`` takes a different approach:

1. At parse time, the function is marked with a string tag.  No class
   lookup happens yet.
2. During module setup, ``[tag_function_macro(tag="tag_name")]`` scans
   the module for all functions carrying the matching tag and
   programmatically attaches the macro class as their annotation.

Because the attachment happens *after* both the function and the class
are compiled, a single module can contain everything.

This pattern is used by many standard library modules:

+----------------------------+-------------------------------------------+
| Module                     | Tags                                      |
+============================+===========================================+
| ``daslib/unroll``          | Compile-time loop unrolling               |
+----------------------------+-------------------------------------------+
| ``daslib/defer``           | Go-style ``defer`` (move code to finally) |
+----------------------------+-------------------------------------------+
| ``daslib/assert_once``     | Fire assertion only on first failure      |
+----------------------------+-------------------------------------------+
| ``daslib/static_let``      | Promote locals to hidden globals          |
+----------------------------+-------------------------------------------+
| ``daslib/safe_addr``       | Safe address-of operations                |
+----------------------------+-------------------------------------------+
| ``daslib/jobque_boost``    | ``parallel_for``, ``new_job``             |
+----------------------------+-------------------------------------------+

In all these modules, the public function and the macro class coexist
in one file — no extra "mod" module is needed.

.. note::

   Our tutorial still uses two files because the *usage* file
   ``require``-s the module, which is the normal deployment pattern.
   The key difference from ``[function_macro]`` is that the module
   itself is self-contained — you never need a *third* helper file
   just to define the macro class.


The module: ``tag_function_macro_mod.das``
==========================================

The module has two parts: the tagged function and the macro class.


Part 1 — The tagged function
-----------------------------

.. code-block:: das

   [tag_function(once_tag)]
   def public once(blk : block) {
       invoke(blk)
   }

``[tag_function(once_tag)]`` does two things:

* Records the string ``"once_tag"`` as a tag on this function.
* Does **not** attach any macro class — that happens later.

The function body (``invoke(blk)``) is a fallback: it runs only if the
macro fails to transform the call for some reason.  In normal operation,
every call to ``once()`` is rewritten by ``transform()`` and the
original body is never executed.


Part 2 — The macro class
--------------------------

.. code-block:: das

   [tag_function_macro(tag="once_tag")]
   class OnceMacro : AstFunctionAnnotation {
       def override transform(var call : smart_ptr<ExprCallFunc>;
                              var errors : das_string) : ExpressionPtr {
           // ... rewrite every call to once()
       }
   }

``[tag_function_macro(tag="once_tag")]`` tells the compiler:

   *During module setup, find every function tagged with* ``once_tag``
   *and attach this class as its function annotation.*

After setup, every call to ``once()`` triggers the ``transform()``
method exactly as if we had used ``[function_macro]``.


Inside ``transform()``
======================

The method receives the call expression and returns a replacement AST.
It proceeds in four steps.


Step 1 — Generate a unique flag name
-------------------------------------

.. code-block:: das

   let flag_name = make_unique_private_name("__once_flag", call.at)

``make_unique_private_name`` combines the prefix with the call-site
line and column numbers, producing names like ``__once_flag_12_5``.
Every call site gets its own name, so multiple ``once()`` calls in the
same function are completely independent.


Step 2 — Create the global flag
---------------------------------

.. code-block:: das

   if (!compiling_module() |> add_global_private_var(flag_name, call.at) <| quote(false)) {
       errors := "can't add global variable {flag_name}"
       return <- default<ExpressionPtr>
   }

``add_global_private_var`` inserts a private ``bool`` variable (initialized
to ``false`` via ``quote(false)``) into the module being compiled.  The
variable is private, so it never leaks into the public API.

If the variable already exists (e.g., the compiler re-runs inference),
the function returns ``false`` and we report an error.


Step 3 — Extract the block body
---------------------------------

.. code-block:: das

   var inscope block_clone <- clone_expression(call.arguments[0])
   var inscope blk <- move_unquote_block(block_clone)
   var inscope stmts : array<ExpressionPtr>
   for (s in blk.list) {
       stmts |> emplace_new <| clone_expression(s)
   }

When the user writes ``once() { ... }``, the first argument is an
``ExprMakeBlock`` wrapping an ``ExprBlock``.  We:

1. Clone the argument expression (never modify the original AST).
2. Unwrap the ``ExprMakeBlock`` → ``ExprBlock`` via ``move_unquote_block``.
3. Copy its statement list into a flat ``array<ExpressionPtr>``.

The statements array is needed because the ``$b()`` splice operator
expects ``array<ExpressionPtr>``, not an ``ExprBlock`` directly.


Step 4 — Build the replacement
--------------------------------

.. code-block:: das

   var inscope replacement <- qmacro_block() {
       if (!$i(flag_name)) {
           $i(flag_name) = true
           $b(stmts)
       }
   }
   replacement |> force_at(call.at)
   return <- replacement

``qmacro_block`` builds an ``ExprBlock`` using the reification
mini-language:

* ``$i(flag_name)`` splices the string as an identifier reference.
* ``$b(stmts)`` splices the statement array into the ``if`` body.

``force_at`` stamps every node in the replacement with the original
call-site location so error messages point to the right place.

The final expansion of:

.. code-block:: das

   once() {
       print("hello\n")
   }

is:

.. code-block:: das

   if (!__once_flag_12_5) {
       __once_flag_12_5 = true
       print("hello\n")
   }


The usage file
==================

.. code-block:: das

   options gen2
   require tag_function_macro_mod

   def test_loop() {
       for (i in range(3)) {
           once() {
               print("initialized (runs once)\n")
           }
           print("  iteration {i}\n")
       }
   }

   def test_multiple() {
       for (i in range(2)) {
           once() {
               print("first once (runs once)\n")
           }
           once() {
               print("second once (runs once)\n")
           }
           print("  pass {i}\n")
       }
   }

   def greet() {
       once() {
           print("welcome! (runs once)\n")
       }
       print("  greet called\n")
   }

   [export]
   def main() {
       print("--- test_loop ---\n")
       test_loop()
       print("\n--- test_multiple ---\n")
       test_multiple()
       print("\n--- test_greet ---\n")
       greet()
       greet()
       greet()
   }


``test_loop`` — each iteration checks the flag; only the first one
fires.  ``test_multiple`` — two ``once()`` calls have *different*
flags (different line numbers), so both fire once.  ``greet`` — the
flag is global, so three separate calls still fire only once.

Full output::

   --- test_loop ---
   initialized (runs once)
     iteration 0
     iteration 1
     iteration 2

   --- test_multiple ---
   first once (runs once)
   second once (runs once)
     pass 0
     pass 1

   --- test_greet ---
   welcome! (runs once)
     greet called
     greet called
     greet called


Key takeaways
=============

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - Concept
     - What it does
   * - ``[tag_function(tag)]``
     - Marks a function with a string tag — no class lookup at parse time
   * - ``[tag_function_macro(tag="..")]``
     - During module setup, attaches the macro class to all functions with matching tag
   * - ``transform()``
     - Called at every call site; returns a replacement ``ExpressionPtr``
   * - ``make_unique_private_name``
     - Generates ``prefix_LINE_COLUMN`` names unique to each call site
   * - ``add_global_private_var``
     - Creates a private mutable module-level variable at compile time
   * - ``clone_expression``
     - Deep-clones an AST node (never mutate the original)
   * - ``move_unquote_block``
     - Unwraps ``ExprMakeBlock`` → ``ExprBlock``
   * - ``qmacro_block``
     - Reification: builds a block from spliced identifiers and statements
   * - ``$i(name)``
     - Splice a string as an identifier
   * - ``$b(stmts)``
     - Splice ``array<ExpressionPtr>`` as a statement list
   * - ``force_at``
     - Stamps source location on all nodes


.. seealso::

   Full source:
   :download:`tag_function_macro_mod.das <../../../../../tutorials/macros/tag_function_macro_mod.das>`,
   :download:`05_tag_function_macro.das <../../../../../tutorials/macros/05_tag_function_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_advanced_function_macro`

   Next tutorial: :ref:`tutorial_macro_structure_macro`

   Standard library examples:
   ``daslib/assert_once.das`` (closest to our ``once()``),
   ``daslib/unroll.das``, ``daslib/defer.das``, ``daslib/static_let.das``

   Language reference: :ref:`Macros <macros>` — full macro system documentation
