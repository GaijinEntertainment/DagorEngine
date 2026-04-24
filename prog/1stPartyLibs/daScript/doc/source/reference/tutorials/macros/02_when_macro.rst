.. _tutorial_macro_when_expression:

.. index::
   single: Tutorial; Macros; When Expression

====================================================
 Macro Tutorial 2: When Expression
====================================================

This tutorial builds a ``when`` call macro — a value-returning match
expression that compiles ``=>`` tuples inside a block into a chain of
equality tests.  It introduces several advanced techniques beyond the basics
covered in :ref:`tutorial_macro_call_macro`.

.. code-block:: das

   let result = when(x) {
       1 => "one"
       2 => "two"
       _ => "other"
   }

The macro transforms this into:

.. code-block:: das

   let result = invoke($(arg : int const) {
       if (arg == 1) { return "one"; }
       if (arg == 2) { return "two"; }
       return "other";
   }, x)

New concepts introduced:

* **``canVisitArgument``** — control argument type-checking order
* **``canFoldReturnResult``** — defer return-type inference
* **``qmacro_block``** — block-level (statement list) reification
* **``$i()``, ``$t()``, ``$b()``** — inject identifiers, types, and lists
* **``clone_type``** and type flag manipulation
* **``can_shadow``** flag for generated variables
* **``typedecl($e(...))``** — infer types from expression nodes
* **``default<T>``** — generate type-safe default values


Prerequisites
=============

You should be comfortable with the material in
:ref:`tutorial_macro_call_macro` — ``[call_macro]``, ``visit()``,
``macro_verify``, ``qmacro``, and ``$e()``.


How macro_verify works
======================

Before diving in, an important detail: ``macro_verify`` is not a simple
function.  It is a *tag function macro* (defined in ``daslib/macro_boost``)
that expands to::

  if (!condition) {
      macro_error(prog, at, message)
      return <- default<ExpressionPtr>
  }

This means ``macro_verify`` **short-circuits** — if the condition is false,
the enclosing ``visit()`` returns an empty expression immediately.  Code
after ``macro_verify`` only runs when the condition passed.


Controlling argument visitation
===============================

A call macro's arguments are type-checked by the compiler *before*
``visit()`` is called.  Sometimes you need to control this — for example,
the ``when`` block contains ``=>`` tuples that should be parsed but not
fully error-checked until after the macro transforms them.

``canVisitArgument`` lets you decide per-argument:

.. code-block:: das

   def override canVisitArgument(expr : smart_ptr<ExprCallMacro>;
           argIndex : int) : bool {
       return true if (argIndex == 0)
       return !is_reporting_compilation_errors()
   }

* **Argument 0** (the condition value): always visited, so its ``_type``
  is available in ``visit()``.
* **Argument 1** (the block): visited during normal compilation
  (``is_reporting_compilation_errors()`` is false → returns true) but
  **not** during the error-reporting pass (after the macro has already
  transformed it).


Deferring return-type inference
===============================

``canFoldReturnResult`` tells the compiler whether the return type of
the enclosing function can be finalized while this macro call is still
unexpanded:

.. code-block:: das

   def override canFoldReturnResult(
           expr : smart_ptr<ExprCallMacro>) : bool {
       return false
   }

Returning ``false`` prevents the compiler from concluding "this function
returns void" before ``when`` has a chance to produce its ``invoke()``
expression.


Building the statement list
===========================

The core of the macro iterates over the block's statements.  Each statement
is an ``ExprMakeTuple`` (the ``=>`` operator creates tuples):

