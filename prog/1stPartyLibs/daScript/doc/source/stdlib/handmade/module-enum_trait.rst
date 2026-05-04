The ENUM_TRAIT module provides reflection utilities for enumerations: iterating
over all values, converting between enum values and strings, and building
lookup tables. The ``[string_to_enum]`` annotation generates a string constructor
for the annotated enum type.

All functions and symbols are in "enum_trait" module, use require to get access to it.

.. code-block:: das

    require daslib/enum_trait

Example:

.. code-block:: das

    require daslib/enum_trait

        enum Color {
            red
            green
            blue
        }

        [export]
        def main() {
            print("{Color.green}\n")
            let c = to_enum(type<Color>, "blue")
            print("{c}\n")
            let bad = to_enum(type<Color>, "purple", Color.red)
            print("fallback = {bad}\n")
        }
        // output:
        // green
        // blue
        // fallback = red
