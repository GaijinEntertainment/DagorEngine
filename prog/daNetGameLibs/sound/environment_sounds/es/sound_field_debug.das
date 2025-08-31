options no_aot
require ecs
require ecs.ecs_template
require math.color
require math.base
require DagorMath
require DagorDebug3D
require DagorSystem
require DagorConsole
require soundSystem
require environment_sounds.modules.sound_field_common


let MAX_RANGE = 500.

let BIOME_COLORS = fixed_array(0x00ffff00, 0x000000ff, 0x00ff0040, 0x0040ffff)


def biome_color(idx : uint)
  return BIOME_COLORS[int(idx) % length(BIOME_COLORS)]


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=soundDebugDrawField)]
def sound_field_draw_debug(evt : Event)

  query() <| $ [es] (sound_field__cells : UInt32List;
                     sound_field__numCells : int2;
                     sound_field__cellSize : float;
                     sound_field__tmsDebug : TMatrixList;
                     sound_field__offset : float2)

    assume cells = sound_field__cells
    assume numCells = sound_field__numCells
    assume cellSize = sound_field__cellSize
    assume offset = sound_field__offset

    begin_draw_cached_debug_lines(true, true, false)
    set_cached_debug_lines_wtm(IDENT_TM)

    for tm in sound_field__tmsDebug
      draw_cached_debug_sphere(tm[3], tm[0].x, E3DCOLOR(0xffffffff), 32)

    end_draw_cached_debug_lines()

    begin_draw_cached_debug_lines(false, false, false)

    let aa = float3(offset.x, 0., offset.y)
    let bb = float3(offset.x + cellSize * float(numCells.x), 0., offset.y + cellSize * float(numCells.y))
    draw_cached_debug_box(BBox3(aa, bb), E3DCOLOR(0xffffffff))

    let invalidBiomeIndex = invalid_biome_index()
    let listener = get_listener_pos()
    let cam = int2(int(floor(safediv((listener.x - offset.x), cellSize))), int(floor(safediv((listener.z - offset.y), cellSize))))
    let viewRange = int(ceil(MAX_RANGE / cellSize))
    let leftBack = max(int2(0, 0), int2(cam.x - viewRange, cam.y - viewRange))
    let rightFront = min(int2(cam.x + viewRange, cam.y + viewRange), numCells)

    for z in range(leftBack.y, rightFront.y)
      for x in range(leftBack.x, rightFront.x)

        var idx0, idx1, val0, val1 : uint
        unpack(cells[x + z * numCells.x], idx0, idx1, val0, val1)

        if idx0 != invalidBiomeIndex || idx1 != invalidBiomeIndex

          // c d
          // a b

          let a = floor_on_ground(float2(offset.x + float(x) * cellSize, offset.y + float(z) * cellSize))
          let b = floor_on_ground(float2(a.x + cellSize, a.z))
          let d = floor_on_ground(float2(a.x + cellSize, a.z + cellSize))
          let c = floor_on_ground(float2(a.x, a.z + cellSize))

          if idx0 != invalidBiomeIndex
            let color = E3DCOLOR(biome_color(idx0) | (val0 << 24u))
            draw_cached_debug_line(a, b, color)
            draw_cached_debug_line(b, d, color)
            draw_cached_debug_line(d, c, color)
            draw_cached_debug_line(c, a, color)

          if idx1 != invalidBiomeIndex
            let color = E3DCOLOR(biome_color(idx1) | (val1 << 24u))
            draw_cached_debug_line(a, b, color)
            draw_cached_debug_line(b, d, color)
            draw_cached_debug_line(d, c, color)
            draw_cached_debug_line(c, a, color)

    end_draw_cached_debug_lines()


[es(tag=sound, on_appear)]
def sound_field_appear_debug(evt : Event;
                             sound_field__numCells : int2)
  let sizeKb = 4 * sound_field__numCells.x * sound_field__numCells.y / 1024
  sound_debug("sound_field is {sound_field__numCells.x}x{sound_field__numCells.y}, {sizeKb}kb")


[ecs_template]
struct sound_debug_draw_field
  soundDebugDrawField : bool


[console_cmd(name="snd.debug_draw_field")]
def sound_debug_draw_field_console_cmd()
  var entEid : EntityId
  find_query() <| $ [es(REQUIRE=soundDebugDrawField)] (eid : EntityId)
    entEid = eid
    return true
  if !entEid
    createEntity("sound_debug_draw_field")
    sound_debug("snd.debug_draw_field=1")
  else
    destroyEntity(entEid)
    sound_debug("snd.debug_draw_field=0")
