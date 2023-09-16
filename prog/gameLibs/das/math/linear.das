module linear shared
require math
require DagorSystem
require DagorMath


[generic] // closest_pt_on_seg from dag_mathUtils.h
def closest_pt_on_seg(point, a, b; var t : float&)
  let dir = normalize(b - a)
  t = dot((point - a), dir)
  if t < 0.
    return a
  if t * t > distance_sq(b, a)
    return b

  return a + dir * t


[generic]
def closest_pt_on_seg(point, a, b)
  var t : float
  return closest_pt_on_seg(point, a, b, t)

[generic]
def distance_sq_to_seg(point, a, b) : float
  var t : float
  let pt = closest_pt_on_seg(point, a, b, t)
  return distance_sq(point, pt)

[generic]
def distance_to_line(point, a, dir) : float
  let t = dot(point - a, dir)
  let pt = a + dir * float3(t)
  return distance(point, pt)

[generic]
def distance_sq_to_line(point, a, dir) : float
  let t = dot(point - a, dir)
  let pt = a + dir * float3(t)
  return distance_sq(point, pt)

[generic]
def point_to_plane_dst(p, norm, point_in_plane : float3) : float
  return dot(point_in_plane, norm) - dot(p, norm)

[generic]
def point_to_tm_dst(p : float3; tm : float3x4) : float
  let axd = length(tm[0])
  let ayd = length(tm[1])
  let azd = length(tm[2])

  let v = tm[3] - p

  let dx = max(.0, dot(v, tm[0]) * safeinv(axd) - 0.5 * axd)
  let dy = max(.0, dot(v, tm[1]) * safeinv(ayd) - 0.5 * ayd)
  let dz = max(.0, dot(v, tm[2]) * safeinv(azd) - 0.5 * azd)

  return length(float3(dx, dy, dz))

def is_2d_lines_crossed(startL1, endL1, startL2, endL2 : float2) : bool
  let den = (endL2.y - startL2.y) * (endL1.x - startL1.x) - (endL2.x - startL2.x) * (endL1.y - startL1.y)
  if den == 0.0
    return false
  let ua = ((endL2.x - startL2.x) * (startL1.y - startL2.y) - (endL2.y - startL2.y) * (startL1.x - startL2.x)) / den
  let ub = ((endL1.x - startL1.x) * (startL1.y - startL2.y) - (endL1.y - startL1.y) * (startL1.x - startL2.x)) / den
  return ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0

//https ://en.wikipedia.org/wiki/Quickhull //but this version is asymptotically slower, todo...
def convex_hull(source : array<float2>) : array<float2>
  if length(source) <= 3
    return <- clone_to_move(source)

  var maxX = source[0].x
  var maxXIdx = 0
  var minX = source[0].x
  var minXIdx = 0
  for p, idx in source, iter_range(source)
    if maxX < p.x
      maxX = p.x
      maxXIdx = idx
    if minX > p.x
      minX = p.x
      minXIdx = idx

  var list : array<float2>
  push(list, source[maxXIdx])
  if minXIdx == maxXIdx
    return <- list
  push(list, source[minXIdx])
  var outside := source
  if minXIdx < maxXIdx
    maxXIdx--
  erase(outside, minXIdx)
  erase(outside, maxXIdx)

  var lastSuccess = 0
  var stepLimit = length(outside) * 3 //JIC
  //for numerical stability we should work with all edges in the same direction.
  //the base edge (which originally is the very first edge) is special since it participates in both branches
  var swapLastEdge = true
  while length(outside) > 0 && stepLimit-- > 0
    var i = 0
    while i < length(list) && length(outside) > 0
      let swap = (i != 0) && (i == length(list) - 1) && swapLastEdge //swap when the base edge is checked the second time in the opposite direction
      if swap
        swapLastEdge = false
      if !extend_hull(list, outside, i, swap)
        if i == (lastSuccess - 1 + length(list)) % length(list) //we've gone full circle and no progress has been made
          //I think this should never happen.
          //But I leave it here JIC, since freezes are nasty and hard to diagnose.
          logerr("Convex hull algo is wrong. Aborting prematurely to avoid endless cycle.")
          for p in source
            print("source: ({p.x}, {p.y})")
          for p in outside
            print("outside: ({p.x}, {p.y})")
          for p in list
            print("list: ({p.x}, {p.y})")
          return <- list
        i++
      else
        lastSuccess = i
  return <- list


def private signed_point_to_line_distance(p, from, to : float2) : float
  if from.x == to.x
    if from.y == to.y
      return .0
    return from.y < to.y ? from.x - p.x : p.x - from.x
  let a = from.y - to.y
  let b = to.x - from.x
  let c = from.x * to.y - to.x * from.y
  return (a * p.x + b * p.y + c) / sqrt(a * a + b * b)

def private point_to_line_relation(p, from, to : float2)
  let a = from.y - to.y
  let b = to.x - from.x
  let c = from.x * to.y - to.x * from.y
  return a * p.x + b * p.y + c

def private test_point_in_2d_triangle(p : float2; polygon : float2[3]; swap_last_edge : bool)
  return (point_to_line_relation(p, polygon[0], polygon[1]) <= .0 &&
          point_to_line_relation(p, polygon[1], polygon[2]) <= .0 &&
            (
              (swap_last_edge && point_to_line_relation(p, polygon[0], polygon[2]) > .0) ||
              (!swap_last_edge && point_to_line_relation(p, polygon[2], polygon[0]) <= .0)
            )
          )


def private extend_hull(var hull : array<float2>; var outside : array<float2>; from_param : int; swap_direction : bool) : bool
  var from = from_param
  var to = (from + 1) % length(hull)
  if swap_direction
    swap(from, to)
  var maxP = 0
  var maxDst = -1.0
  for idx in iter_range(outside)
    let dst = signed_point_to_line_distance(outside[idx], hull[from], hull[to]) * (swap_direction ? -1.0 : 1.0)
    if dst > maxDst
      maxDst = dst
      maxP = idx
  if swap_direction
    swap(from, to)
  if maxDst >= 0.0
    var newOutside : array<float2>
    let polygon = [[float2[3] hull[from]; outside[maxP]; hull[to]]]
    for p, oidx in outside, iter_range(outside)
      if oidx != maxP && !test_point_in_2d_triangle(p, polygon, !swap_direction)
        push(newOutside, p)
    push(hull, outside[maxP], to)
    outside := newOutside
  return maxDst > 0.0