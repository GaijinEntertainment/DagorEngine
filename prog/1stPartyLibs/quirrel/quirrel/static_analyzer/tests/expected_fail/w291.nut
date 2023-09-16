//expect:w291

local function fn(aaa_x, _bbb, ...) {
  return aaa_x + _bbb
}

return fn
