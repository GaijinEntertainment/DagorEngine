The BUILTIN module contains core runtime functions available in all daslang programs
without explicit ``require``. It includes:

- Heap and memory management (``heap_bytes_allocated``, ``heap_report``, ``memory_report``)
- Debug output (``print``, ``debug``, ``stackwalk``)
- Panic and error handling (``panic``, ``terminate``, ``assert``)
- Pointer and memory operations (``intptr``, ``malloc``, ``free``)
- Profiling (``profile``)
- Type conversion (``string``)

All functions and symbols are in "builtin" module, use require to get access to it.

.. code-block:: das

    require builtin

Example:

.. code-block:: das

    [export]
        def main() {
            print("hello, world!\n")
            assert(1 + 1 == 2)
            let s = string(42)
            print("string(42) = {s}\n")
            let name = "daslang"
            print("welcome to {name}\n")
            var arr : array<int>
            arr |> push(10)
            arr |> push(20)
            print("length = {length(arr)}\n")
            print("arr[0] = {arr[0]}\n")
        }
        // output:
        // hello, world!
        // string(42) = 42
        // welcome to daslang
        // length = 2
        // arr[0] = 10
