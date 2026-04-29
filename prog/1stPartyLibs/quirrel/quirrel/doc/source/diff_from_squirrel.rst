.. _diff_from_squirrel:

*******************************************
Differences from Squirrel language
*******************************************

.. index::
    single: diff_from_squirrel


Squirrel language was used a lot in all Gaijin projects since 2006.
Gaijin used Lua before but found Lua too unsafe and hard to debug, and also hard sometimes to bind with native code due to lack of types.
It is well known and there are lot info on this in the Internet.
Squirrel has not only C++ like syntax (which is helpful for C++ programmers sometimes), but also has type system much closer to C/C++ code.
It also has safer syntax and semantic.
However during heavy usage we found that some features of language are not used in our real codebase of millions of LoC in 20+ projects on 10+ platforms.
Some language features were also not very safe, or inconsistent. We needed stricter language with tools for easier refactor and support.
And of course in game-development any extra performance is always welcome.
Basically in any doubt we used 'Zen of Python'.

We renamed language from Squirrel to Quirrel to avoid ambiguity.

------------
New features
------------

Language syntax
===============

* Template strings: ``$"Hello {name}"`` for string interpolation
* Documentation strings: ``@@"docstring"`` for inline documentation
* Compiler directives: ``#pragma`` for configuring compiler options
* Type method access: ``obj.$method`` and ``obj?.$method``
* Numeric literal separators: ``1_000_000`` for better readability
* Supports destructuring assignment to unpack values from arrays and objects (e.g., ``let {a, b} = table``), including destructuring in function parameters with default values and type annotations
* ES2015-style shorthand table initialization: ``{x, y}`` instead of ``{x=x, y=y}``
* Named functions in expressions: ``let foo = function foo() {}``
* Null-safety operators:

  * ``?.`` - Null-conditional member access: ``obj?.property``
  * ``?[`` - Null-conditional indexing: ``arr?[index]``
  * ``?(`` - Null-conditional function call: ``func?(args)``
  * ``??`` - Null-coalescing operator: ``value ?? default``

* New keywords:

  * ``let`` - Immutable binding declaration (alternative to ``local``)
  * ``global`` - Explicit global variable declaration
  * ``not in`` - For membership testing, alternative to ``!(x in y)``
  * ``static`` - For evaluating expressions once on first access and memoizing the result
  * ``const`` can also be used for inline declaration inside expressions

Static analysis
===============

* Comprehensive static analyzer 90+ diagnostic checks
  (including reporting unused variables, functions, imports, parameters,
  unreachable code detection, type mismatches and null safety violations,
  data flow analysis, scope and visibility checks and more)

Compiler architecture
=====================

* AST-based multi-pass compilation (vs. Squirrel's single-pass direct compilation)

VM architecture
====================

* Class method hinting for performance optimization (approaching LuaJIT non-JIT performance)
* Added tracking of changing values of container fields (saving the stack of code that modifies the values)

Type system & immutability
===========================

* Immutability support:

  * ``freeze()`` function for protecting references to instances, tables, arrays from modification
  * ``let`` keyword for non-assignable bindings

* Pure function marking for optimization hints and use in constants

Standard library
================

* Base library additions:

  * ``println()``, ``errorln()`` - print/error with newline variants
  * ``print()``, ``println()``, ``error()``, ``errorln()`` accept arbitrary number of parameters to print
  * ``freeze(obj)`` - return an immutable reference to an object
    (also ``getobjflags(obj)`` to get object flags (e.g., SQOBJ_FLAG_IMMUTABLE) and
    ``is_frozen()`` for container types.
  * ``deduplicate_object(obj)`` - deduplicate table/array contents for memory efficiency
  * Enhanced ``assert()`` - accepts closures for lazy message evaluation
  * Support for ``call()``/``acall()`` type methods for classes
  * Support negative indexes in ``array.slice()`` and ``string.slice()``

* String type methods:

  * Added: ``.hash()``, ``.contains()``, ``.hasindex()``, ``.subst()``, ``.replace()``, ``.join()``, ``.concat()``,
    ``.clone()``, plus moved from stdlib: ``.strip()``, ``.lstrip()``, ``.rstrip()``, ``.split()``,
    ``.split_by_chars()``, ``.escape()``, ``.startswith()``, ``.endswith()`` (some are shared with the string stardard library)
  * Changed: ``.find()`` renamed to ``.indexof()`` to reduce ambiguity

* Table type methods:

  * Added:** ``.each()``, ``.reduce()``, ``.findindex()``, ``.findvalue()``, ``.topairs()``, ``.swap()``,
    ``.__merge()``, ``.__update()``, ``.replace_with()``, ``.hasindex()``, ``.hasvalue()``, ``.clone()``,
    ``.is_frozen()``, ``.getfuncinfos()``
  * Removed: ``.setdelegate()``, ``.getdelegate()``

* Array type methods:

  * Added: ``.each()``, ``.findindex()``, ``.findvalue()``, ``.contains()``, ``.hasindex()``, ``.hasvalue()``,
    ``.totable()``, ``.replace_with()``, ``.swap()``, ``.clone()``, ``.is_frozen()``
  * Changed:
    * ``.find()`` renamed to ``.indexof()``
    * ``.append()`` and ``.extend()`` now accept multiple arguments
  * Removed: ``.push()`` (use ``.append()`` instead)

* Standard library modules:

  * Added: datetime, debug, and serialization modules
  * String module: Functions moved to string type methods for convenience (still available as module functions)

Module system
=============

Support for modules. Two APIs - runtime (``require()`` function) and compile-time (``import`` statement)

----------------
Removed features
----------------

Language features
=================

* Octal number literals - was error-prone, ``0123`` syntax now produces an error
* Comma operator - removed from expressions (was never used, but error-prone)
* Class and member attributes - ``</`` and ``/>`` XML-style attribute syntax removed (were never used in real code)
* ``#`` single line comments - only ``//`` comments supported now (consistency - "one obvious way")
* ``rawcall`` keyword
* Post-initializer syntax - removed due to ambiguity (especially with optional commas)
* ``class::method`` syntax sugar - adding table/class methods this way disallowed
  (as There should be one -- and preferably only one -- obvious way to do it).
  Declare methods in-place or explicitly add slots with ``<-`` operator.

Base library functions
======================

The following functions were removed for security, simplicity, or architectural reasons:
``setroottable()``, ``setconsttable()``, ``seterrorhandler()``

----------------
Changes
----------------

Scoping and declarations
=========================

* Functions and classes are locally scoped
* Constants and enums are locally scoped by default - use ``global`` keyword for global declarations
* ``global`` keyword required for declaring global constants and enums explicitly
* Referencing the closure environment object (``this``) is explicit - no implicit ``this`` lookup
* Usage of the root table is deprecated
* No fallback to global root table for undeclared variables - must use ``::`` operator to access globals (if root table is enabled at all)

Compilation Model
=================

* AST-based compiler (vs. Squirrel's single-pass direct bytecode generation):

  * Multi-pass compilation with intermediate AST representation
  * AST optimizations (closure hoisting, constant folding)
  * Static analyzer operates on AST

C/C++ Binding API
=================

* Human-readable function type declarations:

  * Replaces cryptic type masks (``"ac."``) with readable syntax
  * Example: ``split_by_chars(str: string, separators: string, [skip_empty: bool]): array``

* Non-stack-based C APIs for better performance

Class declaration syntax
========================

* Python-style class inheritance: ``class Baz(Bar)`` instead of ``class Baz extends Bar``
* Class ``extends`` keyword removed in favor of parentheses-based syntax

Operators
=========

* ``delete`` is deprecated:
  * Now configurable via pragmas: ``#forbid-delete-operator`` / ``#allow-delete-operator``
  * Recommended to use ``table.$rawdelete()`` or ``table.rawdelete()`` instead

Type methods
=================

* ``.find()`` renamed to ``.indexof()`` for arrays and strings
* ``array.append()`` and ``array.extend()`` now accept multiple arguments
* ``array.filter()`` argument order changed to match map/reduce conventions
  (and in certain degree the one of ``foreach`` by making key argument optional)
* ``array.filter()``, ``array.map()`` and ``array.reduce()`` callbacks optionally accept the reference
  to the container being processed (instead of passing it via ``this``)
* ``getinfos()`` renamed to ``getfuncinfos()`` and now works on callable objects (instances, tables)


Error handling
==============

* Structured diagnostics with severity levels
* Error callback with source location, diagnostic ID, and extra information
* Configurable static analyzer warnings


--------------------------------
Possible future changes
--------------------------------

See RFCs page

