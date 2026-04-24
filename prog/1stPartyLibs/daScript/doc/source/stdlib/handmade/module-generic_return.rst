The GENERIC_RETURN module provides the ``[generic_return]`` annotation that
allows generic functions to automatically deduce their return type from
the body. This simplifies writing generic utility functions by eliminating
explicit return type specifications.

All functions and symbols are in "generic_return" module, use require to get access to it.

.. code-block:: das

    require daslib/generic_return
