The BASE64 module implements Base64 encoding and decoding.
It provides ``base64_encode`` and ``base64_decode`` for converting between binary data
(strings or ``array<uint8>``) and Base64 text representation.

All functions and symbols are in "base64" module, use require to get access to it.

.. code-block:: das

    require daslib/base64

Example:

.. code-block:: das

    require daslib/base64

        [export]
        def main() {
            let encoded = base64_encode("Hello, daslang!")
            print("encoded: {encoded}\n")
            let decoded = base64_decode(encoded)
            print("decoded: {decoded.text}\n")
        }
        // output:
        // encoded: SGVsbG8sIGRhU2NyaXB0IQ==
        // decoded: Hello, daslang!
