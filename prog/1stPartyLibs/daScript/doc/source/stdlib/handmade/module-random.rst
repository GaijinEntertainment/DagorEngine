The RANDOM module implements pseudo-random number generation using a linear
congruential generator with vectorized state (``int4``). It provides integer,
float, and vector random values, as well as geometric sampling (unit vectors,
points in spheres and disks).

All functions and symbols are in "random" module, use require to get access to it.

.. code-block:: das

    require daslib/random

Example:

.. code-block:: das

    require daslib/random

        [export]
        def main() {
            var seed = random_seed(12345)
            print("int: {random_int(seed)}\n")
            print("float: {random_float(seed)}\n")
            print("float: {random_float(seed)}\n")
        }
        // output:
        // int: 7584
        // float: 0.5848567
        // float: 0.78722495
