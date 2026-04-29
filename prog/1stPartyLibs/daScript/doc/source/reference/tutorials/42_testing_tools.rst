.. _tutorial_testing_tools:

==========================================
Testing Tools (faker + fuzzer)
==========================================

.. index::
    single: Tutorial; Testing Tools
    single: Tutorial; Faker
    single: Tutorial; Fuzzer
    single: Tutorial; Property-Based Testing
    single: Tutorial; Fuzz Testing
    single: Tutorial; Random Test Data

This tutorial covers two testing modules: ``daslib/faker`` for generating
random test data, and ``daslib/fuzzer`` for running stress tests that detect
crashes and invariant violations.

Prerequisites: :ref:`tutorial_testing` (Tutorial 27),
:ref:`tutorial_random` (Tutorial 38).

.. code-block:: das

    require daslib/faker
    require daslib/fuzzer

Faker basics
============

A ``Faker`` struct produces random values for every built-in type.  It wraps
an infinite random ``uint`` iterator internally:

.. code-block:: das

    var fake <- Faker()

    print("{fake |> random_int}\n")      // random int (full range)
    print("{fake |> random_uint}\n")     // random uint (full range)
    print("{fake |> random_float}\n")    // random float (raw bits)
    print("{fake |> random_double}\n")   // random double (raw bits)

    // Random bool (derived from random_uint)
    let b = (fake |> random_uint) % 2u == 0u

    delete fake

.. note::

   ``random_float`` and ``random_double`` generate raw bit patterns — the
   values can be NaN, Inf, or denormalized.  This is intentional for fuzz
   testing.

Random strings and file names
==============================

.. code-block:: das

    var fake <- Faker()
    let s = fake |> any_string()         // random characters
    let fn = fake |> any_file_name       // alphanumeric + underscore + dot
    let ls = fake |> long_string()       // up to max_long_string bytes
    delete fake

* ``any_string`` — random characters up to the regex generator limit
* ``any_file_name`` — file-name-safe characters
* ``long_string`` — may be very long (configurable via ``fake.max_long_string``)

Random vectors
==============

Faker generates vectors of all sizes for every numeric type:

.. code-block:: das

    var fake <- Faker()
    print("{fake |> random_int2}\n")
    print("{fake |> random_int3}\n")
    print("{fake |> random_float2}\n")
    print("{fake |> random_float3}\n")
    delete fake

Random dates
============

Faker generates random dates within a configurable year range:

.. code-block:: das

    var fake <- Faker()
    print("{fake |> date}\n")    // "DayOfWeek, MonthName DD, YYYY"
    print("{fake |> day}\n")     // "DayOfWeek"
    delete fake

Customizing Faker
=================

Faker has configurable fields to control the output:

.. code-block:: das

    var fake <- Faker()
    fake.min_year = 2020u          // restrict year range
    fake.total_years = 5u          // 2020-2025
    fake.max_long_string = 32u     // limit long_string length
    delete fake

fuzz — silent crash detection
=============================

``fuzz(count, block)`` runs a block ``count`` times, catching any panics.
Use it to test that your code handles arbitrary input gracefully:

.. code-block:: das

    def safe_divide(a, b : int) : int {
        if (b == 0) {
            return 0
        }
        return a / b
    }

    var fake <- Faker()
    fuzz(100) {
        let a = fake |> random_int
        let b = fake |> random_int
        safe_divide(a, b)
    }
    delete fake

If any iteration panics, ``fuzz`` catches it silently and continues.

fuzz_debug — verbose crash detection
=====================================

``fuzz_debug`` is identical to ``fuzz`` but does NOT catch panics.  Replace
``fuzz`` with ``fuzz_debug`` when you need to see the actual error message
and stack trace:

.. code-block:: das

    var fake <- Faker()
    fuzz_debug(50) {
        let a = fake |> random_int
        let b = fake |> random_int
        safe_divide(a, b)
    }
    delete fake

Property-based testing
======================

Combine faker + fuzzer for property-based testing: generate random inputs
and verify that invariants always hold:

.. code-block:: das

    def clamp_value(x, lo, hi : int) : int {
        if (x < lo) {
            return lo
        }
        if (x > hi) {
            return hi
        }
        return x
    }

    var fake <- Faker()
    var violations = 0
    fuzz(1000) {
        let x = fake |> random_int
        let result = clamp_value(x, -100, 100)
        if (result < -100 || result > 100) {
            violations ++
        }
    }
    print("violations: {violations}/1000\n")    // 0
    delete fake

Testing string operations
=========================

Faker's ``any_string`` is useful for testing string-processing functions:

.. code-block:: das

    var fake <- Faker()
    var failures = 0
    fuzz(100) {
        let s = fake |> any_string()
        let reversed = reverse_string(s)
        let double_rev = reverse_string(reversed)
        if (double_rev != s) {
            failures ++
        }
    }
    delete fake

Summary
=======

==============================  ================================================
Function                        Description
==============================  ================================================
``Faker()``                     Create faker with default RNG
``fake |> random_int``          Random int (full range)
``fake |> random_uint``         Random uint (full range)
``fake |> random_float``        Random float (raw bits — may be NaN/Inf)
``fake |> any_string()``        Random string
``fake |> any_file_name``       Random file-name-safe string
``fake |> long_string()``       Random byte string (up to ``max_long_string``)
``fake |> date``                Random date string
``fake |> day``                 Random day-of-week string
``fuzz(n) <| block``            Run block n times, catch panics
``fuzz_debug(n) <| block``      Run block n times, panics propagate
==============================  ================================================

.. seealso::

   :ref:`Testing <tutorial_testing>` — testing framework tutorial.

   :ref:`Random Numbers <tutorial_random>` — random number generation.

   Full source: :download:`tutorials/language/42_testing_tools.das <../../../../tutorials/language/42_testing_tools.das>`

   Previous tutorial: :ref:`tutorial_serialization`

   Next tutorial: :ref:`tutorial_interfaces`
