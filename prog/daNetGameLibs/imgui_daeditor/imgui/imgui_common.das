module imgui_common shared

require DagorDataBlock
require DagorImgui


def imgui_has_blk_val(block_name, field_name : string) : bool
  var res = false
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_find_param(field_name) >= 0
  return res


def imgui_get_blk_val(block_name, field_name : string; def_val : int) : int
  var res = def_val
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_getInt(field_name, res)
  return res


def imgui_get_blk_val(block_name, field_name : string; def_val : string) : string
  var res = def_val
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_getStr(field_name, res)
  return res


def imgui_get_blk_val(block_name, field_name : string; def_val : float) : float
  var res = def_val
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_getReal(field_name, res)
  return res


def imgui_get_blk_val(block_name, field_name : string; def_val : bool) : bool
  var res = def_val
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_getBool(field_name, res)
  return res


def imgui_set_blk_val(block_name, field_name : string; val)
  imgui_get_blk() |> datablock_add_block(block_name) <| $(blk)
    blk |> set(field_name, val)


def imgui_del_blk_val(block_name, field_name : string) : bool
  var res = false
  imgui_get_blk() |> datablock_get_block(block_name) <| $(blk)
    res = blk |> datablock_remove_param(field_name)
  return res
