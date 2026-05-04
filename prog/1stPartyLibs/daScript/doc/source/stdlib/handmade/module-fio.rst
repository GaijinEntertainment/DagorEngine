The FIO module implements file input/output and filesystem operations.
It provides functions for reading and writing files (``fopen``, ``fread``, ``fwrite``),
directory management (``mkdir``, ``dir``), path manipulation (``join_path``,
``basename``, ``dirname``), and file metadata queries (``stat``, ``file_time``).

All functions and symbols are in "fio" module, use require to get access to it.

.. code-block:: das

    require daslib/fio

Example:

.. code-block:: das

    require daslib/fio

        [export]
        def main() {
            let fname = "_test_fio_tmp.txt"
            fopen(fname, "wb") $(f) {
                fwrite(f, "hello, daslang!")
            }
            fopen(fname, "rb") $(f) {
                let content = fread(f)
                print("{content}\n")
            }
            remove(fname)
        }
        // output:
        // hello, daslang!
