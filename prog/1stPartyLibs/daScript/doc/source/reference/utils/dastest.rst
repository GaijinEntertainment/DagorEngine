.. _utils_dastest:

.. index::
   single: Utils; dastest
   single: Utils; Testing
   single: Utils; Benchmarks

=============================
 dastest --- Test Framework
=============================

dastest is the daslang testing framework, inspired by the
`Go testing package <https://pkg.go.dev/testing>`_.  It discovers
and runs ``[test]`` and ``[benchmark]`` functions in ``.das`` files
and reports results.

.. contents::
   :local:
   :depth: 2


Quick start
===========

.. code-block:: das

   options gen2
   require dastest/testing_boost public

   [test]
   def test_arithmetic(t : T?) {
       t |> equal(2 + 2, 4)
       t |> equal(3 * 3, 9, "multiplication")
   }

Run it::

   daslang dastest/dastest.das -- --test path/to/test.das


Running tests
=============

dastest is itself a daslang script.  All arguments after ``--`` are
passed to dastest.

Running all tests in a file or directory::

   daslang dastest/dastest.das -- --test path/to/tests/

Running all tests **and** benchmarks::

   daslang dastest/dastest.das -- --bench --test path/to/tests/

Running benchmarks **only** (skip tests)::

   daslang dastest/dastest.das -- --bench --test path/to/tests/ --test-names none

Running specific benchmarks by prefix::

   daslang dastest/dastest.das -- --bench --bench-names vector_alloc --test path/to/tests/ --test-names none

Multiple ``--test`` paths can be provided::

   daslang dastest/dastest.das -- --test tests/lang --test tests/daslib


Command-line arguments
======================

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Argument
     - Description
   * - ``--test <path>``
     - Path to a folder or single ``.das`` file to test.  Can be repeated.
   * - ``--test-names <prefix>``
     - Run only top-level tests whose name starts with *prefix*.
       Use ``none`` to skip all tests (useful with ``--bench``).
   * - ``--test-project <path>``
     - A ``.das_project`` file used to compile the tests.
   * - ``--uri-paths``
     - Print URI-style paths (VS Code friendly).
   * - ``--color``
     - Print colored output.
   * - ``--verbose``
     - Print verbose output.
   * - ``--timeout <seconds>``
     - Panic if total test time exceeds *seconds*.
       ``0`` disables the timeout.  Default: 10 minutes.
   * - ``--compile-only``
     - Compile test files without running them.  Useful for checking
       that tests compile after refactoring.
   * - ``--use-aot``
     - Compile and run tests using AOT (ahead-of-time) compilation.
   * - ``--isolated-mode``
     - Run each test file in a separate process.
       Useful for catching native crashes.
   * - ``--isolated-mode-threads <n>``
     - Number of parallel processes in isolated mode.
       Default: ``2 * hardware_threads``.
   * - ``--test-timeout <seconds>``
     - Per-test-file timeout in isolated mode (seconds).
       ``0`` disables.  Default: ``0`` (no per-file timeout).
   * - ``--timing-outliers <n>``
     - After all tests pass, print the *n* slowest test files
       (timing outliers above 2 standard deviations).
   * - ``--bench``
     - Enable benchmark execution (all benchmarks).
   * - ``--bench-names <prefix>``
     - Run only benchmarks whose name starts with *prefix*.
   * - ``--bench-format <format>``
     - Benchmark output format (see `Benchmark output formats`_).
   * - ``--count <n>``
     - Run all benchmarks *n* times (for sample collection).
   * - ``--json-file <path>``
     - Write a JSON test report to *path*.  Contains per-test timing,
       results, errors, and summary counts.  Useful for CI integration.
   * - ``--failures-only``
     - Suppress PASS and RUN output; show only failures and the final
       summary.  Useful for large test runs where only errors matter.
   * - ``--cov-path <path>``
     - Enable code coverage and write an LCOV report to *path*.  Each
       test file's coverage is appended (the file is truncated at the
       start of the run).  Requires ``daslib/coverage``.  The resulting
       file can be viewed with :ref:`utils_dascov` or standard LCOV
       tools.

Internal arguments (used by isolated mode):

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Argument
     - Description
   * - ``--run <path>``
     - Path to a single file to run in isolated mode (used internally).

.. note::

   Benchmarks run only after all tests in the file have passed.  If any
   test fails, benchmarks are skipped.


Writing tests
=============

Import the testing framework and annotate functions with ``[test]``.
Each test function receives a ``T?`` context pointer used for assertions:

.. code-block:: das

   options gen2
   require dastest/testing_boost public

   [test]
   def test_basic(t : T?) {
       t |> equal(2 + 2, 4)
       t |> success(true, "always passes")
   }


