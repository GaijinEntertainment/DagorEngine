The SAFE_ADDR module provides compile-time checked pointer operations.
``safe_addr`` returns a temporary pointer to a variable only if the compiler
can verify the pointer will not outlive its target. This prevents dangling
pointer bugs without runtime overhead.

All functions and symbols are in "safe_addr" module, use require to get access to it.

.. code-block:: das

    require daslib/safe_addr
