The ASSERT_ONCE module provides the ``assert_once`` macro — an assertion that
triggers only on its first failure. Subsequent failures at the same location
are silently ignored, preventing assertion storms in loops or frequently
called code.

All functions and symbols are in "assert_once" module, use require to get access to it.

.. code-block:: das

    require daslib/assert_once
