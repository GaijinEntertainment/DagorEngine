.. _tutorial_testing:

========================
Testing with dastest
========================

.. index::
    single: Tutorial; Testing
    single: Tutorial; dastest
    single: Tutorial; Assertions

This tutorial covers the daslang testing framework: writing tests with
``[test]``, assertion functions, sub-tests, and running tests.

Setup
=====

Import the testing framework::

  require dastest/testing_boost public

Writing tests
=============

Annotate functions with ``[test]``. Each test receives a ``T?`` context::

  [test]
  def test_arithmetic(t : T?) {
      t |> equal(2 + 2, 4)
      t |> equal(3 * 3, 9, "multiplication")
  }

Assertions
==========

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Function
     - Behavior
   * - ``t |> equal(a, b)``
     - Check ``a == b``; non-fatal on failure
   * - ``t |> strictEqual(a, b)``
     - Check ``a == b``; **fatal** on failure (stops test)
   * - ``t |> success(cond)``
     - Check condition is truthy; non-fatal
   * - ``t |> failure("msg")``
     - Always fails; non-fatal
   * - ``t |> accept(value)``
     - Suppress unused warnings (no assertion)

All assertion functions accept an optional message string as the last
argument.

Sub-tests
=========

Group related checks with ``run``::

  [test]
  def test_strings(t : T?) {
      t |> run("concat") @@(t : T?) {
          t |> equal("ab" + "cd", "abcd")
      }
      t |> run("length") @@(t : T?) {
          t |> equal(length("hello"), 5)
      }
  }

Use ``@@(t : T?) { ... }`` (local function) for the callback.

Running tests
=============

From the command line::

  daslang.exe dastest/dastest.das -- --test path/to/test.das

To test an entire directory::

  daslang.exe dastest/dastest.das -- --test path/to/tests/

Folder filtering with ``.das_test``
====================================

When ``dastest`` scans a directory, it visits all subfolders by default.
A ``.das_test`` file at the root of the test path lets you control which
folders are visited.

If ``dastest`` finds a ``.das_test`` file in the ``--test`` directory, it
compiles and simulates it, then calls its ``can_visit_folder`` function for
every subfolder it encounters during scanning. The function receives the
folder name and a ``bool`` pointer — set it to ``false`` to skip the folder.
If the function returns without writing to the pointer, the folder is visited
(default ``true``).

Here is the ``.das_test`` used by the main ``tests/`` directory:

.. code-block:: das

  options gen2
  options indenting = 4

  require daslib/rtti
  require strings

  [export, pinvoke]
  def can_visit_folder(folder_name : string; var result : bool?) {
      if (folder_name == "dasHV" || folder_name == "dasPUGIXML") {
          *result = false
          return
      }
      if (folder_name == "jit_tests" || folder_name == ".jitted_scripts") {
          let args <- get_command_line_arguments()
          for (arg in args) {
              if (arg == "--use-aot") {
                  *result = false
                  return
              }
          }
      }
  }

The function must be ``[export, pinvoke]`` because ``dastest`` calls it from
a different context via ``invoke_in_context``.

``dasHV`` and ``dasPUGIXML`` are always skipped from the main test scan
because their boost modules register global macros that can interfere with
subsequent compilations. Run them separately with
``--test tests/dasHV`` or ``--test tests/dasPUGIXML``.

``has_module`` (from the ``rtti`` module) can be used in ``.das_test`` to
check whether a module is linked into the current binary. For example,
``has_module("dashv")`` returns ``true`` only when dasHV is enabled.

The ``get_command_line_arguments()`` call reads the command-line arguments
passed to the runner. This example uses it to skip JIT-only test folders
when running in AOT mode (``--use-aot``).

Key points:

- The ``.das_test`` file is a regular daslang script — it can ``require``
  modules and use any logic.
- The file is only loaded when ``dastest`` scans a **directory**. Running
  ``dastest`` on a single ``.das`` file bypasses ``.das_test`` entirely.
- Files whose names start with ``_`` are still skipped regardless of
  ``.das_test`` (this is a file-level convention, not a folder one).
- If no ``.das_test`` file exists, all subfolders are visited.

.. seealso::

   Full source: :download:`tutorials/language/27_testing.das <../../../../tutorials/language/27_testing.das>`

   Next tutorial: :ref:`tutorial_linq`
