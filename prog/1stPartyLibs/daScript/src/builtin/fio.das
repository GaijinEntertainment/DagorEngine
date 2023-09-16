options indenting=4
options remove_unused_symbols = false

require fio
require strings

typedef
    file = FILE const?

[generic]
def fopen(name:string;mode:string;blk:block<(f:file):void>)
    let f = fopen(name,mode)
    invoke(blk,f)
    if f!=null
        fclose(f)

[generic]
def dir(path:string;blk:block<(filename:string):void>)
    builtin_dir(path,blk)

[generic]
def stat(path:string):FStat
    var fs : FStat
    if stat(path,fs)
        fs.is_valid = true
    return fs

[generic]
def fstat(f:file):FStat
    var fs : FStat
    if fstat(f,fs)
        fs.is_valid = true
    return fs

def fread(f:file; blk:block<(data:string const#)>)
    var stmp = fread(f)
    unsafe
        invoke(blk, reinterpret<string#>(stmp))
        delete_string(stmp)

let
    df_magic = 0x12345678

struct df_header
    magic:uint
    size:int

def fload(file:file; size: int; blk: block<(data: array<uint8>): void>): void
    _builtin_load(file, size, blk)

def fload(f:file;buf:auto(BufType) -const)
    var dfh:df_header
    let r1 = _builtin_read(f,dfh,typeinfo(sizeof type<df_header>))
    if r1<0
        return false
    if dfh.magic!=df_magic
        return false
    var loaded = false
    _builtin_load(f,dfh.size) <| $(data:array<uint8>)
        if length(data)!=0
            binary_load(buf,data)
            loaded = true
    return loaded

def fsave(f:file;buf:auto(BufType)const)
    var r1, r2 : int
    binary_save(buf) <| $(data)
        let len = length(data)
        let dfh = [[df_header magic=df_magic, size=len]]
        r1 = _builtin_write(f,dfh,typeinfo(sizeof type<df_header>))
        unsafe
            r2 = _builtin_write(f,addr(data[0]),len)
    if r1<0
        return r1
    if r2<0
        return r2
    return (r1+r2)

def fread(f:file;buf:auto(BufType) const implicit )
    concept_assert(typeinfo(is_raw buf),"can only fread raw pod")
    unsafe
        return _builtin_read(f, addr(buf), typeinfo(sizeof buf))

def fread(f:file;buf:array<auto(BufType)> const implicit )
    concept_assert(typeinfo(is_raw type<BufType>),"can only save fread pod array")
    unsafe
        return _builtin_read(f, addr(buf[0]), length(buf) *( typeinfo(sizeof type<BufType>)))

def fwrite(f:file;buf:auto(BufType) const implicit )
    concept_assert(typeinfo(is_raw buf),"can only fwrite raw pod")
    unsafe
        return _builtin_write(f, addr(buf), typeinfo(sizeof buf))

def fwrite(f:file;buf:array<auto(BufType)> const implicit )
    concept_assert(typeinfo(is_raw type<BufType>),"can only fwrite pod array")
    unsafe
        return _builtin_write(f, addr(buf[0]), length(buf) *( typeinfo(sizeof type<BufType>)))

