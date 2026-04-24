const function non_pure() { println("Non-pure, can be declared as constant, but can't be called as constant initializer") }
non_pure()


const double = @[pure](x) x*2
const quad = const @[pure](x) double(double(x))
const r0 = quad(5)
println(r0)

const square = function [pure](x) {
  return x*x
}
const r1 = square(5)
println(r1)

const greet = const @[pure](name = "world") $"Hello, {name}"
println(greet())
println(greet("Kitty"))

const function [pure] cube(x) {
  return x*x*x
}
println(cube(3))

const function [pure] add(a, b) {
  return a + b
}
println(add(10, 20))

global const function [pure] multiply(x, y) {
  return x * y
}
println(multiply(4, 5))


const function[pure] container(x) {
  function nested(y) {
    return x*y
  }
  return nested(5)
}

const z = container(111)
println(z)

const table = {
  function foo() {}
  bar = @() 12345
}

println(table.bar())
