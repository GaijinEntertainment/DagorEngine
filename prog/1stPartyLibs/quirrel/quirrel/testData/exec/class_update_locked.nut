class A {
  x = 1
}
A()

try {
  A.__update({ y = 2 })
  println("no error")
} catch (e) {
  println(e)
}
println(A.rawin("y"))
