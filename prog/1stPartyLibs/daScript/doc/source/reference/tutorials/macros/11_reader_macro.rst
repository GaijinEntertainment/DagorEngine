.. _tutorial_macro_reader_macro:

.. index::
   single: Tutorial; Macros; Reader Macro
   single: Tutorial; Macros; reader_macro
   single: Tutorial; Macros; AstReaderMacro
   single: Tutorial; Macros; visit vs suffix patterns

====================================================
 Macro Tutorial 11: Reader Macros
====================================================

Previous tutorials intercepted calls, functions, structures, blocks,
variants, for-loops, and lambda captures.  Reader macros go further —
they embed **entirely custom syntax** inside daslang source code.

``[reader_macro(name="X")]`` registers a class that extends
``AstReaderMacro``.  Reader macros are invoked with the syntax
``%X~ character_sequence %%``.  The compiler calls three methods:

``accept(prog, mod, expr, ch, info) → bool``
   Called **per character** during parsing.  ``ch`` is the current
   character; ``expr.sequence`` accumulates the text.  Return ``true``
   to keep reading, ``false`` to stop (typically when ``%%`` is found).

``visit(prog, mod, expr) → ExpressionPtr``
   Called during **type inference** on the ``ExprReader`` node.  Must
   return an AST expression that replaces the reader expression.  This
   is the **visit pattern** — used when the macro appears as an
   expression and produces an AST value.

``suffix(prog, mod, expr, info, outLine&, outFile?&) → string``
   Called immediately **after** ``accept()`` during parsing.  Returns
   a string of daslang source code that is injected back into the
   parser's input stream.  This is the **suffix pattern** — used when
   the macro appears at module level and generates top-level daslang
   declarations (functions, structs, etc.).  The ``ExprReader`` node is
   discarded.

.. note::

   The two patterns serve different contexts:

   - **Visit pattern**: the reader macro appears in an **expression**
     position (e.g., ``var x = %csv~ ... %%``).  ``visit()`` builds
     the resulting AST node.  ``suffix()`` is not used.

   - **Suffix pattern**: the reader macro appears at **module level**
     as a standalone statement (not assigned to a variable).  ``suffix()``
     returns daslang source text that the parser re-parses.  ``visit()``
     is never called because the parser discards the ``ExprReader`` node.

   All reader macros share the same ``accept()`` idiom for collecting
   characters.  The choice between visit and suffix determines where and
   how the macro produces output.


Motivation
==========

Embedding domain-specific notations — CSV data, regular expressions,
JSON literals, template engines — is a common need.  Reader macros let
you write these in their native syntax and transform them at compile time
into efficient daslang code, without runtime parsing overhead.

This tutorial builds both patterns:

- ``%csv~`` — a **visit** reader macro that parses CSV text at compile
  time into a string array (constant embedded in the AST)
- ``%basic~`` — a **suffix** reader macro that transpiles a toy BASIC
  program into a daslang function definition

.. note::

   Macros cannot be used in the module that defines them.  This
   tutorial has **two** source files: a *module* file containing the
   macro definitions and a *usage* file that requires the module.


The module file
===============

``reader_macro_mod.das`` defines two reader macros.

The ``accept()`` idiom
~~~~~~~~~~~~~~~~~~~~~~

Both macros share the same standard ``accept()`` implementation — the
most common pattern in the standard library:

.. code-block:: das

   def override accept(prog : ProgramPtr; mod : Module?;
           var expr : ExprReader?; ch : int; info : LineInfo) : bool {
       if (ch != '\r') {               // skip carriage returns
           append(expr.sequence, ch)    // accumulate in expr.sequence
       }
       if (ends_with(expr.sequence, "%%")) {
           let len = length(expr.sequence)
           resize(expr.sequence, len - 2)   // strip the %%
           return false                     // stop reading
       } else {
           return true                      // keep reading
       }
   }

Characters are appended to ``expr.sequence`` one at a time.  When the
``%%`` terminator is detected, it is stripped and ``accept()`` returns
``false`` to signal the end of the character sequence.

CsvReader — visit pattern
~~~~~~~~~~~~~~~~~~~~~~~~~

``CsvReader`` is registered with ``[reader_macro(name=csv)]``.  Its
``visit()`` method splits the collected sequence by commas, trims each
value, and uses ``convert_to_expression()`` from ``daslib/ast_boost``
to embed the resulting string array in the AST:

.. code-block:: das

   def override visit(prog : ProgramPtr; mod : Module?;
           expr : smart_ptr<ExprReader>) : ExpressionPtr {
       if (is_in_completion()) {
           return <- default<ExpressionPtr>
       }
       let seq = string(expr.sequence)
       var items <- split(seq, ",")
       for (i in range(length(items))) {
           items[i] = strip(items[i])
       }
       return <- convert_to_expression(items, expr.at)
   }

``convert_to_expression`` takes any daslang value and converts it into
AST nodes — here turning an ``array<string>`` into the equivalent of an
array literal.  This is the same utility used by ``daslib/json_boost``
to embed parsed JSON and by ``daslib/regex_boost`` to embed compiled
regex objects.

BasicReader — suffix pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``BasicReader`` is registered with ``[reader_macro(name=basic)]``.  It
overrides ``suffix()`` instead of ``visit()``.  The method parses a
tiny BASIC dialect and returns the equivalent daslang source code:

