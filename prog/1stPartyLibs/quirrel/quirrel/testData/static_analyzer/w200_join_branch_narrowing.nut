// Narrowing learned in one branch must not survive the control-flow join.

function narrowsInThen(arr, s, cond, f) {  // w200: x nullable via the else path
  local x = arr.indexof(s)
  if (cond) {
    if (x == null)
      return 0
    f(1)
  } else {
    f(2)
  }
  return x + 1
}

function narrowsNoElse(arr, s, cond) {     // w200: no else, join stays nullable
  local x = arr.indexof(s)
  if (cond) {
    if (x == null)
      return 0
  }
  return x + 1
}

function narrowsInElse(arr, s, cond, f) {  // w200: mirror of the first case
  local x = arr.indexof(s)
  if (cond) {
    f(1)
  } else {
    if (x == null)
      return 0
    f(2)
  }
  return x + 1
}

function narrowsInTry(arr, s) {            // w200: try-body narrowing dies on the catch path
  local x = arr.indexof(s)
  try {
    if (x == null)
      throw "e"
  } catch (e) {
  }
  return x + 1
}

function narrowsBothPaths(arr, s, cond) {  // ok: both paths guarantee non-null
  local x = arr.indexof(s)
  if (cond) {
    if (x == null)
      return 0
  } else {
    if (x == null)
      return 1
  }
  return x + 1
}

let _keep = [narrowsInThen, narrowsNoElse, narrowsInElse, narrowsInTry, narrowsBothPaths]
