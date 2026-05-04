.. _tutorial_hello_world:

===================
Hello World
===================

.. index::
    single: Tutorial; Hello World

Your first daslang program introduces the essential building blocks:
``options gen2`` to enable modern syntax, ``[export] def main`` as the entry
point, ``print`` for console output, and string interpolation.

Setup
=====

Every daslang source file in this tutorial series starts with::

  options gen2

This enables gen2 syntax, which requires curly braces ``{ }`` around blocks
and parentheses ``( )`` around conditions — a familiar style for anyone coming
from C, C++, or similar languages.

Entry point
===========

A runnable daslang program needs a ``main`` function marked with ``[export]``::

  [export]
  def main {
      print("Hello, World!\n")
  }

``[export]`` makes the function visible to the host application (in this case
the ``daslang`` interpreter).  ``print`` writes text to the console but does
**not** add a newline — you must include ``\n`` explicitly.

String interpolation
====================

Inside a string literal, any expression enclosed in ``{ }`` is evaluated and
converted to text::

  let name = "daslang"
  print("Welcome to {name}!\n")

  let a = 10
  let b = 20
  print("{a} + {b} = {a + b}\n")

To print a literal brace, escape it with a backslash: ``\{`` and ``\}``.

Running the tutorial
====================

::

  daslang.exe tutorials/language/01_hello_world.das

Expected output::

  Hello, World!
  Welcome to daslang!
  2 + 3 = 5
  10 + 20 = 30
  Use {braces} for interpolation

Full source: :download:`tutorials/language/01_hello_world.das <../../../../tutorials/language/01_hello_world.das>`

See also
========

* **Next:** :ref:`tutorial_variables` — variables and types
* :ref:`Program structure <program_structure>` — how daslang programs are organized
* :ref:`Expressions <expressions>` — expression syntax including string builders
