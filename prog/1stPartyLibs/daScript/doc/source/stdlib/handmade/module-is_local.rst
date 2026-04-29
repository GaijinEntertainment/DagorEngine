The IS_LOCAL module provides compile-time checks for whether a variable
is locally allocated (on the stack) versus heap-allocated. This enables
writing generic code that optimizes differently based on allocation strategy.

All functions and symbols are in "is_local" module, use require to get access to it.

.. code-block:: das

    require daslib/is_local
