#allow-compiler-internals
let lam = @(a, b) $${ return a - b }

let xx = $${
  let x = 4
  function fn(a) { return $${return a != 4 ? a * 2 : 3} }
  println(lam(fn(x), 100))
}
println(xx)
