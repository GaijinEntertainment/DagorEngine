module random shared
require math
require DagorRandom public

def rnd_point_on_disk(radius : float) : float2
  return rnd_point_on_ring(0f, radius)

def rnd_point_on_ring(radius : float2) : float2
  return rnd_point_on_ring(radius.x, radius.y)

def rnd_point_on_ring(radius_from, radius_to : float) : float2
  if radius_from > radius_to
    return float2()

  let rndX = gfrnd()
  let rndY = gfrnd()
  let rndAngle = gfrnd()

  var radius : float
  if rndX * (radius_to - radius_from) + radius_from < rndY * (radius_to + radius_from)
    radius =         rndX * (radius_to - radius_from) + radius_from
  else
    radius = (1.0 - rndX) * (radius_to - radius_from) + radius_from

  var s, c : float
  sincos(2f * PI * rndAngle, s, c)
  return float2(radius * c, radius * s)

def choose_index_by_weight(list)
  var totalWeight = .0
  for item in list
    totalWeight += item.weight
  var rnd = gfrnd() * totalWeight
  for item, idx in list, iter_range(list)
    rnd -= item.weight
    if rnd <= .0
      return idx
  return length(list) - 1

def random_horz_matrix()
  let angle = gfrnd() * 2f * PI
  let c = cos(angle)
  let s = sin(angle)
  var tm : float3x4
  tm[0] = float3(c, 0f, s)
  tm[1] = float3(0, 1, 0)
  tm[2] = float3(-s, 0f, c)
  tm[3] = float3(0)
  return tm

def shuffle(var list; up_to : int = -1)
  let maxSize = length(list)
  let size = up_to >= 0 ? min(up_to, maxSize) : maxSize
  for i in range(size)
    let idx = grnd() % (maxSize - i)
    swap(list[i], list[i + idx])

//for the case when the list itself cannot be shuffled
def get_shuffled_indexes(list; up_to : int = -1)
  var indexes <- [{for idx in iter_range(list); idx}]
  shuffle(indexes, up_to)
  return <- subarray(indexes, range(up_to))


def rnd_float(a : float2)
  return rnd_float(a.x, a.y)

def rnd_int(a : int2)
  return rnd_int(a.x, a.y)

def _rnd_float(var seed : int&; a : float2)
  return _rnd_float(seed, a.x, a.y)

def _rnd_int(var seed : int&; a : int2)
  return _rnd_int(seed, a.x, a.y)

def get_random_point_in_triangle(a, b, c : float3)
  let r1 = gfrnd()
  let r2 = gfrnd()
  if r2 + r1 <= 1.0
    return a + (b - a) * r1 + (c - a) * r2
  else
    return a + (b - a) * (1.0 - r1) + (c - a) * (1.0 - r2)

def rnd_dir2()
  var dir : float2
  sincos(gfrnd() * 2. * PI, dir.y, dir.x)
  return dir

def rnd_dir3()
  let phi = 2. * PI * gfrnd()
  let theta = acos(rnd_float(-1.0, 1.0))
  return float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta))

def rnd_sign()
  return (grnd() & 1) == 0 ? 1.0 : -1.0

[generic]
def random_in_tm(tm : float3x4)
  return tm[3] + tm[0] * rnd_float(-0.5, 0.5) + tm[1] * rnd_float(-0.5, 0.5) + tm[2] * rnd_float(-0.5, 0.5)