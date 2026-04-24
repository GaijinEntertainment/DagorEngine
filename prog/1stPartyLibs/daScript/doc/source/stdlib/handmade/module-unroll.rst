The UNROLL module implements compile-time loop unrolling. The ``unroll``
macro replaces a ``for`` loop with a constant ``range`` bound by stamping
out each iteration as separate inlined code, eliminating loop overhead.

All functions and symbols are in "unroll" module, use require to get access to it.

.. code-block:: das

    require daslib/unroll

Example:

.. code-block:: das

    require daslib/unroll

        [export]
        def main() {
            unroll() {
                for (i in range(4)) {
                    print("step {i}\n")
                }
            }
        }
        // output:
        // step 0
        // step 1
        // step 2
        // step 3
