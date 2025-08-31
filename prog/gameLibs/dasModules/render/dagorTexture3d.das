options indenting = 2
require DagorTexture3D

let BAD_TEXTUREID = D3DRESID()

[generic]
def updateData(var buf : Sbuffer; data : auto(DataType); flags : uint)
  static_if typeinfo is_raw(data)
    __builtin_updateData(buf, 0u, typeinfo sizeof(data), unsafe(addr(data)), flags)
  static_elif typeinfo is_any_vector(type<DataType>) || typeinfo is_array(type<DataType>)
    __builtin_updateData(buf, 0u, uint(typeinfo sizeof(data[0]) * _::length(data)), unsafe(addr(data[0])), flags)
    concept_assert(typeinfo is_raw(data[0]), "updateData can write only array of pod")
  else
    concept_assert(false, "updateData supports pod structs or arrays of pod")

[generic]
def updateData(var buf : Sbuffer; data : auto(DataType); src_offset, num_elems : int; flags : uint)
  static_if typeinfo is_any_vector(type<DataType>) || typeinfo is_array(type<DataType>)
    __builtin_updateData(buf, 0u, uint(typeinfo sizeof(data[0]) * num_elems), unsafe(addr(data[src_offset])), flags)
    concept_assert(typeinfo is_raw(data[0]), "updateData can write only array of pod")
  else
    concept_assert(false, "updateData with offset and elems count supports only arrays of pod")
