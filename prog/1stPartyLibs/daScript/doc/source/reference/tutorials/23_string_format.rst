.. _tutorial_string_format:

================================
String Builder and Formatting
================================

.. index::
    single: Tutorial; Format Specifiers
    single: Tutorial; String Builder
    single: Tutorial; build_string

This tutorial covers format specifiers in string interpolation and
programmatic string construction with ``build_string``.

Format specifiers
=================

Inside ``{expr}`` interpolation, add ``:flags width.precision type``::

  let x = 42
  print("{x:08x}\n")      // 0000002a
  print("{3.14:.2f}\n")   // 3.14

**Flags:**

- ``0`` — zero-pad
- ``-`` — left-align
- ``+`` — force sign

**Type characters:**

- ``d/i`` — signed decimal, ``u`` — unsigned decimal
- ``x/X`` — hex lower/upper, ``o`` — octal
- ``f`` — fixed-point float, ``e/E`` — scientific
- ``g/G`` — general (shortest of ``f`` or ``e``)

Escaping braces
===============

Use ``\{`` and ``\}`` for literal curly braces::

  print("\{value\}\n")      // prints: {value}
  print("\{{x}\}\n")        // prints: {42}

build_string
============

``build_string`` constructs a string efficiently using a writer::

  require strings

  var csv = build_string() $(var writer : StringBuilderWriter) {
      writer |> write("name,score\n")
      writer |> write("Alice,95\n")
  }

Writer functions:

- ``write(value)`` — write any printable value
- ``write_char(ch)`` — write a single character
- ``write_chars(ch, count)`` — write a character N times
- ``write_escape_string(str)`` — write with escape sequences visible

.. seealso::

   :ref:`String builder <string_builder>` in
   the language reference.

   Full source: :download:`tutorials/language/23_string_format.das <../../../../tutorials/language/23_string_format.das>`

   Next tutorial: :ref:`tutorial_pattern_matching`
