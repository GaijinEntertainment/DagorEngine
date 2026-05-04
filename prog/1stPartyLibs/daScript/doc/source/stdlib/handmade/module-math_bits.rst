The MATH_BITS module provides bit manipulation functions for floating point
numbers, including type punning between integer and float representations,
and efficient integer math operations like ``int_bits_to_float`` and
``float_bits_to_int``.

All functions and symbols are in "math_bits" module, use require to get access to it.

.. code-block:: das

    require daslib/math_bits

Example:

.. code-block:: das

    require daslib/math_bits

        [export]
        def main() {
            let f = uint_bits_to_float(0x3F800000u)
            print("uint_bits_to_float(0x3F800000) = {f}\n")
            let back = float_bits_to_uint(1.0)
            print("float_bits_to_uint(1.0) = {back}\n")
        }
        // output:
        // uint_bits_to_float(0x3F800000) = 1
        // float_bits_to_uint(1.0) = 0x3f800000
