options indenting = 2

let VERY_BIG_NUMBER = 2147440000.0

[generic]
def BBox3(bbox : bbox3f)
  return <- BBox3(bbox.bmin.xyz, bbox.bmax.xyz)
