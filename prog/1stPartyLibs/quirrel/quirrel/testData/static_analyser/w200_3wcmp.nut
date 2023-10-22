

local function _itemsSorter(a, b) { // -return-different-types
    a = a?.item
    b = b?.item
    if (!a || !b)
      return a <=> b
    return 42
  }