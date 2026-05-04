if (__name__ == "__analysis__")
  return


function foo() {}

let x =foo()
let y = x?.y
let b = {}

local _r = null

if (x && y > 0) {
  _r = b[10] / (y * 60)
}
