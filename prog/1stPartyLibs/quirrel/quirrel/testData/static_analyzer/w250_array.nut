if (__name__ == "__analysis__")
  return

//-file:undefined-global

let t = []

let _x = (::a != [])
let _y = (::a != {})
let _z = (::a != t)
let _xx = (::a == @ (v) v)
