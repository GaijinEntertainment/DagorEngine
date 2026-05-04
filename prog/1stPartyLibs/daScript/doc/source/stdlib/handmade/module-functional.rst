The FUNCTIONAL module implements lazy iterator adapters and higher-order
function utilities including ``filter``, ``map``, ``reduce``, ``fold``,
``scan``, ``flatten``, ``flat_map``, ``enumerate``, ``chain``, ``pairwise``,
``iterate``, ``islice``, ``cycle``, ``repeat``, ``sorted``, ``sum``,
``any``, ``all``, ``tap``, ``for_each``, ``find``, ``find_index``, and
``partition``.

See :ref:`tutorial_functional` for a hands-on tutorial.

All functions and symbols are in "functional" module, use require to get access to it.

.. code-block:: das

    require daslib/functional

Example:

.. code-block:: das

    require daslib/functional

        [export]
        def main() {
            var src <- [iterator for (x in range(6)); x]
            var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })
            for (v in evens) {
                print("{v} ")
            }
            print("\n")
        }
        // output:
        // 0 2 4
