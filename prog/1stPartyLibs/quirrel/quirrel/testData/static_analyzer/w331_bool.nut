function sameBoolReturn(x) {
  if (x)
    return false
  return false
}

function differentBoolReturn(x) {
  if (x)
    return false
  return true
}

return [
  sameBoolReturn,
  differentBoolReturn
]
