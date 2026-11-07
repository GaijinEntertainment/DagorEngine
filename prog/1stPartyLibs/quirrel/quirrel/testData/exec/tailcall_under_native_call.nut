// closure.call() does a tail call; under a native call it must still run the callee

function count(...) {
  return vargv.len()
}

function addDefault(a, b = 10) {
  return a + b
}

function fail() {
  throw "fail"
}

println(count.call(null, 1, 2))
println(count.call.call(count, null, 1, 2))
println(count.call.pcall(count, null, 1, 2, 3))
println(count.call.acall([count, null, 1]))
println(addDefault.call.call(addDefault, null, 1))

local sum = 0
for (local i = 0; i < 100; i++)
  sum += count.call.call(count, null, i)
println(sum)

try {
  fail.call.call(fail, null)
} catch (e) {
  println($"caught: {e}")
}
