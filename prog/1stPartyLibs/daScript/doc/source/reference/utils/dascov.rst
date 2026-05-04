.. _utils_dascov:

.. index::
   single: Utils; dascov
   single: Utils; Code Coverage
   single: Utils; LCOV

=====================================
 dascov --- Code Coverage
=====================================

dascov is a command-line tool for measuring code coverage of daslang
programs.  It instruments code at compile time via ``daslib/coverage``,
runs it, and reports per-file, per-function, and branch coverage.

.. contents::
   :local:
   :depth: 2


Quick start
===========

Measure coverage and print a brief summary::

   daslang utils/dascov/main.das -- - path/to/script.das

Measure coverage and write an LCOV report::

   daslang utils/dascov/main.das -- coverage.lcov path/to/script.das

View a summary of an existing LCOV file::

   daslang utils/dascov/main.das -- coverage.lcov


Usage modes
===========

dascov has two modes of operation: **instrument and measure** (runs a
daslang program with coverage enabled) and **LCOV summary** (parses an
existing LCOV file and prints a summary).

Instrument and measure
----------------------

::

   daslang utils/dascov/main.das -- <output> <input_file> [<args>...]

- ``<output>`` -- coverage destination:

  - ``-`` -- print a brief summary to stdout.
  - ``<filename>`` -- write a full LCOV-format report to the file.

- ``<input_file>`` -- the ``.das`` file to instrument and run.
- ``<args>`` -- any remaining arguments are forwarded to the target
  script as its command-line arguments.

dascov compiles the target file with coverage instrumentation enabled,
runs its ``main`` function, then collects and outputs the results.

LCOV summary
------------

::

   daslang utils/dascov/main.das -- <lcov_file> [--exclude <prefix>]...

Parses an existing LCOV coverage file (e.g. produced by
``dastest --cov-path``) and prints a human-readable summary to stdout.

- ``<lcov_file>`` -- path to the LCOV file.
- ``--exclude <prefix>`` -- skip files whose path starts with
  *prefix*.  Can be repeated to exclude multiple prefixes.

Example::

   daslang utils/dascov/main.das -- coverage.lcov --exclude tests/ --exclude daslib/


Output formats
==============

Brief summary (stdout)
----------------------

When the output is ``-``, dascov prints a columnar summary with
per-file and per-function hit/miss counts::

   Coverage Summary
   ==============================================
   File            Hits      Missed    Coverage
   ----------------------------------------------
   main.das        3         1         75%
     main          3         1         75%
   ----------------------------------------------
   TOTAL           3         1         75%

LCOV format (file)
------------------

When the output is a filename, dascov writes standard
`LCOV <https://github.com/linux-test-project/lcov>`_ trace data.
The format includes:

- ``SF:<source_file>`` -- source file path
- ``FN:<line>,<function>`` -- function definition
- ``FNDA:<hits>,<function>`` -- function hit count
- ``FNF:<count>`` / ``FNH:<count>`` -- functions found / hit
- ``BRDA:<line>,<block>,<branch>,<hits>`` -- branch data
- ``BRF:<count>`` / ``BRH:<count>`` -- branches found / hit
- ``DA:<line>,<hits>`` -- line hit count
- ``LF:<count>`` / ``LH:<count>`` -- lines found / hit

This output is compatible with standard LCOV tools, IDE coverage
plugins, and CI/CD integrations.


Integration with dastest
========================

dastest can generate LCOV coverage data directly via the
``--cov-path`` flag::

   daslang dastest/dastest.das -- --test tests/ --cov-path coverage.lcov

Each test file's coverage is appended to the output file.  After the
run, use dascov to view a summary::

   daslang utils/dascov/main.das -- coverage.lcov --exclude tests/

This workflow gives you coverage of your library code exercised by
your test suite, excluding the test files themselves.


Programmatic use
================

The underlying ``daslib/coverage`` module can be used directly in
daslang code.  Adding ``require daslib/coverage`` to a program enables
compile-time coverage instrumentation via an ``[infer_macro]``.

Key functions:

- ``fill_coverage_report(dest, name)`` -- fill a string with LCOV
  data (for cross-context use via ``invoke_in_context``).
- ``fill_coverage_summary(dest, per_function)`` -- fill a string with
  a brief summary.
- ``summary_from_lcov_file(filename, excludes)`` -- parse an LCOV file
  and return a summary string.


.. seealso::

   :ref:`utils_dastest` -- the test framework (generates LCOV via ``--cov-path``)

   ``daslib/coverage`` -- the coverage instrumentation module
