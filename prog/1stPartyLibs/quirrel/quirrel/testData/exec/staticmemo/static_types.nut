class A {
  function fn() { return 444 }
}

let a = A()

println(static null)
println(static true)
println(static 123)
println(static 123.25)
println(static "asdf")
println((static [678])[0])
println((static {a = 987})["a"])
println("closure" == typeof static println)
println(static println("print"))
println(typeof static A)
println(typeof static a)
println(typeof static a.fn)

function t()
{
  println(static a.fn())
}

t()
t()
