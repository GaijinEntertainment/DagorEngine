from "types" import Table

function dynamic_content(o) {
  assert(o instanceof Table, $"expected table in dynamic_content(), got '{type(o)}'")
  o.__dynamic_content__ <- true
  return o
}

return { dynamic_content }
