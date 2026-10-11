function takeTables(...: table) {
  return vargv.len()
}

local obj = {}
let objRef = obj.weakref()
try {
  takeTables(obj, 1)
}
catch (e) {
  println(e)
}
obj = null
// the failed call must not keep the vararg it already took
println($"freed: {objRef.ref() == null}")
