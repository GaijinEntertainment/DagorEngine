//expect:w227

let function foo(a) { //-declared-never-used -unused-func-param
  local b = function() {
    local x = 1
    local a = x
    print(a)
    return x
  }
  return b()
}