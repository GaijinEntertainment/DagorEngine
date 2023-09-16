
let function foo(a, b, c) {
  if (a) {
    local x = 10
    local z = 20
    if (b) {
      local w = 30
      local y = 40
      if (c) {
        local u = 490
        return function(q) {
          return q + u + w
        }
      }
    } else {
      return function(t) {
        return t + x
      }
    }
  }
}
