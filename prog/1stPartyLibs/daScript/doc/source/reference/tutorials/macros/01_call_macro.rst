.. _tutorial_macro_call_macro:

.. index::
   single: Tutorial; Macros; Call Macros

====================================
 Macro Tutorial 1: Call Macros
====================================

Call macros intercept function-call syntax at compile time and replace it with
arbitrary AST.  When the compiler sees ``hello()`` or ``printf("...", args)``, it
invokes your macro's ``visit`` method instead of looking for a function —
giving you full control over what code is generated.

This tutorial builds three progressively complex call macros:

1. ``hello()`` — the simplest possible macro (no arguments)
2. ``greet("name")`` — argument validation and string builder construction
3. ``printf(fmt, args...)`` — format-string parsing with argument reordering

.. note::

   Macros cannot be used in the module that defines them.  Every macro tutorial
   therefore has **two** source files: a *module* file containing the macro
   definitions and a *usage* file that requires the module and exercises the
   macros.

Prerequisites
=============

Familiarity with daslang basics (functions, strings, control flow) is assumed.
No prior macro experience is required — concepts are introduced one at a time.

Key imports used by the module::

  require daslib/ast              // AST node types (ExprConstString, etc.)
  require daslib/ast_boost        // AST helpers and ExpressionPtr
  require daslib/templates_boost  // qmacro, $e() reification
  require daslib/strings_boost    // ExprStringBuilder
  require daslib/macro_boost      // [call_macro] annotation, macro_verify


Section 1 — hello(): Minimal call macro
========================================

A call macro is a class that extends ``AstCallMacro``, annotated with
``[call_macro(name="...")]``:

.. code-block:: das

   [call_macro(name="hello")]
   class HelloMacro : AstCallMacro {
       def override visit(prog : ProgramPtr; mod : Module?;
               var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
           macro_verify(length(expr.arguments) == 0, prog, expr.at,
               "hello() takes no arguments")
           return <- qmacro(print("hello, call macro!\n"))
       }
   }

The ``visit`` method receives:

* **prog** — the program being compiled (used for error reporting)
* **mod** — the module where the call appears
* **expr** — the call expression (with ``.arguments`` and ``.at`` for source location)

It returns an ``ExpressionPtr`` — the AST tree that replaces the call.
``qmacro(...)`` is a *reification* helper: you write normal daslang syntax
inside it and it builds the corresponding AST at compile time.

Usage::

  hello()   // → print("hello, call macro!\n")


Section 2 — greet("name"): Argument validation
===============================================

The ``greet`` macro validates its single argument and builds a string
interpolation expression:

.. code-block:: das

   [call_macro(name="greet")]
   class GreetMacro : AstCallMacro {
       def override visit(prog : ProgramPtr; mod : Module?;
               var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
           macro_verify(length(expr.arguments) == 1, prog, expr.at,
               "greet() requires exactly one string argument")
           macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at,
               "greet() argument must be a string literal")
           var inscope sbuilder <- new ExprStringBuilder(at = expr.at)
           sbuilder.elements |> emplace_new <| new ExprConstString(
               value := "hello, ", at = expr.at)
           sbuilder.elements |> emplace_new <| clone_expression(expr.arguments[0])
           sbuilder.elements |> emplace_new <| new ExprConstString(
               value := "!\n", at = expr.at)
           return <- qmacro(print($e(sbuilder)))
       }
   }

Key techniques:

* **``expr.arguments[0] is ExprConstString``** — compile-time type check on the
  AST node to verify the argument is a string literal.
* **``macro_verify``** — emits a compile error and returns an empty expression
  if the condition is false.
* **``ExprStringBuilder``** — the AST node for string interpolation
  (``"hello, {name}!\n"``).  Its ``.elements`` array holds literal strings and
  interpolated expressions.
* **``clone_expression``** — duplicates an AST node.  Always clone arguments
  before inserting them into new AST — the original may be used elsewhere.
* **``$e(expr)``** inside ``qmacro`` — splices an expression node into the
  reified AST.

Usage::

  greet("world")    // → print("hello, world!\n")
  greet("daslang")  // → print("hello, daslang!\n")


Section 3 — printf(fmt, args...): Format-string parsing
========================================================

The ``printf`` macro parses a format string at compile time, replacing
``(N)`` placeholders with the corresponding argument expressions:

.. code-block:: das

   printf("player (1) scored (2) points\n", "Alice", score)
   // → print("player {\"Alice\"} scored {score} points\n")

Arguments can be **reordered** and **repeated**:

.. code-block:: das

   printf("result: (2) from (1)\n", "source", 100)
   printf("(1) and (1) and (1)\n", "echo")

The implementation iterates over the format string character by character,
looking for ``(`` ... ``)`` pairs.  For each placeholder it:

1. Extracts the number with ``chop`` and converts it with ``to_int``
2. Validates bounds with ``macro_verify``
3. Inserts a ``clone_expression`` of the referenced argument

.. code-block:: das

   [call_macro(name="printf")]
   class PrintfMacro : AstCallMacro {
       def override visit(prog : ProgramPtr; mod : Module?;
               var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
           macro_verify(length(expr.arguments) >= 1, prog, expr.at,
               "printf requires at least a format string argument")
           macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at,
               "first argument to printf must be a constant string")
           let totalArgs = length(expr.arguments)
           var inscope sbuilder <- new ExprStringBuilder(at = expr.at)
           let format = string((expr.arguments[0] as ExprConstString).value)
           var pos = 0
           let format_len = length(format)
           while (pos < format_len) {
               var open = find(format, '(', pos)
               if (open == -1) {
                   let tail = format.chop(pos, format_len - pos)
                   sbuilder.elements |> emplace_new <| new ExprConstString(
                       value := tail, at = expr.at)
                   break
               }
               if (open > pos) {
                   let text = format.chop(pos, open - pos)
                   sbuilder.elements |> emplace_new <| new ExprConstString(
                       value := text, at = expr.at)
               }
               var close = find(format, ')', open + 1)
               macro_verify(close != -1, prog, expr.at,
                   "unmatched '(' in format string")
               var argNumStr = format.chop(open + 1, close - open - 1)
               var argNum = to_int(argNumStr)
               macro_verify(argNum >= 1, prog, expr.at,
                   "argument number must be >= 1")
               macro_verify(argNum < totalArgs, prog, expr.at,
                   "argument index out of range")
               sbuilder.elements |> emplace_new <| clone_expression(
                   expr.arguments[argNum])
               pos = close + 1
           }
           return <- qmacro(print($e(sbuilder)))
       }
   }


Running the tutorial
====================

::

  daslang.exe tutorials/macros/01_call_macro.das

Expected output::

  hello, call macro!
  hello, world!
  hello, daslang!
  player Alice scored 42 points
  result: 100 from source
  echo and echo and echo
  pi is approximately 3.14, or roughly 3

.. seealso::

   Full source:
   :download:`call_macro_mod.das <../../../../../tutorials/macros/call_macro_mod.das>`,
   :download:`01_call_macro.das <../../../../../tutorials/macros/01_call_macro.das>`

   Next tutorial: :ref:`tutorial_macro_when_expression`

   Language reference: :ref:`Macros <macros>` — full macro system documentation

