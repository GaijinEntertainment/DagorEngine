options no_aot
require ecs
require DagorConsole
require DagorShaders
require ProjectiveDecals
require danetlibs.blood_decals.render.blood_decals_common

[console_cmd(name="blood_decals.debug", hint="<mode 0 - off, 1 - draw boxes, 2 - draw depth>")]
def blood_decals_debug(mode : int)
  if (0 <= mode && mode < 3)
    DagorShaders::set_int(get_shader_variable_id("blood_decals_debug_mode", true), mode)

[console_cmd(name="blood_decals.count")]
def blood_decals_count()
  query() <| $ [es] (blood_decals_decals_count : int)
    console_print("decals count: {blood_decals_decals_count}")

[console_cmd(name="blood_decals.clear")]
def blood_decals_clear()
  query() <| $ [es] (var blood_decals_manager : RingBufferDecals; var blood_decals_decals_count : int&)
    blood_decals_manager |> clear()
    blood_decals_decals_count = 0

[console_cmd(name="blood_decals.add")]
def blood_decals_add(distance : float)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if !camera__active
      return false
    let eye = transform[3]
    let forward = transform[2]
    add_blood_decal(distance, eye, forward);
    return true

[console_cmd(name="blood_decals.add_around")]
def blood_decals_add_around(distance : float; resX, resY : int)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if !camera__active
      return false
    let eye = transform[3]

    for i in range(resX)
      for j in range(resY)

        var x = (float(i) + 0.5f) / float(resX * 2 - 1)
        var y = (float(j) + 0.5f) / float(resY * 2 - 1)
        // hemisphere only
        let xx = x
        x = (x + y) / 2f
        y = (y - xx) / 2f
        // ---
        x = clamp(x, -1f, 1f)
        y = clamp(y, -1f, 1f)

        let z = 1f - (abs(x) + abs(y))
        if (abs(x) + abs(y) > 1f)

          let absX = abs(x)
          x = -(abs(y) - 1f) * sign(x)
          y = -(absX - 1f) * sign(y)

        add_blood_decal(distance, eye, rotate(transform, normalize(float3(x, y, z))))

    return true