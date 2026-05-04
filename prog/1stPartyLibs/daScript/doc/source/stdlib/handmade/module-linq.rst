The LINQ module provides query-style operations on sequences: filtering
(``where_``), projection (``select``), sorting (``order``, ``order_by``),
deduplication (``distinct``), pagination (``skip``, ``take``), aggregation
(``sum``, ``average``, ``aggregate``), and element access (``first``, ``last``).

See also :doc:`linq_boost` for pipe-syntax macros with underscore shorthand.
See :ref:`tutorial_linq` for a hands-on tutorial.

All functions and symbols are in "linq" module, use require to get access to it.

.. code-block:: das

    require daslib/linq

Example:

.. code-block:: das

    require daslib/linq

        [export]
        def main() {
            var src <- [iterator for (x in range(10)); x]
            var evens <- where_(src, $(x : int) : bool { return x % 2 == 0; })
            for (v in evens) {
                print("{v} ")
            }
            print("\n")
        }
        // output:
        // 0 2 4 6 8
