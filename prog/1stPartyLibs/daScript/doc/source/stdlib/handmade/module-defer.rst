The DEFER module implements the ``defer`` pattern — the ability to schedule cleanup
code to run at scope exit, similar to Go's ``defer``. The deferred block is moved
to the ``finally`` section of the enclosing scope at compile time.

All functions and symbols are in "defer" module, use require to get access to it.

.. code-block:: das

    require daslib/defer

Example:

.. code-block:: das

    require daslib/defer

        [export]
        def main() {
            print("start\n")
            defer() {
                print("cleanup runs last\n")
            }
            print("middle\n")
        }
        // output:
        // start
        // middle
        // cleanup runs last
