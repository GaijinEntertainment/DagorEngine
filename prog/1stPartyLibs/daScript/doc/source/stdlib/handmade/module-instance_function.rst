The INSTANCE_FUNCTION module provides the ``[instance_function]`` annotation
for creating bound method-like functions. It captures the ``self`` reference
at call time, enabling object-oriented dispatch patterns in daslang.

All functions and symbols are in "instance_function" module, use require to get access to it.

.. code-block:: das

    require daslib/instance_function
