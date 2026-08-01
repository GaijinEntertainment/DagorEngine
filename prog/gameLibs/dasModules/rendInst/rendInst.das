require RendInst
options indenting = 2

[generic]
def rendinst_iterateRiGenExtraResId(callback : block<(id : int) : void>) : void
  let poolsCount = rendinst_getRiGenExtraResCount()
  for id in range(poolsCount)
    callback(id)


[generic]
def rendinst_iterateRiGenExtraResId(callback : block<(id : int) : bool>) : void
  let poolsCount = rendinst_getRiGenExtraResCount()
  for id in range(poolsCount)
    if !callback(id)
      break


[generic]
def rendinst_iterateRiGenExtraResId(from : uint, callback : block<(id : int) : void>) : void
  let poolsCount = rendinst_getRiGenExtraResCount()
  for id in range(from, poolsCount)
    callback(id)