Assertions
==========

All assertion functions accept an optional message string as the last
argument.

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


Sub-tests
=========

Group related checks with ``run``:

.. code-block:: das

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


Benchmarks
==========

Annotate functions with ``[benchmark]``.  Each benchmark receives a
``B?`` context.  Use ``b |> run("name")`` to define sub-benchmarks:

.. code-block:: das

   options gen2
   require dastest/testing_boost public

   [benchmark]
   def dyn_array(b : B?) {
       let size = 100

       b |> run("fill_vec") <| $() {
           var vec : array<int>
           for (i in range(size)) {
               vec |> push(i)
           }
       }

       b |> run("fill_vec_reserve") <| $() {
           var vec : array<int>
           vec |> reserve(size)
           for (i in range(size)) {
               vec |> push(i)
           }
       }
   }

The runner measures iterations until the timing is stable and reports
ns/op.


Benchmark output formats
========================

``native`` (default)
   Human-readable columnar output with colored values::

      === RUN BENCHMARK 'dyn_array' [INTERP]
        fill_vec            12345 ns/op     1024 B/op       100 allocs/op
        fill_vec_reserve     3456 ns/op      800 B/op         1 allocs/op

   Columns: ns/op, B/op (heap bytes), allocs/op (heap allocations),
   SB/op (string heap bytes), strings/op (string allocations).
   Zero-allocation columns are highlighted green.

``go``
   `Go benchmark format <https://golang.org/design/14313-benchmark-format>`_,
   tab-separated, compatible with ``benchstat`` and other Go tooling::

      100		12345.0 ns/op	1024 B/op	100 allocs/op	0 SB/op	0 strings/op

``json``
   One JSON object per sub-benchmark (one per line), containing all
   fields from the ``BenchmarkRunStats`` struct::

      {"name":"dyn_array","sub_name":"fill_vec","n":100,"time_ns":1234500,"allocs":100,"heap_bytes":1024,"string_allocs":0,"string_heap_bytes":0,"func_type":"INTERP"}

   Fields:

   - ``name`` -- top-level benchmark function name
   - ``sub_name`` -- sub-benchmark name (from ``b |> run("...")``)
   - ``n`` -- number of iterations
   - ``time_ns`` -- total elapsed time in nanoseconds
   - ``allocs`` -- total heap allocations
   - ``heap_bytes`` -- total heap bytes allocated
   - ``string_allocs`` -- total string heap allocations
   - ``string_heap_bytes`` -- total string heap bytes
   - ``func_type`` -- execution mode (``"INTERP"``, ``"AOT"``, ``"JIT"``)


Folder filtering with ``.das_test``
====================================

When dastest scans a directory, it visits all subfolders by default.
A ``.das_test`` file at the root of the test path lets you control which
folders are visited.

If dastest finds a ``.das_test`` file in the ``--test`` directory, it
compiles and simulates it, then calls its ``can_visit_folder`` function
for every subfolder it encounters.  The function receives the folder
name and a ``bool`` pointer -- set it to ``false`` to skip the folder.
If the function returns without writing to the pointer, the folder is
visited (default ``true``).

.. code-block:: das

   options gen2

   require daslib/rtti
   require strings

   [export, pinvoke]
   def can_visit_folder(folder_name : string; var result : bool?) {
       if (folder_name == "dasHV" || folder_name == "dasPUGIXML") {
           *result = false
           return
       }
   }

The function must be ``[export, pinvoke]`` because dastest calls it
from a different context via ``invoke_in_context``.

``has_module`` (from the ``rtti`` module) can check whether a module
is linked into the current binary.  For example,
``has_module("dashv")`` returns ``true`` only when dasHV is enabled.

Key points:

- The ``.das_test`` file is a regular daslang script -- it can
  ``require`` modules and use any logic.
- The file is only loaded when dastest scans a **directory**.  Running
  dastest on a single ``.das`` file bypasses ``.das_test`` entirely.
- Files whose names start with ``_`` are always skipped.
- If no ``.das_test`` file exists, all subfolders are visited.


Test file conventions
=====================

- Test files live under ``tests/`` or alongside the code they test.
- Each file must ``require dastest/testing_boost public``.
- Use ``options gen2`` at the top.
- Files starting with ``_`` are skipped by dastest.
- Use ``expect`` to suppress specific compilation errors in
  negative-test files.


.. seealso::

   :ref:`tutorial_testing` -- introductory tutorial on writing tests

   ``dastest/testing_boost`` -- the testing boost module (assertions, sub-tests, benchmarks)

   :ref:`utils_dascov` -- standalone code coverage tool (parses LCOV output from ``--cov-path``)
