The MATH_BOOST module adds geometric types (``AABB``, ``AABR``, ``Ray``),
angle conversion (``degrees``, ``radians``), intersection tests, color space
conversion (``linear_to_SRGB``, ``RGBA_TO_UCOLOR``), and view/projection
matrix construction (``look_at_lh``, ``perspective_rh``).

All functions and symbols are in "math_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/math_boost

Example:

.. code-block:: das

    require daslib/math_boost

        [export]
        def main() {
            print("degrees(PI) = {degrees(PI)}\n")
            print("radians(180) = {radians(180.0)}\n")
            var box = AABB(min = float3(0), max = float3(10))
            print("box = ({box.min}) - ({box.max})\n")
        }
        // output:
        // degrees(PI) = 180
        // radians(180) = 3.1415927
        // box = (0,0,0) - (10,10,10)
