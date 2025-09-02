require ecs
require ecs.safe
require math.base
require DagorMath
require soundEvent
require soundSystem
require danetlibs.renderer.includes.pufd_events
require environment_sounds.modules.sound_field_common
require DagorSystem
require gameObject
require strings


struct Biome
  transforms : array<float3x4>
  alpha2Value : float2
  extent : float


def max_radius(tm : float3x4; extent : float) : float2
  let x = max(tm[0].x, tm[0].z) + extent
  return float2(x, x)


def init_field(var field : array<float>&; val : float)
  for f in field
    f = val

/*
def make_field(biome : Biome&
               cell_size : float
               num_cells : int2
               offset : float2
               var field : array<float>&)

  let max_dist = 255.
  init_field(field, max_dist)

  for tm in biome.transforms
    let p = tm[3].xz - offset
    let r = max_radius(tm, biome.extent)
    let a = max(int2(0, 0), int2(floor((p - r) / cell_size)))
    let b = min(int2(floor((p + r) / cell_size)), num_cells - int2(1))

    let ca = int2((a.x + b.x) / 2, (a.y + b.y) / 2)
    let cr2 = int(ceil(square(r.x / cell_size)))

    for z in range(a.y, b.y)
      for x in range(a.x, b.x)
        if square(x - ca.x) + square(z - ca.y) < cr2
          set(0., x, z, field, num_cells)

  distance_field(num_cells, field)

  for f in field
    f = 1. - saturate(safediv(f, biome.radius))
*/


def ease_in_out(t : float)
  return t * t * (3. - 2. * t)


def make_field(biome : Biome&;
               cell_size : float;
               num_cells : int2;
               offset : float2;
               var field : array<float>&)

  init_field(field, 0.)

  for tm in biome.transforms
    let p = tm[3].xz - offset
    let r = max_radius(tm, biome.extent)
    let a = max(int2(0, 0), int2(floor((p - r) / cell_size)) - int2(8))
    let b = min(int2(floor((p + r) / cell_size)) + int2(8), num_cells)

    for z in range(a.y, b.y)
      for x in range(a.x, b.x)
        let alpha = safediv(distance(float2((float(x) + 0.5) * cell_size, (float(z) + 0.5) * cell_size), p), r.x)
        let val = ease_in_out(cvt(alpha, biome.alpha2Value.x, biome.alpha2Value.y, 1., 0.))
        replace(val, x, z, field, num_cells)


def make_bounds(biomes : array<Biome>; var aa : float2&; var bb : float2&) : bool
  aa = float2(FLT_MAX, FLT_MAX)
  bb = float2(-FLT_MAX, -FLT_MAX)
  for biome in biomes
    for tm in biome.transforms
      let r = max_radius(tm, biome.extent)
      aa = min(aa, tm[3].xz - r)
      bb = max(bb, tm[3].xz + r)
  return bb.x >= aa.x


def resize_field(var arr &; num_cells : int2)
  resize(arr, num_cells.x * num_cells.y)


def init_cells(var cells &)
  let defValuePacked = empty_cell()
  for c in cells
    c = defValuePacked


def write_field(field : array<float>;
                biome_idx : uint;
                var cells &)

  for f, c in field, cells

    if f > 0.
      let val = clamp(uint(f * 255.), 0u, 0xff)

      var idx0, idx1, val0, val1 : uint
      unpack(c, idx0, idx1, val0, val1)

      let idx = biome_idx
      if idx0 == idx
        val0 = max(val0, val)
      elif idx1 == idx
        val1 = max(val1, val)
      elif idx0 == invalid_biome_index()
        idx0 = idx
        val0 = val
      elif idx1 == invalid_biome_index()
        idx1 = idx
        val1 = val
      elif val > val0 && val0 < val1
        idx0 = idx
        val0 = val
      elif val > val1 && val1 < val0
        idx1 = idx
        val1 = val

      c = pack(idx0, idx1, val0, val1)


