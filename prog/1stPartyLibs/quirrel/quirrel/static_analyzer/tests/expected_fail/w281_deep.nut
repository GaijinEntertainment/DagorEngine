//expect:w281

let function fn(arr) {
  local v = arr ? arr : []
  return v.append(1, 2)
}

return fn

