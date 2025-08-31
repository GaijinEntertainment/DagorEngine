module sound_field_common shared

require math
require math.base
require DagorMath
require Dacoll


def get(x, y : int; field : array<float>; num_cells : int2)
  return field[x + y * num_cells.x]


def set(f : float; x, y : int; var field : array<float>&; num_cells : int2)
  field[x + y * num_cells.x] = f


def replace(f : float; x, y : int; var field : array<float>&; num_cells : int2)
  let idx = x + y * num_cells.x
  field[idx] = max(f, field[idx])


def append(f : float; x, y : int; var field : array<float>&; num_cells : int2)
  field[x + y * num_cells.x] += f


def neighbor_blend(num_cells : int2;
                   var field : array<float>&)

  for z in range(0, num_cells.y - 1)
    for x in range(0, num_cells.x - 1)
      let c = get(x, z, field, num_cells)
      let cx = (get(x + 1, z, field, num_cells) + c) * 0.5
      let cz = (get(x, z + 1, field, num_cells) + c) * 0.5
      let cxz = (get(x + 1, z + 1, field, num_cells) + c) * 0.5
      set(max(max(max(c, cx), cz), cxz), x, z, field, num_cells)

  var z = num_cells.y
  while --z > 0
    var x = num_cells.x
    while --x > 0
      let c = get(x, z, field, num_cells)
      let cx = (get(x - 1, z, field, num_cells) + c) * 0.5
      let cz = (get(x, z - 1, field, num_cells) + c) * 0.5
      let cxz = (get(x - 1, z - 1, field, num_cells) + c) * 0.5
      set(max(max(max(c, cx), cz), cxz), x, z, field, num_cells)


/*
def distance_field(num_cells : int2
                   var field : array<float>&)

  let d2 = sqrt(2.)
  for z in range(1, num_cells.y)
    for x in range(1, num_cells.x)
      let c = get(x, z, field, num_cells)
      let cx = get(x - 1, z, field, num_cells)
      let cz = get(x, z - 1, field, num_cells)
      let cxz = get(x - 1, z - 1, field, num_cells)
      set(min(min(min(c, cx + 1.), cz + 1.), cxz + d2), x, z, field, num_cells)

  var z = num_cells.y - 1
  while --z >= 0
    var x = num_cells.x - 1
    while --x >= 0
      let c = get(x, z, field, num_cells)
      let cx = get(x + 1, z, field, num_cells)
      let cz = get(x, z + 1, field, num_cells)
      let cxz = get(x + 1, z + 1, field, num_cells)
      set(min(min(min(c, cx + 1.), cz + 1.), cxz + d2), x, z, field, num_cells)
*/


def pack(idx0, idx1, val0, val1 : uint) : uint
  return val1 << 24u | val0 << 16u | idx1 << 8u | idx0


def unpack(c : uint; var idx0 : uint&; var idx1 : uint&; var val0 : uint&; var val1 : uint&)
  idx0 = c & 0xff
  idx1 = (c >> 8u) & 0xff
  val0 = (c >> 16u) & 0xff
  val1 = (c >> 24u)


def invalid_biome_index() : uint
  return 0xffu


def empty_cell() : uint
  return pack(invalid_biome_index(), invalid_biome_index(), 0u, 0u)


def floor_on_ground(pos : float2) : float3
  let maxHeight = 5000.
  var height = maxHeight * 2.
  tracedown_normalized(float3(pos.x, maxHeight, pos.y), height, ETF_HEIGHTMAP)
  return float3(pos.x, maxHeight - height, pos.y)


def trace_ground_y(pos_xz : float2)
  let maxHeight = 5000.
  var height = maxHeight * 2.
  tracedown_normalized(float3(pos_xz.x, maxHeight, pos_xz.y), height, ETF_LMESH | ETF_HEIGHTMAP/* | ETF_FRT | ETF_RI | ETF_STRUCTURES*/)
  return maxHeight - height


def trace_water_depth(pos_xz : float2)
  var waterY = 0.
  let pos3d = float3(pos_xz.x, 0., pos_xz.y)
  if traceht_water(pos3d, waterY)
    return max(0., waterY - trace_ground_y(pos_xz))
  return 0.


def floor_on_water(pos_xz : float2) : float3
  var waterY = 0.
  let pos3d = float3(pos_xz.x, 0., pos_xz.y)
  traceht_water(pos3d, waterY)
  return float3(pos_xz.x, waterY, pos_xz.y)


def pack_water(dir : float2; sound_radius : float) : uint
  let x = uint(saturate((dir.x / sound_radius + 1.) * 0.5) * 255.)
  let y = uint(saturate((dir.y / sound_radius + 1.) * 0.5) * 255.)
  //let z = 0u
  //let w = 0u
  return x << 24u | y << 16u// | z << 8u | w


def unpack_water(c : uint; sound_radius : float)
  let x = (c >> 24u)
  let y = (c >> 16u) & 0xff
  //let z = (c >> 8u) & 0xff
  //let w = c & 0xff
  return float2((float(x) / 255. - 0.5) * 2. * sound_radius, (float(y) / 255. - 0.5) * 2. * sound_radius)


def cell_idx(x, z : int; num_cells : int2)
  return x + z * num_cells.x
