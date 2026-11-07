#forbid-clone-operator

let t = {
    a = 10, b = 20
}

let ct = t.clone()

println($"Table t->a: original = {t.a}, cloned method {ct.a}")
println($"Table t->b: original = {t.b}, cloned method {ct.b}")

let arr = ["a", "b", "c"]

let arr1 = arr.clone()

println($"Array[0]: original = {arr[0]}, cloned method {arr1[0]}")
println($"Array[1]: original = {arr[1]}, cloned method {arr1[1]}")
println($"Array[2]: original = {arr[2]}, cloned method {arr1[2]}")

let s = "test"

let s2 = s.clone()

println($"String: orig '{s}', cloned method '{s2}'")

class C {
    x = 10
}

let obj = C()

let obj1 = obj.clone()

println($"Instance: orig {obj.x}, cloned method {obj1.x}")

try {
    let c1 = C.clone()
    println("Class: clone method -- FAILED")
} catch (e) {
    println($"Class: clone method -- OK, exception '{e}'")
}

let inum = 10
let fnum = 5.5

let inum1 = inum.clone()
let fnum1 = fnum.clone()

println($"Integer: orig {inum}, cloned method {inum1}")
println($"Float: orig {fnum}, cloned method {fnum1}")

let cls = @(x) $"x = '{x}'"

let cls2 = cls.clone()

println($"Closure: orig {cls(42)}, cloned method {cls2(42)}")
