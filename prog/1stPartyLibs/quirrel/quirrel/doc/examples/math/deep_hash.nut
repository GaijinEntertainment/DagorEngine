from "math" import hash, deep_hash

// depth 1 behaves exactly like hash(): only the object itself is looked at
println("deep_hash([1,2,3], 1) == hash([1,2,3]) =", deep_hash([1, 2, 3], 1) == hash([1, 2, 3]))

// with enough depth, deep_hash actually compares contents
println("deep_hash([1,2,3]) == deep_hash([1,2,4]) =", deep_hash([1, 2, 3]) == deep_hash([1, 2, 4]))
println("deep_hash({a={x=1}}) == deep_hash({a={x=2}}) =", deep_hash({a = {x = 1}}) == deep_hash({a = {x = 2}}))

let depth = 201
try {
  deep_hash(5, depth) // depth must stay within 1..200
} catch (e) {
  println("deep_hash(5, depth) throws:", e)
}
