// Test helpers for FRP test suite

//-file:plus-string

local fmtVal
fmtVal = function(v) {
  let t = type(v)
  if (t == "array")
    return $"[{", ".join(v.map(fmtVal))}]"
  if (t == "table") {
    let keys = v.keys().sort()
    return "{" + ", ".join(keys.map(@(k) $"{k}={fmtVal(v[k])}")) + "}"
  }
  if (t == "string")
    return v
  if (v == null)
    return "null"
  return v.tostring()
}

return {
  fmtVal
}
