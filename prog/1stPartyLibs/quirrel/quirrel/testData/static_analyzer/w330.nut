function assignedBack(x, y) {
  x.z = y.w
  y.w = x.z
}

function letAssignedBack(b) {
  let a = b
  b = a
}

function localAssignedBack(m) {
  local h = m.p
  m.p = h
}

function notAdjacent(x, y) {
  x.z = y.w
  y.touch()
  y.w = x.z
}

function notAssignedBack(x, y) {
  x.z = y.w
  y.w = x.other
}

return [
  assignedBack,
  letAssignedBack,
  localAssignedBack,
  notAdjacent,
  notAssignedBack
]
