The SOA (Structure of Arrays) module transforms array-of-structures data layouts
into structure-of-arrays layouts for better cache performance. It provides macros
that generate parallel arrays for each field of a structure, enabling SIMD-friendly
data access patterns.

See :ref:`tutorial_soa` for a hands-on tutorial.

All functions and symbols are in "soa" module, use require to get access to it.

.. code-block:: das

    require daslib/soa
