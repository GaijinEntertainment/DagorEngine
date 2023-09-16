let arr = [123, 567]
let [a, b = 22, c = 11] = arr
println(a) // => 123
println(b) // => 567
println(c) // => 567

let function foo() {
  return {x = 555, y=777, z=999, w=111}
}
let {x, y=1, q=3} = foo()
println(x) // => 555
println(y) // => 777
println(q) // => 3
