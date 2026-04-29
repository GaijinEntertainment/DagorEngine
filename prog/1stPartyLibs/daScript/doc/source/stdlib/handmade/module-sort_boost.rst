The SORT_BOOST module provides the ``qsort`` macro that uniformly sorts
built-in arrays, dynamic arrays, and C++ handled vectors using the same
syntax. It automatically wraps handled types in ``temp_array`` as needed.

All functions and symbols are in "sort_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/sort_boost
