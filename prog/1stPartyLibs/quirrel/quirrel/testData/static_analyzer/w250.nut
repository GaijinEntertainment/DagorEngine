if (__name__ == "__analysis__")
  return

//expect:w250

let t = []

let _x = (::a != [])
let _y = (::a != {})
let _z = (::a != t)
let _xx = (::a == @ (v) v)

//-file:undefined-global