The archive module implements general purpose serialization infrastructure.

All functions and symbols are in "archive" module, use require to get access to it. ::

    require daslib/archive

To correctly support serialization of the specific type, you need to define and implement `serialize` method for it.
For example this is how DECS implements component serialization: ::

    def public serialize ( var arch:Archive; var src:Component )
        arch |> serialize(src.name)
        arch |> serialize(src.hash)
        arch |> serialize(src.stride)
        arch |> serialize(src.info)
        invoke(src.info.serializer, arch, src.data)

