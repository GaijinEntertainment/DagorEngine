options no_aot
require ecs
require ecs.ecs_template
require math.color
require math.base
require DagorMath
require DagorDebug3D
require DagorConsole
require soundSystem
require environment_sounds.modules.sound_field_common


def draw_cell(x, z : int; offset : float2; cell_size : float; color : E3DCOLOR; draw_full : bool)

  // c d
  // a b

  let a = floor_on_water(float2(offset.x + float(x) * cell_size, offset.y + float(z) * cell_size))
  let b = floor_on_water(float2(a.x + cell_size, a.z))
  let c = floor_on_water(float2(a.x, a.z + cell_size))
  let d = floor_on_water(float2(a.x + cell_size, a.z + cell_size))

  draw_cached_debug_line(a, b, color)
  if draw_full
    draw_cached_debug_line(b, d, color)
    draw_cached_debug_line(d, c, color)
  draw_cached_debug_line(c, a, color)


def make_color_temp(t : float) : E3DCOLOR

  let clr0 = float3(0., 0., 1.)
  let clr1 = float3(0., 1., 0.)
  let clr2 = float3(1., 1., 1.)

  let clr = saturate(t < 0.5 ? lerp(clr0, clr1, t * 2.) : lerp(clr1, clr2, (t - 0.5) * 2.))

  let r = uint(clr.x * 255.) << 16u
  let g = uint(clr.y * 255.) << 8u
  let b = uint(clr.z * 255.)
  let a = 255u << 24u

  return E3DCOLOR(r | g | b | a)


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawWaterField)]
def water_sound_field_draw_debug(evt : Event)

  query() <| $ [es] (water_sound_field__eventPos : float3;
                     water_sound_field__cellSize : float;
                     water_sound_field__offset : float2;
                     water_sound_field__numCells : int2;
                     water_sound_field__cells : UInt8List)

    assume cellSize = water_sound_field__cellSize
    assume offset = water_sound_field__offset
    assume ncells = water_sound_field__numCells
    assume cells = water_sound_field__cells

    let cam2d = get_listener_pos().xz
    let camInCell = (cam2d - offset) / cellSize
    let cellIdx = int2(floor(camInCell))

    let drawRange = 25
    let mn = int2(max(0, cellIdx.x - drawRange), max(0, cellIdx.y - drawRange))
    let mx = int2(min(cellIdx.x + drawRange, ncells.x), min(cellIdx.y + drawRange, ncells.y))

    begin_draw_cached_debug_lines(false, false, false)

    for z in range(mn.y, mx.y)
      for x in range(mn.x, mx.x)
        let cell = float(cells[cell_idx(x, z, ncells)]) / 255.
        if cell > 0.
          draw_cell(x, z, offset, cellSize, make_color_temp(cell), false)

    if cellIdx.x >= 0 && cellIdx.x < ncells.x && cellIdx.y >= 0 && cellIdx.y < ncells.y
      draw_cell(cellIdx.x, cellIdx.y, offset, cellSize, E3DCOLOR(0xffffffff), true)

    let color = E3DCOLOR(0xffffff00)
    let s = float3(cam2d.x, 0., cam2d.y)
    let e = float3(water_sound_field__eventPos.x, 0., water_sound_field__eventPos.z)
    draw_cached_debug_line(s, e, color)

    // bounds
    let a = floor_on_water(float2(offset.x, offset.y))
    let b = floor_on_water(float2(a.x + cellSize * float(ncells.x), a.z))
    let c = floor_on_water(float2(a.x, a.z + cellSize * float(ncells.y)))
    let d = floor_on_water(float2(a.x + cellSize * float(ncells.x), a.z + cellSize * float(ncells.y)))

    draw_cached_debug_line(a, b, color)
    draw_cached_debug_line(b, d, color)
    draw_cached_debug_line(d, c, color)
    draw_cached_debug_line(c, a, color)

    end_draw_cached_debug_lines()


[ecs_template]
struct sound_debug_draw_water_field
  soundDebugDrawWaterField : bool


[console_cmd(name="snd.debug_draw_water_field")]
def sound_debug_draw_water_field_console_cmd()
  var entEid : EntityId
  find_query() <| $ [es(REQUIRE=soundDebugDrawWaterField)] (eid : EntityId)
    entEid = eid
    return true
  if !entEid
    createEntity("sound_debug_draw_water_field")
    sound_debug("snd.debug_draw_water_field=1")
  else
    destroyEntity(entEid)
    sound_debug("snd.debug_draw_water_field=0")
