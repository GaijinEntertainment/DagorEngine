// Arrays

let a = [1, 2, 3, 4]
let b = [5, 6, 7]

let aRef = a.replace_with(b)
b[0] = 999

assert(aRef == a)
assert(a[0]==5)

println("== a ==")
foreach (x in a)
  println(x)
println("== aRef ==")
foreach (x in aRef)
  println(x)
println("== b ==")
foreach (x in b)
  println(x)

// Tables

let p = {x=123, y=456}
let q = {z=777}

let pRef = p.replace_with(q)
q.z = "foo"

assert(pRef == p)
assert(p.z == 777)
assert("x" not in p)


println("== p ==")
foreach (k, v in p)
  println(k, "=", v)
println("== pRef ==")
foreach (k, v in pRef)
  println(k, "=", v)
println("== q ==")
foreach (k, v in q)
  println(k, "=", v)
