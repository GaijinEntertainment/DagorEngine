options no_aot
require ecs
require ecs.common
require player
require DagorConsole
require DagorSystem
require math.base
require DngCamera
require DagorFiles
require DagorDataBlock


[console_cmd(name="camera.save")]
def console_camera_save(slot = "slot0")
  var cameraTm = IDENT_TM
  let cameraFound = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      cameraTm = transform
      return true
    return false
  if !cameraFound
    console_print("No active cameras found.")
    return

  let filename = "cameraPositions.blk"
  using() <| $(var cameraPositions : DataBlock)
    var fileStat : DagorStat
    if df_stat(filename, fileStat) >= 0
      datablock_load(cameraPositions, filename)
    cameraPositions |> datablock_remove_block(slot)
    datablock_add_block(cameraPositions, slot) <| $(cameraSlot)
      cameraSlot |> add("tm", cameraTm)
    datablock_save_to_text_file(cameraPositions, filename)


[console_cmd(name="camera.restore")]
def console_camera_restore(slot = "slot0")
  var cameraTm = IDENT_TM
  let filename = "cameraPositions.blk"
  using() <| $(var cameraPositions : DataBlock)
    var fileStat : DagorStat
    if df_stat(filename, fileStat) >= 0
      datablock_load(cameraPositions, filename)
      datablock_get_block(cameraPositions, slot) <| $(cameraSlot)
        if datablock_param_exists(cameraSlot, "tm", -1)
          cameraTm = datablock_getTm(cameraSlot, "tm", IDENT_TM)
          set_cam_itm(cameraTm)


[console_cmd(name="camera.fov")]
def console_camera_fov(new_fov = -1.0)
  let cameraFound = find_query() <| $ [es] (camera__active : bool; var fov aka camera_fov : float&)
    if camera__active
      if new_fov > 0.0
        camera_fov = new_fov
        console_print("Camera FOV changed to {camera_fov}.")
      else
        console_print("Camera FOV is {camera_fov}.")
      return true
    return false

  if !cameraFound
    console_print("No active cameras with FOV found.")


[console_cmd(name="camera.dir")]
def console_camera_dir(dir_1 = 2.0; dir_2 = 2.0; dir_3 = 2.0)
  var cameraTm = IDENT_TM
  let cameraFound = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      cameraTm = transform
      return true
    return false
  if !cameraFound
    console_print("No active cameras found.")
    return

  var cameraDir = cameraTm[2]
  if dir_1 > 1.0 && dir_2 > 1.0 && dir_3 > 1.0
    console_print("Camera dir {cameraDir.x} {cameraDir.y} {cameraDir.z}")
    set_clipboard_ansi_text("{cameraDir.x}, {cameraDir.y}, {cameraDir.z}")
    return
  elif dir_2 > 1.0 && dir_3 > 1.0
    cameraDir.y = dir_1
  elif dir_3 > 1.0
    cameraDir.x = dir_1
    cameraDir.z = dir_2
  else
    cameraDir.x = dir_1
    cameraDir.y = dir_2
    cameraDir.z = dir_3

  cameraTm[2] = cameraDir
  cameraTm[0] = cross(float3(0.0, 1.0, 0.0), cameraTm[2])
  cameraTm[1] = cross(cameraTm[2], cameraTm[0])
  cameraTm |> orthonormalize()
  set_cam_itm(cameraTm)


[console_cmd(name="camera.tm")]
def console_camera_tm()
  let cameraFound = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      console_print("Camera tm {transform}")
      set_clipboard_ansi_text("{transform}")
      return true
    return false
  if !cameraFound
    console_print("No active cameras found.")
    return
