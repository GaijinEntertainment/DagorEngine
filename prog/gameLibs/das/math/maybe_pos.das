options indenting = 2
module maybe_pos shared

require math


variant MaybePos
  pos : float3
  invalid : bool


def MaybePos()
  return MaybePos(invalid = true)


def MaybePos(pos : float3)
  return MaybePos(pos = pos)


variant MaybeTm
  tm : float3x4
  invalid : bool


def MaybeTm()
  return MaybeTm(invalid = true)


def MaybeTm(tm : float3x4)
  return MaybeTm(tm = tm)
