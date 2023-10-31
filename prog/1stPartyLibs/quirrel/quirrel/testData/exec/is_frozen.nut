

let a1 = [10, 20, 30]
let a2 = freeze([10, 20, 30])


println($"Array R: {a1.is_frozen()}")
println($"Arrat F: {a2.is_frozen()}")


let t1 = { a = 10, b = 20, c = 30 }
let t2 = freeze({ a = 10, b = 20, c = 30 })

println($"Table R: {t1.is_frozen()}")
println($"Table F: {t2.is_frozen()}")


let C = class {}
let F  = freeze(class {})

println($"Class R: {C.is_frozen()}")
println($"Class F: {F.is_frozen()}")


let ri = C()
let fi = freeze(C())

println($"Instance R: {ri.is_frozen()}")
println($"Instance F: {fi.is_frozen()}")
