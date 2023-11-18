//expect:w227

let function foo(a, c) { //-declared-never-used
  local b = function() {
    local x = c
    local a = x
    ::print(a)
    return x
  }
  return b()
}