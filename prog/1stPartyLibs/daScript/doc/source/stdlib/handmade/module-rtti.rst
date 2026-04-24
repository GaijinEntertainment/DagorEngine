The RTTI module exposes runtime type information and program introspection facilities.
It allows querying module structure, type declarations, function signatures, annotations,
and other compile-time metadata at runtime. Used primarily by macro libraries and
code generation tools.

All functions and symbols are in "rtti" module, use require to get access to it.

.. code-block:: das

    require daslib/rtti
