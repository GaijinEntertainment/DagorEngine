//expect:w228

function fn() { //-declared-never-used
  local f = 123
  local c = { f = 3 }
  return c.f
}
