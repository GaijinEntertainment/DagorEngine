//expect:w234

local x = 0 // can be modified before calling "fn1"

function fn1() { //-declared-never-used
  print(1 / (x))
}

let y = 0 // always equal to 0

function fn2() { //-declared-never-used
  print(1 / (y))
}
