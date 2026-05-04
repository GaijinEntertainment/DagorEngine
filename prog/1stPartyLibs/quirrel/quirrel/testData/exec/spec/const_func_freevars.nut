const with_outer_vars_1 = @"
let w = 123
const function[pure] container2(x) {
  function nested(y) {
    return x*y*w //<-----
  }
  return nested(5)
}

const z2 = container2(111)
println(z)
"
try {
  compilestring(with_outer_vars_1)
} catch (e) {
  println(e)
}


const with_outer_vars_2 = @"
let w = 123
const function[pure] container3(x) {
  function nested(y) {
    return x*y
  }
  return nested(5)*w //<-----
}

const z2 = container2(111)
println(z)
"
try {
  compilestring(with_outer_vars_2)
} catch (e) {
  println(e)
}
