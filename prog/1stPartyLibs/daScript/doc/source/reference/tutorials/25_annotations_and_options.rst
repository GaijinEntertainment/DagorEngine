.. _tutorial_annotations:

========================
Annotations and Options
========================

.. index::
    single: Tutorial; Annotations
    single: Tutorial; Options
    single: Tutorial; Export
    single: Tutorial; Init

This tutorial covers function and struct annotations, and compiler
options that control lint, safety, and optimization.

Function annotations
====================

Annotations are placed in ``[ ]`` before the function definition::

  [export]
  def main { ... }

  [sideeffects]
  def log(msg : string) { print(msg) }

  [unused_argument(y)]
  def use_only_x(x, y : int) : int { return x * 2 }

Common function annotations:

- ``[export]`` — callable from host application
- ``[sideeffects]`` — has side effects, won't be eliminated
- ``[init]`` — runs at context initialization
- ``[finalize]`` — runs at context shutdown
- ``[deprecated(message="...")]`` — produces warning on use
- ``[unused_argument(x)]`` — suppress unused arg lint
- ``[unsafe_operation]`` — calling requires ``unsafe`` block
- ``[no_aot]`` — disable AOT for this function
- ``[jit]`` — request JIT compilation

Combining annotations
======================

Separate multiple annotations with commas::

  [export, sideeffects]
  def annotated_func() { ... }

Struct annotations
==================

- ``[safe_when_uninitialized]`` — zero-filled memory is valid
- ``[cpp_layout]`` — C++ struct alignment
- ``[persistent]`` — survives context reset

Private functions
=================

Use the ``private`` keyword::

  def private helper() : string {
      return "module-private"
  }

Compiler options
================

Set at the top of a file::

  options gen2
  options no_unused_function_arguments
  options stack = 32768

Key options:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Option
     - Description
   * - ``gen2``
     - Enable gen2 syntax
   * - ``no_unused_function_arguments``
     - Unused args become errors
   * - ``strict_smart_pointers``
     - Strict smart pointer checks (default: on)
   * - ``no_global_variables``
     - Disallow module-level globals
   * - ``unsafe_table_lookup``
     - ``tab[key]`` requires unsafe (default: on)
   * - ``stack = N``
     - Stack size in bytes
   * - ``no_aot``
     - Disable AOT for module
   * - ``lint``
     - Enable/disable all lint checks
   * - ``no_deprecated``
     - Deprecated usage becomes error

.. seealso::

   :ref:`Annotations <annotations>`, :ref:`Options <options>` in the
   language reference.

   Full source: :download:`tutorials/language/25_annotations_and_options.das <../../../../tutorials/language/25_annotations_and_options.das>`

   Next tutorial: :ref:`tutorial_contracts`
