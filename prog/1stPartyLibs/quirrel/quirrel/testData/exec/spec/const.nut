const a = 5
const b = "foo"
const c = null
const d = [1, 2, 3, {x=555}]
const e = b
const f = d[1]
const g = d[3].x

println(d[3].x)
println(e)
println(f)
println(g)

// Inline constant
let x = const [5,6,7,d,e]
println(x[2])

let y = 2>3 ? const [1,2,3] : const [8,9,0]
println(y[2])
