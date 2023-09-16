options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers

module archive shared private

require daslib/contracts
require daslib/apply
require daslib/templates
require math
require strings
require rtti

class public Serializer
    //! Base class for serializers.
    def abstract write ( bytes:void? implicit; size:int ) : bool    //! Write binary data to stream.
    def abstract read ( bytes:void? implicit; size:int ) : bool     //! Read binary data from stream.
    def abstract error ( code:string ) : void                       //! Report error to the archive
    def abstract OK : bool                                          //! Return status of the archive

class public MemSerializer : Serializer
    //! This serializer stores data in memory (in the array<uint8>)
    def MemSerializer
        //! Initialize the serializer for reading or writing.
        pass    // writing
    def MemSerializer ( from:array<uint8> )
        data := from
    def extractData : array<uint8>
        //! Extract the data from the serializer.
        return <- data
    def getCopyOfData : array<uint8>
        //! Returns copy of the data from the seiralizer.
        var cdata := data
        return <- cdata
    def getLastError
        //! Returns last serialization error.
        return lastError
    def override OK
        //! Implements 'OK' method, which returns true if the serializer is in a valid state.
        return lastError==""
    def override write ( bytes:void? implicit; size:int ) : bool
        //! Appends bytes at the end of the data.
        let pos = length(data)
        data |> resize_no_init(pos + size)
        unsafe
            memcpy ( addr(data[pos]), bytes, size)
        return true
    def override read ( bytes:void? implicit; size:int ) : bool
        //! Reads bytes from data, advances the reading position.
        let newOffset = readOffset + size
        let maxOffset = length(data)
        if newOffset > maxOffset
            error("reading past the end of stream")
            readOffset = maxOffset
            return false
        unsafe
            memcpy ( bytes, addr(data[readOffset]), size)
        readOffset = newOffset
        return true
    def override error ( code:string ) : void
        //! Sets the last error code.
        lastError = code
    private data:array<uint8>
    private readOffset:int
    private lastError:string

struct public Archive
    //! Archive is a combination of serialization stream, and state (version, and reading status).
    version : uint
    reading : bool
    stream  : Serializer?

def public serialize_raw ( var arch:Archive; var value : auto(TT)& )
    //! Serialize raw data (straight up bytes for raw pod)
    if arch.reading
        arch.stream->read(unsafe(addr(value)), typeinfo(sizeof value))
    else
        arch.stream->write(unsafe(addr(value)), typeinfo(sizeof value))

def public read_raw ( var arch:Archive; var value : auto(TT)& )
    //! Read raw data (straight up bytes for raw pod)
    assert(arch.reading)
    arch.stream->read(unsafe(addr(value)), typeinfo(sizeof value))

def public write_raw ( var arch:Archive; var value : auto(TT)& )
    //! Write raw data (straight up bytes for raw pod)
    assert(!arch.reading)
    arch.stream->write(unsafe(addr(value)), typeinfo(sizeof value))

[expect_any_enum(value) || expect_any_workhorse_raw(value) || expect_value_handle(value)]
def public serialize ( var arch:Archive; var value : auto(TT)& )
    //! Serializes structured data, based on the `value` type.
    arch |> serialize_raw(value)

[expect_any_function(value)]
def public serialize ( var arch:Archive; var value:auto(TT)& )
    if arch.reading
        var mnh : uint64
        arch |> read_raw(mnh)
        if mnh != 0ul
            unsafe
                value = reinterpret<TT-#>(get_function_by_mangled_name_hash(mnh))
    else
        var mnh = get_function_mangled_name_hash(value)
        arch |> write_raw(mnh)

def public serialize ( var arch:Archive; var value : float3x3 )
    arch |> serialize_raw(value)

def public serialize ( var arch:Archive; var value : float3x4 )
    arch |> serialize_raw(value)

def public serialize ( var arch:Archive; var value : float4x4 )
    arch |> serialize_raw(value)

[expect_any_struct(value)]
def public serialize ( var arch:Archive; var value : auto(TT)& )
    if arch.reading
        delete value
    apply(value) <| $ [unused_argument(name)] ( name:string; field )
        arch |> _::serialize(field)

[expect_any_tuple(value)]
def public serialize ( var arch:Archive; var value : auto(TT)& )
    if arch.reading
        delete value
    apply(value) <| $ [unused_argument(name)] ( name:string; field )
        arch |> _::serialize(field)

[expect_any_variant(value)]
def public serialize ( var arch:Archive; var value : auto(TT)& )
    if arch.reading
        delete value
        var index : int
        arch |> read_raw(index)
        unsafe
            value |> set_variant_index(index)
    else
        var index = variant_index(value)
        arch |> write_raw(index)
    apply(value) <| $ [unused_argument(name)] ( name:string; field )
        arch |> _::serialize(field)

def public serialize ( var arch:Archive; var value : auto(TT)[] )
    for element in value
        arch |> _::serialize(element)

def public serialize ( var arch:Archive; var value : array<auto(TT)> )
    if arch.reading
        var len : int
        unsafe
            delete value
        arch |> read_raw(len)
        value |> resize(len)
        for element in value
            arch |> _::serialize(element)
    else
        var len = length(value)
        arch |> write_raw(len)
        for element in value
            arch |> _::serialize(element)

def public serialize ( var arch:Archive; var value : table<auto(KT);auto(VT)> )
    if arch.reading
        var len : int
        arch |> read_raw(len)
        delete value
        for _ in range(len)
            var k : KT-const-&-#
            arch |> _::serialize(k)
            arch |> _::serialize(value[k])
    else
        var len = length(value)
        arch |> write_raw(len)
        for k,v in keys(value),values(value)
            var K = k
            arch |> _::serialize(K)
            arch |> _::serialize(v)

def public serialize ( var arch:Archive; var value : string& )
    unsafe
        if arch.reading
            var len: int
            arch |> read_raw(len)
            if len > 0
                var tbuf: array<uint8>
                tbuf |> resize_no_init(len+1)                           // todo: implement preallocat string with raii
                arch.stream->read(addr(tbuf[0]), len)
                tbuf[len] = 0u8
                value = clone_string(reinterpret<string#> addr(tbuf[0]))
                delete tbuf
            else
                value = ""
        else
            var len = length(value)
            arch |> write_raw(len)
            if len > 0
                arch.stream->write(reinterpret<void?> value, len)

def public serialize ( var arch:Archive; var value:auto(TT)? )
    if arch.reading
        value = null

def public mem_archive_save ( var t:auto& )
    //! Saves the object to a memory archive. Result is array<uint8> with the serialized data.
    var mems = new MemSerializer()
    var sarch = [[Archive reading=false, stream=mems]]
    sarch |> serialize(t)
    return <- mems->extractData()

def public mem_archive_load ( var data:array<uint8>; var t:auto&; canfail:bool=false ) : bool
    //! Loads the object from a memory archive. `data` is the array<uint8> with the serialized data, returned from `mem_archive_save`.
    var memr = new MemSerializer(data)
    var rarch = [[Archive reading=true, stream=memr]]
    rarch |> serialize(t)
    if !memr->OK()
        if !canfail
            panic(memr->getLastError())
        return false
    return true
