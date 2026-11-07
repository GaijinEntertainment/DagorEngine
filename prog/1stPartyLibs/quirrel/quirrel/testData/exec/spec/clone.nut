let t = {
    a = 10, b = 20
}

let ct = clone t

println($"Table t->a: original = {t.a}, cloned op = {ct.a}")
println($"Table t->b: original = {t.b}, cloned op = {ct.b}")

let arr = ["a", "b", "c"]

let arr2 = clone arr

println($"Array[0]: original = {arr[0]}, cloned op = {arr2[0]}")
println($"Array[1]: original = {arr[1]}, cloned op = {arr2[1]}")
println($"Array[2]: original = {arr[2]}, cloned op = {arr2[2]}")

let s = "test"

let s1 = clone s

println($"String: orig '{s}', cloned op '{s1}'")

class C {
    x = 10
}

let obj = C()

let obj2 = clone obj

println($"Instance: orig {obj.x}, cloned op {obj2.x}")

try {
    let c1 = clone C
    println("Class: clone op -- FAILED")
} catch (e) {
    println($"Class: clone op -- OK, exception '{e}'")
}

let inum = 10
let fnum = 5.5

let inum2 = clone inum
let fnum2 = clone fnum

println($"Integer: orig {inum}, cloned op {inum2}")
println($"Float: orig {fnum}, cloned op {fnum2}")

let cls = @(x) $"x = '{x}'"

let cls1 = clone cls

println($"Closure: orig {cls(42)}, cloned op {cls1(42)}")
