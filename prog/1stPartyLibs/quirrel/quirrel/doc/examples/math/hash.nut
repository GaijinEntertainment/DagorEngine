from "math" import hash

println("hash(\"abc\") == hash(\"abc\") =", hash("abc") == hash("abc"))
println("hash(\"abc\") == hash(\"abd\") =", hash("abc") == hash("abd"))
println("hash(42) == hash(42) =", hash(42) == hash(42))

// hash() never looks past the object itself, so any two arrays (or tables)
// hash the same, whatever their size or content
println("hash([1,2,3]) == hash([\"x\"]) =", hash([1, 2, 3]) == hash(["x"]))
println("hash({a=1}) == hash({}) =", hash({a = 1}) == hash({}))

let fn = hash
try {
  hash(fn) // functions cannot be hashed
} catch (e) {
  println("hash(fn) throws:", e)
}
