The STRINGS_BOOST module extends string handling with splitting, joining,
padding, character replacement, and edit distance computation.

All functions and symbols are in "strings_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/strings_boost

Example:

.. code-block:: das

    require daslib/strings_boost

        [export]
        def main() {
            let parts = split("one,two,three", ",")
            print("split: {parts}\n")
            print("join: {join(parts, " | ")}\n")
            print("[{wide("hello", 10)}]\n")
            print("distance: {levenshtein_distance("kitten", "sitting")}\n")
        }
        // output:
        // split: [[ one; two; three]]
        // join: one | two | three
        // [hello     ]
        // distance: 3
