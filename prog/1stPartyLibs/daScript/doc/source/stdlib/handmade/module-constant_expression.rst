The CONSTANT_EXPRESSION module provides the ``[constant_expression]`` function
annotation. Functions marked with this annotation are evaluated at compile
time when all arguments are constants, replacing the call with the computed
result.

All functions and symbols are in "constant_expression" module, use require to get access to it.

.. code-block:: das

    require daslib/constant_expression
