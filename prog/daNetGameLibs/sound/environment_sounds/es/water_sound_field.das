require ecs
require math.base
require DagorMath
require soundSystem
require soundEvent
require environment_sounds.modules.sound_field_common
require level


def is_water(offset : float2; cell_xz : int2; cell_size : float; threshold : float)
  return (trace_water_depth(offset + float2(float(cell_xz.x) - 0.25, float(cell_xz.y) - 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) + 0.25, float(cell_xz.y) - 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) - 0.25, float(cell_xz.y) + 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) + 0.25, float(cell_xz.y) + 0.25) * cell_size) > threshold)


def water_mask(var mask : array<bool>&; num_cells : int2; cell_size, depth_threshold : float; offset : float2; var water_aa : int2&; var water_bb : int2&)
  resize(mask, num_cells.x * num_cells.y)
  water_aa = num_cells
  water_bb = int2(0)
  for z in range(0, num_cells.y)
    for x in range(0, num_cells.x)
      mask[cell_idx(x, z, num_cells)] = is_water(offset, int2(x, z), cell_size, depth_threshold)
      if mask[cell_idx(x, z, num_cells)]
        water_aa = min(water_aa, int2(x, z))
        water_bb = max(water_bb, int2(x, z))


[es(tag=sound, on_appear, on_event=EventLevelLoaded)]
def water_sound_field_init(evt : Event;
                           water_sound_field__cellSize : float;
                           water_sound_field__depthThreshold : float;
                           water_sound_field__path : string;
                           water_sound_field__worldAABB : float4;
                           var water_sound_field__soundRadius : float&;
                           var water_sound_field__offset : float2&;
                           var water_sound_field__numCells : int2&;
                           var water_sound_field__cells : UInt32List&)

  if water_sound_field__numCells.x != 0 || !is_level_loaded()
    return

  assume cellSize = water_sound_field__cellSize
  assume offset = water_sound_field__offset
  assume ncells = water_sound_field__numCells

  offset = water_sound_field__worldAABB.xy
  ncells = int2(ceil((water_sound_field__worldAABB.zw - water_sound_field__worldAABB.xy) / cellSize))

  var aa, bb : int2
  var mask : array<bool>
  water_mask(mask, ncells, cellSize, water_sound_field__depthThreshold, offset, aa, bb)

  if aa.x >= bb.x
    ncells = int2(0, 0)
    return

  water_sound_field__soundRadius = max(1., get_max_distance(water_sound_field__path))
  let extent = water_sound_field__soundRadius / cellSize

  aa = max(aa - int2(extent), int2(0))
  bb = min(bb + int2(extent + 2.), ncells)

  let maskNCells = ncells
  ncells = max(bb - aa, int2(0))
  let maxDist = 1000.
  let kernel = int(extent) + 2
  offset += float2(aa) * cellSize

  sound_debug("init water_sound_field aa={aa} bb={bb} ncells={ncells}, size={ncells.x * ncells.y * 4 / 1024}")

  resize(water_sound_field__cells, ncells.x * ncells.y)

  for z in range(0, ncells.y)
    for x in range(0, ncells.x)

      var maxDistSq = square(maxDist)
      var maxDir = float2(maxDist, maxDist)
      let mn = int2(max(0, x - kernel), max(0, z - kernel))
      let mx = int2(min(x + kernel + 1, ncells.x - 1), min(z + kernel + 1, ncells.y - 1))

      for zz in range(mn.y, mx.y)
        for xx in range(mn.x, mx.x)
          if mask[cell_idx(xx + aa.x, zz + aa.y, maskNCells)]
            let dir = float2(float(xx - x), float(zz - z))
            let distSq = length_sq(dir)
            if maxDistSq > distSq
              maxDistSq = distSq
              maxDir = dir

      water_sound_field__cells[cell_idx(x, z, ncells)] = pack_water(maxDir * cellSize, water_sound_field__soundRadius)



[es(no_order, tag=sound, on_event=ParallelUpdateFrameDelayed)]
def water_sound_field_update(evt : Event;
                             water_sound_field__soundRadius : float;
                             water_sound_field__cellSize : float;
                             water_sound_field__offset : float2;
                             water_sound_field__numCells : int2;
                             water_sound_field__cells : UInt32List;
                             water_sound_field__path : string;
                             var water_sound_field__event : SoundEvent&;
                             var water_sound_field__eventPos : float3&)

  assume cellSize = water_sound_field__cellSize
  assume offset = water_sound_field__offset
  assume ncells = water_sound_field__numCells
  assume cells = water_sound_field__cells
  assume soundRadius = water_sound_field__soundRadius

  if ncells.x == 0
    return

  var nw = float2(soundRadius, soundRadius)
  let cam2d = get_listener_pos().xz
  let camInCell = (cam2d - offset) / cellSize
  let cellIdx = int2(floor(camInCell))
  if cellIdx.x >= 0 && cellIdx.y >= 0 && cellIdx.x + 1 < ncells.x && cellIdx.y + 1 < ncells.y

    let na = unpack_water(cells[cell_idx(cellIdx.x, cellIdx.y, ncells)], soundRadius)
    if length_sq(na) < square(soundRadius)
      let nb = unpack_water(cells[cell_idx(cellIdx.x + 1, cellIdx.y, ncells)], soundRadius)
      let nc = unpack_water(cells[cell_idx(cellIdx.x, cellIdx.y + 1, ncells)], soundRadius)
      let nd = unpack_water(cells[cell_idx(cellIdx.x + 1, cellIdx.y + 1, ncells)], soundRadius)

      let t = camInCell - float2(cellIdx)
      nw = lerp(lerp(na, nb, t.x), lerp(nc, nd, t.x), t.y)

  let shouldPlay = length_sq(nw) < square(soundRadius)
  water_sound_field__eventPos = shouldPlay ? floor_on_water(cam2d + nw) : get_listener_pos()
  play_or_abandon(water_sound_field__event, water_sound_field__path, water_sound_field__eventPos, shouldPlay)
