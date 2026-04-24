//-file:undefined-global

local z
function _foo(y) {
  ++z
  z--
  ::x == y
}
