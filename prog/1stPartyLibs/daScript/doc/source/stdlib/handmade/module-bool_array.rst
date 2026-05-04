The BOOL_ARRAY module provides a compact boolean array implementation using
bit-packing. Each boolean value uses a single bit instead of a byte,
providing an 8x memory reduction compared to ``array<bool>``.

All functions and symbols are in "bool_array" module, use require to get access to it.

.. code-block:: das

    require daslib/bool_array
