The MATCH module implements pattern matching on variants, structs, tuples,
arrays, and scalar values. Supports variable capture, wildcards, guard
expressions, and alternation. ``static_match`` enforces exhaustive matching
at compile time.

See :ref:`tutorial_pattern_matching` for a hands-on tutorial.

All functions and symbols are in "match" module, use require to get access to it.

.. code-block:: das

    require daslib/match

Example:

.. code-block:: das

    require daslib/match

        enum Color {
            red
            green
            blue
        }

        def describe(c : Color) : string {
            match (c) {
                if (Color.red) { return "red"; }
                if (Color.green) { return "green"; }
                if (_) { return "other"; }
            }
            return "?"
        }

        [export]
        def main() {
            print("{describe(Color.red)}\n")
            print("{describe(Color.green)}\n")
            print("{describe(Color.blue)}\n")
        }
        // output:
        // red
        // green
        // other
