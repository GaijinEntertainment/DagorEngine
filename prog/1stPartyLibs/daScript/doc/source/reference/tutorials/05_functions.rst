.. _tutorial_functions:

========================
Functions
========================

.. index::
    single: Tutorial; Functions

This tutorial covers defining functions, parameter passing, default arguments,
function overloading, returning multiple values with tuples, and early return.

Defining functions
==================

Use ``def`` to declare a function.  Specify parameter types after a colon and
the return type after the parameter list::

  def greet(name : string) {
      print("Hello, {name}!\n")
  }

  def add(a, b : int) : int {
      return a + b
  }

When multiple parameters share a type, list them with commas before the single
type annotation: ``a, b : int``.

Pass-by-value vs pass-by-reference
===================================

By default, parameters are passed by value and are immutable.  Use ``var``
and ``&`` for a mutable reference::

  def increment(var x : int&) {
      x++
  }

  var val = 42
  increment(val)
  print("{val}\n")     // 43

Default arguments
=================

Parameters can have default values.  Callers may omit them::

  def formatNumber(value : int; prefix : string = "#") : string {
      return "{prefix}{value}"
  }

  formatNumber(42)                     // uses default: "#42"
  formatNumber(42, [prefix = ">"])     // named argument: ">42"

To specify a parameter by name, use named argument syntax:
``[name = value]``.

Function overloading
====================

Multiple functions can share the same name if their parameter types differ.
The compiler selects the correct overload at compile time::

  def describe(x : int)    { print("integer: {x}\n") }
  def describe(x : float)  { print("float: {x}\n") }
  def describe(x : string) { print("string: \"{x}\"\n") }

Returning multiple values
=========================

Use a named tuple to return multiple values::

  def divmod(a, b : int) : tuple<quotient:int; remainder:int> {
      return (quotient = a / b, remainder = a % b)
  }

  let result = divmod(17, 5)
  print("{result.quotient}\n")     // 3

  // Destructure the tuple
  let (q, r) = divmod(23, 7)

Early return
============

Functions can return early from any point::

  def classifyAge(age : int) : string {
      if (age < 0)  { return "invalid" }
      if (age < 13) { return "child" }
      if (age < 20) { return "teenager" }
      return "adult"
  }

Running the tutorial
====================

::

  daslang.exe tutorials/language/05_functions.das

Full source: :download:`tutorials/language/05_functions.das <../../../../tutorials/language/05_functions.das>`

See also
========

* **Next:** :ref:`tutorial_arrays` — arrays
* :ref:`Functions <functions>` — full function reference
* :ref:`Blocks <blocks>` — anonymous function blocks
* :ref:`Lambdas <lambdas>` — closures and captures