.. code-block:: das

   def override suffix(prog : ProgramPtr; mod : Module?;
           var expr : ExprReader?; info : LineInfo;
           var outLine : int&; var outFile : FileInfo?&) : string {
       let seq = string(expr.sequence)
       var lines <- split(seq, "\n")
       var func_name = "basic_program"
       var stmts : array<string>
       for (line in lines) {
           let trimmed = strip(line)
           if (empty(trimmed)) { continue }
           if (starts_with(trimmed, "DEF ")) {
               func_name = strip(slice(trimmed, 4))
               continue
           }
           // parse: NUMBER COMMAND args
           let sp1 = find(trimmed, " ")
           if (sp1 < 0) { continue }
           let after_num = strip(slice(trimmed, sp1 + 1))
           if (starts_with(after_num, "PRINT ")) {
               let arg = strip(slice(after_num, 6))
               if (starts_with(arg, "\"")) {
                   let inner = slice(arg, 1, length(arg) - 1)
                   stmts |> push("print(\"{inner}\\n\")")
               } else {
                   stmts |> push("print(\"\{{arg}\}\\n\")")
               }
           } elif (starts_with(after_num, "LET ")) {
               stmts |> push("var {strip(slice(after_num, 4))}")
           }
       }
       return build_string() <| $(var w) {
           w |> write("def {func_name}() \{\n")
           for (stmt in stmts) {
               w |> write("    {stmt}\n")
           }
           w |> write("\}\n")
       }
   }

The returned string is valid gen2 daslang code.  The parser receives
this text and parses it as a normal function definition at module level.

.. note::

   The suffix must produce **gen2 syntax** (with braces) if the
   source file uses ``options gen2``.  String escaping requires care:
   ``\{`` and ``\}`` produce literal braces (avoiding string
   interpolation), while ``{func_name}`` interpolates the variable.


The usage file
==============

``11_reader_macro.das`` demonstrates both patterns.

**Section 1** — CSV reader (visit pattern):

.. code-block:: das

   var data <- %csv~ Alice, 30, New York %%
   print("  items ({length(data)}):\n")
   for (item in data) {
       print("    '{item}'\n")
   }

Output::

   --- Section 1: CSV reader (visit pattern) ---
     items (3):
       'Alice'
       '30'
       'New York'
     colors (3):
       'red'
       'green'
       'blue'

The ``%csv~`` expression evaluates to an ``array<string>`` — the CSV
values are parsed and embedded at compile time, not at runtime.

**Section 2** — BASIC transpiler (suffix pattern):

.. code-block:: das

   %basic~
   DEF basic_hello
   10 PRINT "Hello from BASIC"
   20 LET x = 42
   30 PRINT x
   %%

This appears at **module level** (not inside a function).  The suffix
generates a function ``basic_hello()`` that the rest of the file can
call:

.. code-block:: das

   [export]
   def main() {
       basic_hello()
   }

Output::

   --- Section 2: BASIC transpiler (suffix pattern) ---
   Hello from BASIC
   42

The generated function is indistinguishable from hand-written code — it
participates in type checking, AOT compilation, and all other compiler
phases normally.


Visit vs Suffix
===============

+------------------+--------------------------+---------------------------+
| Aspect           | Visit pattern            | Suffix pattern            |
+==================+==========================+===========================+
| Override         | ``visit()``              | ``suffix()``              |
+------------------+--------------------------+---------------------------+
| When called      | Type inference           | Parsing (after accept)    |
+------------------+--------------------------+---------------------------+
| Returns          | AST expression           | daslang source text       |
+------------------+--------------------------+---------------------------+
| Usage context    | Expression position      | Module level              |
+------------------+--------------------------+---------------------------+
| ExprReader fate  | Replaced by visit result | Discarded by parser       |
+------------------+--------------------------+---------------------------+
| Examples (stdlib)| regex, json, stringify   | spoof_instance            |
+------------------+--------------------------+---------------------------+


Real-world usage
================

The standard library includes several reader macros:

- ``daslib/regex_boost.das`` — ``RegexReader`` (visit): compiles a
  regular expression at parse time and embeds the compiled ``Regex``
  struct directly in the AST.  Usage: ``%regex~pattern%%``
- ``daslib/json_boost.das`` — ``JsonReader`` (visit): parses JSON at
  compile time and embeds the resulting ``JsonValue`` tree.
  Usage: ``%json~ {...} %%``
- ``daslib/stringify.das`` — ``LongStringReader`` (visit): embeds a
  multi-line string literal without escaping.
  Usage: ``%stringify~ text %%``
- ``daslib/spoof.das`` — ``SpoofTemplateReader`` (visit) +
  ``SpoofInstanceReader`` (suffix): a template engine that stores
  templates as strings and instantiates them by generating daslang
  source code at parse time.


.. seealso::

   Full source:
   :download:`11_reader_macro.das <../../../../../tutorials/macros/11_reader_macro.das>`,
   :download:`reader_macro_mod.das <../../../../../tutorials/macros/reader_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_capture_macro`

   Next tutorial: :ref:`tutorial_macro_typeinfo_macro`

   Standard library: ``daslib/regex_boost.das``, ``daslib/json_boost.das``,
   ``daslib/stringify.das``, ``daslib/spoof.das``

   Language reference: :ref:`Macros <macros>` — full macro system documentation
