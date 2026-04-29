The STATIC_LET module implements the ``static_let`` pattern — local variables
that persist across function calls, similar to C ``static`` variables. The
variable is initialized once on first call and retains its value in subsequent
invocations.

All functions and symbols are in "static_let" module, use require to get access to it.

.. code-block:: das

    require daslib/static_let

Example:

.. code-block:: das

    require daslib/static_let

        def counter() : int {
            static_let() {
                var count = 0
            }
            count ++
            return count
        }

        [export]
        def main() {
            print("{counter()}\n")
            print("{counter()}\n")
            print("{counter()}\n")
        }
        // output:
        // 1
        // 2
        // 3
