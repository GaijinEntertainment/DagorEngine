
function dynamic_content(o) {
  assert(type(o) == "table", $"expected table in dynamic_content(), got '{type(o)}'")
  o.__dynamic_content__ <- true
  return o
}

return { dynamic_content }
