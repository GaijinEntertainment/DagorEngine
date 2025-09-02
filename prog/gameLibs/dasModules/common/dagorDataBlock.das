options indenting = 2
require DagorDataBlock
require DagorMath

[generic]
def datablock_find_param(blk : DataBlock | #; name : string; start_after : int = -1)
  return datablock_find_param(blk, name, start_after)

[generic]
def datablock_getStr(blk : DataBlock const? | #; param_name : string; default_value : string)
  return blk != null ? datablock_getStr(*blk, param_name, default_value) : default_value

[generic]
def datablock_getBool(blk : DataBlock const? | #; param_name : string; default_value : bool)
  return blk != null ? datablock_getBool(*blk, param_name, default_value) : default_value

[generic]
def datablock_getInt(blk : DataBlock const? | #; param_name : string; default_value : int)
  return blk != null ? datablock_getInt(*blk, param_name, default_value) : default_value

[generic]
def datablock_getInt64(blk : DataBlock const? | #; param_name : string; default_value : int64)
  return blk != null ? datablock_getInt64(*blk, param_name, default_value) : default_value

[generic]
def datablock_getReal(blk : DataBlock const? | #; param_name : string; default_value : float)
  return blk != null ? datablock_getReal(*blk, param_name, default_value) : default_value

[generic]
def datablock_getPoint2(blk : DataBlock const? | #; param_name : string; default_value : float2)
  return blk != null ? datablock_getPoint2(*blk, param_name, default_value) : default_value

[generic]
def datablock_getPoint3(blk : DataBlock const? | #; param_name : string; default_value : float3)
  return blk != null ? datablock_getPoint3(*blk, param_name, default_value) : default_value

[generic]
def datablock_getPoint4(blk : DataBlock const? | #; param_name : string; default_value : float4)
  return blk != null ? datablock_getPoint4(*blk, param_name, default_value) : default_value

[generic]
def datablock_getIPoint2(blk : DataBlock const? | #; param_name : string; default_value : int2)
  return blk != null ? datablock_getIPoint2(*blk, param_name, default_value) : default_value

[generic]
def datablock_getIPoint3(blk : DataBlock const? | #; param_name : string; default_value : int3)
  return blk != null ? datablock_getIPoint3(*blk, param_name, default_value) : default_value

[generic]
def datablock_getIPoint4(blk : DataBlock const? | #; param_name : string; default_value : int4)
  return blk != null ? datablock_getIPoint4(*blk, param_name, default_value) : default_value

[generic]
def datablock_getE3dcolor(blk : DataBlock const? | #; param_name : string; default_value : E3DCOLOR)
  return blk != null ? datablock_getE3dcolor(*blk, param_name, default_value) : default_value

[generic]
def datablock_get_block(var data_block : DataBlock ==const | #; id : uint; blk : block<(var b : DataBlock) : void>)
  var resultBlock = datablock_get_block(data_block, id)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false

[generic]
def datablock_get_block(data_block : DataBlock ==const | #; id : uint; blk : block<(b : DataBlock) : void>)
  let resultBlock = datablock_get_block(data_block, id)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false

[generic]
def datablock_get_block(var data_block : DataBlock ==const | #; name : string; blk : block<(var b : DataBlock) : void>)
  var resultBlock = datablock_get_block_by_name(data_block, name)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false

[generic]
def datablock_get_block(data_block : DataBlock ==const | #; name : string; blk : block<(b : DataBlock) : void>)
  let resultBlock = datablock_get_block_by_name(data_block, name)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false

[generic]
def datablock_for_each_block(var data_block : DataBlock ==const | #; blk : block<(var b : DataBlock) : void>)
  for i in 0u..data_block.blockCount
    var b = data_block |> datablock_get_block(i)
    if b != null
      invoke(blk, *b)

[generic]
def datablock_for_each_block(data_block : DataBlock ==const | #; blk : block<(b : DataBlock) : void>)
  for i in 0u..data_block.blockCount
    let b = data_block |> datablock_get_block(i)
    if b != null
      invoke(blk, *b)

[generic]
def datablock_for_each_block(var data_block : DataBlock ==const | #; name : string; blk : block<(var b : DataBlock) : void>)
  let name_id = data_block |> datablock_getNameId(name)
  if name_id != -1
    for i in 0u..data_block.blockCount
      var b = data_block |> datablock_get_block(i)
      if b != null && *b |> datablock_getBlockNameId() == name_id
        invoke(blk, *b)

[generic]
def datablock_for_each_block(data_block : DataBlock ==const | #; name : string; blk : block<(b : DataBlock) : void>)
  let name_id = data_block |> datablock_getNameId(name)
  if name_id != -1
    for i in 0u..data_block.blockCount
      let b = data_block |> datablock_get_block(i)
      if b != null && *b |> datablock_getBlockNameId() == name_id
        invoke(blk, *b)

[generic]
def datablock_for_each_param(data_block : DataBlock | #; blk : block<(i : int; t : int) : void>)
  for i in 0u..data_block.paramCount
    let t = data_block |> datablock_getParamType(i)
    invoke(blk, int(i), t)

[generic]
def datablock_for_each_param(data_block : DataBlock | #; name : string; blk : block<(i : int; t : int) : void>)
  let name_id = data_block |> datablock_getNameId(name)
  if name_id != -1
    for i in 0u..data_block.paramCount
      if data_block |> datablock_getParamNameId(i) == name_id
        let t = data_block |> datablock_getParamType(i)
        invoke(blk, int(i), t)

[generic]
def datablock_add_block(var data_block : DataBlock | #; name : string; blk : block<(var b : DataBlock) : void>)
  var resultBlock = datablock_add_block(data_block, name)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false

[generic]
def datablock_add_new_block(var data_block : DataBlock | #; name : string; blk : block<(var b : DataBlock) : void>)
  var resultBlock = datablock_add_new_block(data_block, name)
  if resultBlock != null
    invoke(blk, *resultBlock)
    return true
  return false
