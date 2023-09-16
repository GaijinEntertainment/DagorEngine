options indenting = 2
require DagorFiles

[generic]
def df_open(name : string; flags : int; blk : block<(f : DagFile const?) : void>)
  var f = df_open(name, flags)
  invoke(blk, f)
  df_close(f)

[generic]
def df_read(f : DagFile const? const; buf : auto(BufType)- const)
  concept_assert(typeinfo(is_ref_type type<BufType>), "can only read ref type")
  concept_assert(typeinfo(is_raw type<BufType>), "can only read raw pod type")
  return _builtin_df_read(f, buf, typeinfo(sizeof type<BufType>))

[generic]
def df_write(f : DagFile const? const; buf : auto(BufType) const)
  concept_assert(typeinfo(is_ref_type type<BufType>), "can only write ref type")
  concept_assert(typeinfo(is_raw type<BufType>), "can only write raw pod type")
  return _builtin_df_write(f, buf, typeinfo(sizeof type<BufType>))

let
  df_magic = 0x12345678

struct df_header
  magic : uint
  size : int

def df_load(f : DagFile const?; buf : auto(BufType)- const)
  var dfh : df_header
  let r1 = _builtin_df_read(f, dfh, typeinfo(sizeof type<df_header>))
  if r1 < 0
    return false
  if dfh.magic != df_magic
    return false
  var loaded = false
  _builtin_df_load(f, dfh.size) <| $(data : array<uint8>)
    if length(data) != 0
      binary_load(buf, data)
      loaded = true
  return loaded

def df_save(f : DagFile const?; buf : auto(BufType) const)
  var r1, r2 : int
  binary_save(buf) <| $(data)
    let len = length(data)
    let dfh = [[df_header magic=df_magic, size=len]]
    r1 = _builtin_df_write(f, dfh, typeinfo(sizeof type<df_header>))
    r2 = _builtin_df_write(f, data, len)
  if r1 < 0
    return r1
  if r2 < 0
    return r2
  return (r1 + r2)



