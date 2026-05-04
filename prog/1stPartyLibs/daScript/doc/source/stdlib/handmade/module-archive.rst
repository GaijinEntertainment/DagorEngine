The ARCHIVE module implements general-purpose serialization infrastructure.
It provides the ``Archive`` type and ``serialize`` functions for reading and
writing binary data. Custom types are supported by implementing ``serialize``
for each type.

All functions and symbols are in "archive" module, use require to get access to it.

.. code-block:: das

    require daslib/archive

To correctly support serialization of the specific type, you need to define and
implement ``serialize`` method for it.
For example this is how DECS implements component serialization:

.. code-block:: das

    def public serialize ( var arch:Archive; var src:Component )
        arch |> serialize(src.name)
        arch |> serialize(src.hash)
        arch |> serialize(src.stride)
        arch |> serialize(src.info)
        invoke(src.info.serializer, arch, src.data)

Example:

.. code-block:: das

    require daslib/archive

        struct Foo {
            a : float
            b : string
        }

        [export]
        def main() {
            var original = Foo(a = 3.14, b = "hello")
            var data <- mem_archive_save(original)
            var loaded : Foo
            data |> mem_archive_load(loaded)
            delete data
            print("a = {loaded.a}, b = {loaded.b}\n")
        }
        // output:
        // a = 3.14, b = hello
