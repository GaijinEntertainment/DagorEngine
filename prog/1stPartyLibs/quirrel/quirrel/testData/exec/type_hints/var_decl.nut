
let i: int = 5
let t: table = {a = [1, null], b = "abc"}
let { a: array|null, b: string|null = "" } = t
let [v0: int|null = -999, v1: int|null] = a

println(i)
println(type(a), b)
println(v0, v1)
