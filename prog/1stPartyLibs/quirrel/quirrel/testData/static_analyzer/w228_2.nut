//expect:w228

let function fn() { //-declared-never-used
  local f = 123
  local c = { f = 3 }
  return c.f
}
