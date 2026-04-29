The DYNAMIC_CAST_RTTI module implements runtime dynamic casting between class
types using RTTI information. It provides safe downcasting with null results
on type mismatch, similar to C++ ``dynamic_cast``.

All functions and symbols are in "dynamic_cast_rtti" module, use require to get access to it.

.. code-block:: das

    require daslib/dynamic_cast_rtti
