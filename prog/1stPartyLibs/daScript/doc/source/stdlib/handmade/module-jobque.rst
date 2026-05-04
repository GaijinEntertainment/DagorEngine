The JOBQUE module provides low-level job queue and threading primitives.
It includes thread-safe channels for inter-thread communication, lock boxes
for shared data access, job status tracking, and fine-grained thread
management. For higher-level job abstractions, see ``jobque_boost``.

See :ref:`tutorial_jobque` for a hands-on tutorial.

All functions and symbols are in "jobque" module, use require to get access to it.

.. code-block:: das

    require jobque

Example:

.. code-block:: das

    require jobque

        [export]
        def main() {
            with_atomic32() $(counter) {
                counter |> set(10)
                print("value = {counter |> get}\n")
                let after_inc = counter |> inc
                print("after inc = {after_inc}\n")
                let after_dec = counter |> dec
                print("after dec = {after_dec}\n")
            }
        }
        // output:
        // value = 10
        // after inc = 11
        // after dec = 10