.. code-block:: das

   for (stmt, idx in blk.list, count()) {
       let tupl = stmt as ExprMakeTuple
       assume cond_value = tupl.values[0]
       let is_default = (cond_value is ExprVar)
                      && (cond_value as ExprVar).name == "_"

For each case, we build either a conditional return or an unconditional
return using ``qmacro_block``:

.. code-block:: das

   if (is_default) {
       list |> emplace_new <| qmacro_block() {
           return $e(tupl.values[1])
       }
   } else {
       list |> emplace_new <| qmacro_block() {
           if ($i(arg_name) == $e(tupl.values[0])) {
               return $e(tupl.values[1])
           }
       }
   }

Key reification escapes:

* **``$e(expr)``** — splices an expression node (as in tutorial 1)
* **``$i(name)``** — converts a string into an identifier (``ExprVar``)
* **``qmacro_block() { ... }``** — produces a statement list
  (``ExpressionPtr``) rather than a single expression


Assembling the block
====================

After building the statement list, we need a typed block argument.
``clone_type`` copies the condition's inferred type, and we adjust flags:

.. code-block:: das

   var inscope cond_type <- clone_type(cond._type)
   cond_type.flags.ref = false      // compare values, not references
   cond_type.flags.constant = true  // argument is read-only

Then we assemble the block and mark its argument as shadowable:

.. code-block:: das

   var inscope call_block <- qmacro(
       $($i(arg_name) : $t(cond_type)){ $b(list); })
   ((call_block as ExprMakeBlock)._block as ExprBlock)
       .arguments[0].flags.can_shadow = true

* **``$t(type)``** — injects a ``TypeDeclPtr`` into the reified AST
* **``$b(list)``** — injects an ``array<ExpressionPtr>`` as the block body
* **``can_shadow``** — allows nested ``when()`` calls to each introduce
  their own ``__when_arg__`` without name conflicts

The final result is an ``invoke`` call:

.. code-block:: das

   return <- qmacro(invoke($e(call_block), $e(cond)))


Auto-generated default case
============================

If no ``_`` default case is provided, the generated block would have no
return on some code paths — the compiler would reject it.  The macro
solves this by automatically generating a default that returns the type's
default value (``""`` for strings, ``0`` for ints, etc.):

.. code-block:: das

   if (!any_default) {
       assume first_value = (blk.list[0] as ExprMakeTuple).values[1]
       list |> emplace_new <| qmacro_block() {
           return default<typedecl($e(first_value))>
       }
   }

* **``typedecl($e(expr))``** — extracts the type from an expression node.
  Here we use the first case's value expression to infer what type the
  ``when`` block should return.
* **``default<T>``** — produces the default value for type ``T``
  (empty string, zero, null pointer, etc.).

This means ``when()`` without ``_`` is safe:

.. code-block:: das

   let found = when(y) {
       1 => "found one"
       2 => "found two"
   }
   // if y is neither 1 nor 2, found == "" (default string)


Usage examples
==============

Integer matching:

.. code-block:: das

   var x = 2
   let result = when(x) {
       1 => "one"
       2 => "two"
       _ => "other"
   }
   // result == "two"

String matching:

.. code-block:: das

   let lang = "daslang"
   let greeting = when(lang) {
       "python"  => "import this"
       "daslang" => "hello, call macro!"
       _         => "unknown language"
   }

Nested when (``can_shadow`` in action):

.. code-block:: das

   let nested = when(a) {
       1 => when(b) {
           1 => "a=1, b=1"
           2 => "a=1, b=2"
           _ => "a=1, b=other"
       }
       _ => "a=other"
   }


Running the tutorial
====================

::

  daslang.exe tutorials/macros/02_when_macro.das

Expected output::

  x=2: two
  lang=daslang: hello, call macro!
  n=3: triple (3 items)
  y=42: ''
  nested: a=1, b=2
  val=3.14: pi

.. seealso::

   Full source:
   :download:`when_macro_mod.das <../../../../../tutorials/macros/when_macro_mod.das>`,
   :download:`02_when_macro.das <../../../../../tutorials/macros/02_when_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_call_macro`

   Next tutorial: :ref:`tutorial_macro_function_macro`

   Language reference: :ref:`Macros <macros>` — full macro system documentation

