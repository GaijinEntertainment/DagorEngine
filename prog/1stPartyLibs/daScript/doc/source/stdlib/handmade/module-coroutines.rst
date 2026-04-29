The COROUTINES module provides coroutine infrastructure including the
``[coroutine]`` function annotation, ``yield_from`` for delegating to
sub-coroutines, and ``co_await`` for composing asynchronous generators.
Coroutines produce values lazily via ``yield`` and can be iterated with
``for``.

See :ref:`tutorial_iterators_and_generators` for a hands-on tutorial.

All functions and symbols are in "coroutines" module, use require to get access to it.

.. code-block:: das

    require daslib/coroutines

Example:

.. code-block:: das

    require daslib/coroutines

        [coroutine]
        def fibonacci() : int {
            var a = 0
            var b = 1
            while (true) {
                yield a
                let next = a + b
                a = b
                b = next
            }
        }

        [export]
        def main() {
            var count = 0
            for (n in fibonacci()) {
                print("{n} ")
                count ++
                if (count >= 10) {
                    break
                }
            }
            print("\n")
        }
        // output:
        // 0 1 1 2 3 5 8 13 21 34
