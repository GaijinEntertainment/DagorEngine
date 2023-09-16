module maybe_pos shared

require math


variant MaybePos
  pos : float3
  invalid : void?


variant MaybeTm
  tm : float3x4
  invalid : void?
