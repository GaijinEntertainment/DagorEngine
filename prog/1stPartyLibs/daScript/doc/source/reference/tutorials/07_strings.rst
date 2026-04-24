.. _tutorial_strings:

========================
Strings
========================

.. index::
    single: Tutorial; Strings

This tutorial covers string literals, escape sequences, string interpolation,
character access, slicing, type conversions, case conversion, searching,
padding, trimming, formatted output, and utilities from the ``strings`` and
``daslib/strings_boost`` modules.

String literals
===============

String literals are enclosed in double quotes.  Standard escape sequences are
supported:

.. code-block:: das

    print("tab:\there\n")
    print("newline:\nhere\n")
    print("backslash: \\\n")
    print("quote: \"\n")

String interpolation
====================

Expressions inside ``{ }`` are evaluated and inserted:

.. code-block:: das

    let name = "daslang"
    let version = 0.6
    print("{name} version {version}\n")

Length and character access
===========================

``length`` returns the number of bytes.  ``character_at`` returns the integer
character code at a given index.  Convert back with ``to_char``:

.. code-block:: das

    let text = "Hello"
    print("{length(text)}\n")     // 5

    let ch = character_at(text, 0)
    print("{ch}\n")               // 72  (ASCII for 'H')
    print("{to_char(ch)}\n")      // H

Slicing
=======

``slice`` extracts a substring by start and end indices:

.. code-block:: das

    let phrase = "Hello, World!"
    print("{slice(phrase, 0, 5)}\n")   // Hello
    print("{slice(phrase, 7)}\n")      // World!

Comparison
==========

Strings support the usual comparison operators:

.. code-block:: das

    "abc" == "abc"   // true
    "abc" < "xyz"    // true

Use ``compare_ignore_case`` for case-insensitive comparison.  It returns 0 when
strings are equal ignoring case:

.. code-block:: das

    compare_ignore_case("Hello", "hello")   // 0 (equal)
    compare_ignore_case("abc", "XYZ")       // negative (a < X)

Case conversion
===============

``to_upper`` and ``to_lower`` create new strings with changed case.
``capitalize`` uppercases the first character and lowercases the rest:

.. code-block:: das

    to_upper("hello")              // "HELLO"
    to_lower("WORLD")              // "world"
    capitalize("hello world")      // "Hello world"

Converting to and from strings
==============================

``string(value)`` converts numeric types to a string. For booleans, use string
interpolation:

.. code-block:: das

    let numStr = string(42)      // "42"
    let fltStr = string(3.14)    // "3.14"

    // For booleans, use interpolation:
    let flag = true
    print("{flag}\n")            // true

To convert strings to numbers, ``require strings`` and use ``to_int`` /
``to_float``:

.. code-block:: das

    require strings

    let n = to_int("123")       // 123
    let f = to_float("3.14")    // 3.14

Note: ``int("123")`` does **not** work — you must use ``to_int``.

Searching strings
=================

``find`` returns the index of the first occurrence, or ``-1`` if not found.
``contains`` (from ``daslib/strings_boost``) returns a boolean.  ``count``
counts non-overlapping occurrences.  ``last_index_of`` finds the last
occurrence:

.. code-block:: das

    let sentence = "the quick brown fox"
    find(sentence, "quick")            // 4
    find(sentence, "slow")             // -1

    contains(sentence, "quick")        // true
    contains(sentence, "slow")         // false

    count("abcabcabc", "abc")          // 3

    last_index_of("one/two/three", "/")  // 7

Prefix, suffix, and trimming
=============================

``starts_with`` and ``ends_with`` test for prefixes and suffixes.
``trim_prefix`` and ``trim_suffix`` remove them if present.
``strip`` removes leading and trailing whitespace:

.. code-block:: das

    starts_with("image.png", "image")           // true
    ends_with("image.png", ".png")              // true

    trim_prefix("Hello World", "Hello ")        // "World"
    trim_suffix("file.txt", ".txt")             // "file"
    trim_suffix("file.txt", ".png")             // "file.txt" (unchanged)

    strip("  hello  ")                          // "hello"

``is_null_or_whitespace`` checks if a string is empty or contains only
whitespace:

.. code-block:: das

    is_null_or_whitespace("")     // true
    is_null_or_whitespace("  ")   // true
    is_null_or_whitespace("hi")   // false

Padding
=======

``pad_left`` and ``pad_right`` pad a string to a minimum width with a fill
character (default is space):

.. code-block:: das

    pad_left("42", 6)          // "    42"
    pad_left("42", 6, '0')    // "000042"
    pad_right("hi", 6)        // "hi    "
    pad_right("hi", 6, '.')   // "hi...."

Join and split
==============

``join`` concatenates an array of strings with a separator.
``split`` splits a string by a delimiter:

.. code-block:: das

    let parts = fixed_array("one", "two", "three")
    join(parts, ", ")                    // "one, two, three"

    var items <- split("apple,banana,cherry", ",")
    // ["apple", "banana", "cherry"]

    replace("aabbcc", "bb", "XX")       // "aaXXcc"

Formatted output with fmt
=========================

``fmt`` uses ``{fmt}``-library style format specifiers (not ``printf``-style).
It returns a formatted string:

.. code-block:: das

    fmt(":d", 42)          // "42"
    fmt(":05d", 7)         // "00007"
    fmt(":x", 255)         // "ff"
    fmt(":.2f", 3.14159)   // "3.14"

Building strings
================

``build_string`` constructs strings efficiently using a writer:

.. code-block:: das

    let result = build_string() $(var writer) {
        write(writer, "Name: ")
        write(writer, "Alice")
        write(writer, ", Score: ")
        write(writer, string(100))
    }
    print("{result}\n")   // "Name: Alice, Score: 100"

Running the tutorial
====================

::

  daslang.exe tutorials/language/07_strings.das

.. seealso::

    Full source: :download:`tutorials/language/07_strings.das <../../../../tutorials/language/07_strings.das>`

    Next tutorial: :ref:`tutorial_structs`

    :ref:`string_builder` in the language reference
