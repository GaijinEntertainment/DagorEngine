function fn_def(x: int, y: float = -100.0): float {
  return x * y
}


function fn(x: int, y: float): float {
  return x + y
}

function apply(func: function|null, x: int, y: float): float|null {
  return fn?(x, y)
}

function va(x: bool, ...: string|null) {
  println(x, vargv[0])
}

println(fn_def(10))
println(apply(fn, 5, 10.25))
va(true, "abc", "", null, "342")