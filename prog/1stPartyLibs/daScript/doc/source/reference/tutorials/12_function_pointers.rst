.. _tutorial_function_pointers:

=================================
Function Pointers
=================================

.. index::
    single: Tutorial; Function Pointers
    single: Tutorial; Anonymous Functions
    single: Tutorial; @@

This tutorial covers getting a pointer to a named function with ``@@``,
the ``function<>`` type, calling function pointers, disambiguating
overloads with ``@@<signature>``, anonymous functions, and the distinction
between ``function<>``, ``lambda<>``, and ``block<>``.

Getting a function pointer
==========================

Use ``@@`` before a function name to get a pointer to it. The result is
a value of type ``function<>``::

  def double_it(x : int) : int {
      return x * 2
  }

  let fn = @@double_it       // type: function<(x:int):int>
  print("{fn(5)}\n")         // 10

Function pointers support **call syntax** — call them like regular
functions. ``invoke()`` is the explicit alternative::

  fn(5)              // call syntax
  invoke(fn, 5)      // same result

Passing functions as arguments
==============================

Declare a parameter with ``function<>`` type to accept function pointers::

  def apply(f : function<(x:int):int>; value : int) : int {
      return f(value)
  }

  apply(@@double_it, 7)        // 14
  apply(@@negate, 7)           // -7

Only ``@@``-created values can be passed to ``function<>`` parameters.
Lambdas (``@``) and blocks (``$``) are rejected by the compiler.

Disambiguating overloads
========================

When multiple functions share a name, specify the signature::

  def add(a, b : int) : int { return a + b }
  def add(a, b : float) : float { return a + b }

  let fn_int   = @@<(a:int;b:int):int> add
  let fn_float = @@<(a:float;b:float):float> add

  fn_int(3, 4)        // 7
  fn_float(1.5, 2.5)  // 4

Anonymous functions
===================

``@@`` can also create a function inline — no name, no capture::

  let square <- @@(x : int) : int => x * x
  print("{square(6)}\n")        // 36

Multi-line version::

  let clamp <- @@(x : int) : int {
      if (x < 0) { return 0 }
      if (x > 100) { return 100 }
      return x
  }

Anonymous functions **cannot** capture outer variables::

  var outer = 10
  let bad <- @@(x : int) : int => x + outer   // ERROR: can't locate variable

Pass anonymous functions directly to ``function<>`` parameters::

  apply(@@(x : int) : int => x * x + 1, 8)    // 65

function<> vs lambda<> vs block<>
==================================

These are three **distinct** callable types:

.. list-table::
   :header-rows: 1
   :widths: 20 25 25 30

   * - Feature
     - ``function<>`` (@@)
     - ``lambda<>`` (@)
     - ``block<>`` ($)
   * - Allocation
     - None
     - Heap
     - Stack
   * - Capture
     - None
     - By copy
     - By reference
   * - Storable
     - Yes
     - Yes (move)
     - No
   * - Returnable
     - Yes
     - Yes
     - No

A ``function<>`` parameter accepts **only** ``@@`` values.
A ``lambda<>`` parameter accepts **only** ``@`` values.
A ``block<>`` parameter accepts any of them (most flexible).

Use ``function<>`` for pure transforms with no state.
Use ``lambda<>`` when you need captured variables.
Use ``block<>`` for temporary callback parameters.

.. seealso::

   :ref:`Functions <functions>` in the language reference.

   Full source: :download:`tutorials/language/12_function_pointers.das <../../../../tutorials/language/12_function_pointers.das>`

Next tutorial: :ref:`tutorial_blocks`
