require ecs
require math.base
require DagorMath
require soundSystem
require soundEvent
require environment_sounds.modules.sound_field_common
require sound_utils.modules.sound_player_common
require level


def is_water(offset : float2; cell_xz : int2; cell_size : float; threshold : float)
  return (trace_water_depth(offset + float2(float(cell_xz.x) - 0.25, float(cell_xz.y) - 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) + 0.25, float(cell_xz.y) - 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) - 0.25, float(cell_xz.y) + 0.25) * cell_size) > threshold &&
          trace_water_depth(offset + float2(float(cell_xz.x) + 0.25, float(cell_xz.y) + 0.25) * cell_size) > threshold)


def make_water(var water : array<float>&; var num_cells : int2&; cell_size, depth_threshold : float; var offset : float2&; extent : int)
  resize(water, num_cells.x * num_cells.y)
  var aa = num_cells
  var bb = int2(0)
  for z in range(0, num_cells.y)
    for x in range(0, num_cells.x)
      water[cell_idx(x, z, num_cells)] = is_water(offset, int2(x, z), cell_size, depth_threshold) ? 1. : 0.
      if water[cell_idx(x, z, num_cells)] != 0.
        aa = min(aa, int2(x, z))
        bb = max(bb, int2(x + 1, z + 1))

  if aa.x >= bb.x
    num_cells = int2(0, 0)
    return

  aa = max(int2(aa.x - extent, aa.y - extent), int2(0))
  bb = min(int2(bb.x + extent, bb.y + extent), num_cells)

  offset += float2(aa) * cell_size
  let prevNumCells = num_cells
  num_cells = max(bb - aa, int2(0))
  for z in range(0, num_cells.y)
    for x in range(0, num_cells.x)
      water[cell_idx(x, z, num_cells)] = water[cell_idx(x + aa.x, z + aa.y, prevNumCells)]


[es(tag=sound, on_appear, on_event=EventLevelLoaded)]
def water_sound_field_init(evt : Event;
                           water_sound_field__cellSize : float;
                           water_sound_field__depthThreshold : float;
                           water_sound_field__worldAABB : float4;
                           var water_sound_field__offset : float2&;
                           var water_sound_field__numCells : int2&;
                           var water_sound_field__cells : UInt8List&)

  if water_sound_field__numCells.x != 0 || !is_level_loaded()
    return

  assume cellSize = water_sound_field__cellSize
  assume offset = water_sound_field__offset
  assume ncells = water_sound_field__numCells

  offset = water_sound_field__worldAABB.xy
  ncells = int2(ceil((water_sound_field__worldAABB.zw - water_sound_field__worldAABB.xy) / cellSize))

  var water : array<float>
  make_water(water, ncells, cellSize, water_sound_field__depthThreshold, offset, 2)

  resize(water_sound_field__cells, ncells.x * ncells.y)

  neighbor_blend(ncells, water)
  neighbor_blend(ncells, water)

  for z in range(0, ncells.y)
    for x in range(0, ncells.x)
      water_sound_field__cells[cell_idx(x, z, ncells)] = uint8(saturate(water[cell_idx(x, z, ncells)]) * 255.)

  sound_debug("init water_sound_field ncells={ncells}, size(kb)={ncells.x * ncells.y / 1024}")


[es(no_order, tag=sound, on_event=ParallelUpdateFrameDelayed)]
def water_sound_field_update(evt : Event;
                             water_sound_field__cellSize : float;
                             water_sound_field__offset : float2;
                             water_sound_field__numCells : int2;
                             water_sound_field__cells : UInt8List;
                             water_sound_field__path : string;
                             var water_sound_field__eventRadius : float&;
                             var water_sound_field__event : SoundEvent&;
                             var water_sound_field__eventPos : float3&)

  assume cellSize = water_sound_field__cellSize
  assume offset = water_sound_field__offset
  assume ncells = water_sound_field__numCells
  assume cells = water_sound_field__cells

  if ncells.x == 0
    return

  var vol = 0.
  let listener = get_listener_pos()
  let cam2d = listener.xz
  let camInCell = (cam2d - offset) / cellSize
  let cellIdx = int2(floor(camInCell))
  if cellIdx.x >= 0 && cellIdx.y >= 0 && cellIdx.x + 1 < ncells.x && cellIdx.y + 1 < ncells.y

    let na = float(cells[cell_idx(cellIdx.x, cellIdx.y, ncells)]) / 255.
    let nb = float(cells[cell_idx(cellIdx.x + 1, cellIdx.y, ncells)]) / 255.
    let nc = float(cells[cell_idx(cellIdx.x, cellIdx.y + 1, ncells)]) / 255.
    let nd = float(cells[cell_idx(cellIdx.x + 1, cellIdx.y + 1, ncells)]) / 255.
    let t = camInCell - float2(cellIdx)
    vol = lerp(lerp(na, nb, t.x), lerp(nc, nd, t.x), t.y)

  var shouldPlay = vol > (water_sound_field__event.enabled ? 0. : 0.05)
  water_sound_field__eventPos = floor_on_water(cam2d)
  if sound_banks_is_master_preset_loaded()
    if shouldPlay
      if water_sound_field__eventRadius <= 0.
        water_sound_field__eventRadius = max(1., get_max_distance(water_sound_field__path))
      shouldPlay = distance_sq(listener, water_sound_field__eventPos) < square(water_sound_field__event.enabled ? water_sound_field__eventRadius : water_sound_field__eventRadius * 0.9)
    if play_or_abandon(water_sound_field__event, water_sound_field__path, water_sound_field__eventPos, shouldPlay)
      set_volume(water_sound_field__event, vol)
  else
    release_play_or(water_sound_field__event)
