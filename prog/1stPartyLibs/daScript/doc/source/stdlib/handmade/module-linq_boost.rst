The LINQ_BOOST module extends LINQ with pipe-friendly macros using underscore
syntax for inline predicates and selectors. Expressions like
``arr |> _where(_ > 3) |> _select(_ * 2)`` provide concise functional pipelines.

See also :doc:`linq` for the full set of query operations.
See :ref:`tutorial_linq` for a hands-on tutorial.

All functions and symbols are in "linq_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/linq_boost

Example:

.. code-block:: das

    require daslib/linq
        require daslib/linq_boost

        [export]
        def main() {
            var src <- [iterator for (x in range(10)); x]
            var evens <- _where(src, _ % 2 == 0)
            for (v in evens) {
                print("{v} ")
            }
            print("\n")
        }
        // output:
        // 0 2 4 6 8
