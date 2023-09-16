module interpolation shared
require ecs
require DagorMath

def interpolate_by_point_list(list : Point2List; value : float)
  if length(list) == 0
    return 0.0

  var min = list[0]
  var max = list[length(list) - 1]

  if value <= min.x
    return min.y
  if value >= max.x
    return max.y

  var first = 0
  var last = length(list)

  while first < last
    let middle = (first + last) >> 1
    if value <= list[middle].x
      last = middle
    else
      first = middle + 1

  min = list[last - 1]
  max = list[last]

  let t = safediv(value - min.x, max.x - min.x)
  return lerp(min.y, max.y, t)