[es(tag=sound, on_appear, on_event=EventGameObjectsCreated)]
def sound_field_on_appear(evt : Event;
                          sound_field__biomeTypes : Array;
                          sound_field__cellSize : float;
                          var sound_field__cells : UInt32List&;
                          var sound_field__tmsDebug : TMatrixList&;
                          var sound_field__numCells : int2&;
                          var sound_field__offset : float2&)

  if sound_field__numCells.x != 0
    return

  var biomes : array<Biome>
  resize(biomes, length(sound_field__biomeTypes))
  var numObjects = 0

  find_query() <| $ [es] (game_objects : GameObjects)

    for it, biome in sound_field__biomeTypes, biomes
      let gameObjectType = it as Object
      let gameObject = gameObjectType?.gameObject ?? ""
      biome.extent = gameObjectType?.extent ?? 0.
      biome.alpha2Value = gameObjectType?.alpha2Value ?? float2(0., 1.)

      let scene = game_objects |> get_scene_game_objects_by_name(gameObject)
      if scene != null
        scene |> find_scene_game_objects() <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
          let t : float3x4 := tm
          biome.transforms |> push(t)
          ++numObjects
          return false
      else
        sound_debug("gameobject '{gameObject}' is missing")

    return true

  assume cells = sound_field__cells
  assume cellSize = sound_field__cellSize
  assume numCells = sound_field__numCells
  assume offset = sound_field__offset

  clear(cells)
  clear(sound_field__tmsDebug)
  numCells = int2(0, 0)
  offset = float2(0, 0)

  if numObjects <= 0
    return

  var aa, bb : float2
  if !make_bounds(biomes, aa, bb)
    return

  numCells = int2(ceil((bb - aa) / cellSize)) + int2(1, 1)
  offset = floor(aa / cellSize) * cellSize

  resize_field(cells, numCells)

  var field : array<float>
  resize_field(field, numCells)

  init_cells(cells)

  for biome, biomeIdx in biomes, range(length(biomes))
    make_field(biome, cellSize, numCells, offset, field)
    write_field(field, uint(biomeIdx), cells)

  sound_debug("init sound_field ncells={numCells}, objects={numObjects}")

  for biome in biomes
    for tm in biome.transforms
      sound_field__tmsDebug |> push(tm)



def sample_cell(wpos : float2;
                cells : UInt32List;
                offset : float2;
                num_cells : int2;
                cell_size : float)

  assert(num_cells.x * num_cells.y == length(cells))

  if !empty(cells)

    let cam = float2(safediv((wpos.x - offset.x), cell_size), safediv((wpos.y - offset.y), cell_size))

    let cx = int(floor(cam.x))
    let cz = int(floor(cam.y))

    if cx >= 0 && cx < num_cells.x && cz >= 0 && cz < num_cells.y
      return cells[cx + cz * num_cells.x]

  return empty_cell()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_field_update(info : ParallelUpdateFrameDelayed;
                       sound_field__vars : StringList;
                       sound_field__cells : UInt32List;
                       sound_field__offset : float2;
                       sound_field__numCells : int2;
                       sound_field__cellSize : float)

  if !sound_banks_is_master_preset_loaded()
    return

  let cell = sample_cell(get_listener_pos().xz, sound_field__cells, sound_field__offset, sound_field__numCells, sound_field__cellSize)

  var idx0, idx1, val0, val1 : uint
  unpack(cell, idx0, idx1, val0, val1)

  for name, idx in sound_field__vars, range(length(sound_field__vars))
    let v = uint(idx) == idx0 ? float(val0) / 255. : uint(idx) == idx1 ? float(val1) / 255. : 0.
    set_var_global(string(name), v)


[es(tag=sound, on_disappear)]
def sound_field_disappear(evt : Event;
                          sound_field__vars : StringList)
  if sound_banks_is_master_preset_loaded()
    for name in sound_field__vars
      set_var_global(string(name), 0.)
