

let t = {
    a = 10, b = 20
}

let ct0 = clone t

#forbid-clone-operator

let ct1 = t.clone()

#allow-clone-operator

let ct2 = clone t

#forbid-clone-operator

let ct3 = t.clone()

println($"Table t->a: original = {t.a}, cloned op = {ct2.a}, cloned method {ct3.a}")
println($"Table t->b: original = {t.b}, cloned op = {ct2.b}, cloned method {ct3.b}")

let arr = ["a", "b", "c"]

let arr1 = arr.clone()

#allow-clone-operator

let arr2 = clone arr


println($"Array[0]: original = {arr[0]}, cloned op = {arr2[0]}, cloned method {arr1[0]}")
println($"Array[1]: original = {arr[1]}, cloned op = {arr2[1]}, cloned method {arr1[1]}")
println($"Array[2]: original = {arr[2]}, cloned op = {arr2[2]}, cloned method {arr1[2]}")

let s = "test"

let s1 = clone s

#forbid-clone-operator
let s2 = s.clone()

println($"String: orig '{s}', cloned op '{s1}', cloned method '{s2}'")

class C {
    x = 10
}

let obj = C()

let obj1 = obj.clone()

#allow-clone-operator

let obj2 = clone obj

println($"Instance: orig {obj.x}, cloned op {obj2.x}, cloned method {obj1.x}")

try {
    let c1 = clone C
    println("Class: clone op -- FAILED")
} catch (e) {
    println($"Class: clone op -- OK, exception '{e}'")
}

#forbid-clone-operator

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

#allow-clone-operator

let inum2 = clone inum
let fnum2 = clone fnum

println($"Integer: orig {inum}, cloned op {inum2}, cloned method {inum1}")
println($"Float: orig {fnum}, cloned op {fnum2}, cloned method {fnum1}")

let cls = @(x) $"x = '{x}'"

let cls1 = clone cls

#forbid-clone-operator

let cls2 = cls.clone()

println($"Closure: orig {cls(42)}, cloned op {cls1(42)}, cloned method {cls2(42)}")
