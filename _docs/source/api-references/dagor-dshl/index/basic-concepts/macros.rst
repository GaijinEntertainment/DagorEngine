.. _macros:

======
Macros
======

Macros are declared as:

.. code-block:: c

  macro NAME(argument1, argument2, ...)
    ...
  endmacro

Within macro, all occurences of ``argument1`` would be replaced with argument passed to the macro invokation, including hlsl blocks within macro.

There is no way to undefine macro, but you can define it optionally:

.. code-block:: c

  define_macro_if_not_defined NAME(argument1)
    ...
  endmacro

which will only define macro ``NAME`` if it wasn't defined previously.

We have some naming conventions regarding macros:

- No hlsl code should be in ``INIT_...``  macros.
- If your macro includes other macros, ``USE_...``  and ``INIT_...``  macros should be separated accordingly.
