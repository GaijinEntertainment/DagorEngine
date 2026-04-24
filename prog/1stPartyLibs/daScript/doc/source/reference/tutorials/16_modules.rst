.. _tutorial_modules:

=============================
Modules and Program Structure
=============================

.. index::
    single: Tutorial; Modules
    single: Tutorial; Require
    single: Tutorial; Program Structure

This tutorial covers the file layout (options, module, require), creating
and using a separate module file, public vs private visibility, qualified
calls with ``::``, and common standard library modules.

File layout
===========

A typical daslang file follows this order::

  options gen2                         // compiler options
  module my_module public              // module declaration
  require math                         // imports
  // ... declarations (struct, enum, def, etc.)

The ``module`` line must appear before any declarations.

Module declaration
==================

The ``module`` keyword names the current file. Omit it to default to the
filename::

  module my_lib public      // public declarations by default
  module my_lib private     // private declarations by default
  module my_lib shared      // shared across contexts

Creating a module
=================

Create a file ``tutorial_helpers.das`` next to your main script::

  options gen2
  module tutorial_helpers public

  struct Item {
      name : string
      value : int
  }

  def make_item(n : string; v : int) : Item {
      return Item(name=n, value=v)
  }

  def describe(item : Item) : string {
      return "{item.name} (worth {item.value})"
  }

  def private internal_id(item : Item) : int {
      return item.value * 31
  }

  let MAX_ITEMS = 100

Using a module
==============

In the main file, import and use it::

  require tutorial_helpers

  let sword = make_item("Sword", 150)
  print("{describe(sword)}\n")
  print("max: {MAX_ITEMS}\n")

If the module file is in the same directory, ``require`` finds it by name.

Qualified calls
===============

Use ``module::function`` to disambiguate::

  let shield = tutorial_helpers::make_item("Shield", 80)
  let r = math::sqrt(16.0)

This is essential when two modules export the same function name.

Visibility
==========

Each declaration can override the module default::

  def public  helper() { ... }    // always visible
  def private internal() { ... }  // only within this module

Private functions like ``internal_id()`` above cannot be called from
other modules — the compiler reports an error.

Re-exporting
============

``require ... public`` re-exports a module to your importers::

  require math public       // importers of this module also get math

Common standard library modules
================================

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Module
     - Purpose
   * - ``math``
     - sin, cos, sqrt, atan2, etc.
   * - ``strings``
     - to_int, to_float, starts_with, etc.
   * - ``fio``
     - File I/O
   * - ``rtti``
     - Runtime type information
   * - ``daslib/json``
     - JSON parsing
   * - ``daslib/regex``
     - Regular expressions
   * - ``daslib/algorithm``
     - Sorting and searching
   * - ``daslib/strings_boost``
     - Advanced string utilities

.. seealso::

   :ref:`Modules <modules>`, :ref:`Program Structure <program_structure>` in the
   language reference.

   Full source: :download:`tutorials/language/16_modules.das <../../../../tutorials/language/16_modules.das>` and
   :download:`tutorials/language/tutorial_helpers.das <../../../../tutorials/language/tutorial_helpers.das>`

Next tutorial: :ref:`tutorial_move_copy_clone`
