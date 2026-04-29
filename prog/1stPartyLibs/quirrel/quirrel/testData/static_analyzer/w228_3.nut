//-file:expr-cannot-be-null

function _fn() {
  local f = 123
  local c = { f = 3 }
  return c?.f
}
