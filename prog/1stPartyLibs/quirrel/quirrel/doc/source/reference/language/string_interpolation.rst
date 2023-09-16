.. _string_interpolation:


========================
$ - String interpolation
========================

.. index::
    single: String interpolation


The ``$`` special character identifies a string literal as an interpolated string.
An interpolated string is a string literal that might contain interpolation expressions.
String interpolation provides a more readable and convenient syntax to create formatted strings
than a string concatenation or ``subst()`` method.

The following example uses both features to produce the same output:

::

  local x = 123
  local y = 567
  print("x = {0}, sin(y) = {1}".subst(x, math.sin(y)))
  print($"x = {x}, sin(y) = {math.sin(y)}")

Internally string interpolation translates to ``subst()`` call.

Nested interpolation strings are not supported.

To use curly brackets {} inside interpolation string, ``{`` and ``}`` characters should be escaped using ``\``

::

  local foo = 123
  print($"\{ foo = {foo} \}") // will output: { foo = 123 }
