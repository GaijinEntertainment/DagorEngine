class Sample {}

function fails(call) {
  try {
    call()
    return false
  }
  catch (_err) {
    return true
  }
}

assert(Sample.newmember("field", "value") == Sample)
assert(Sample.rawin("field"))
assert(Sample().field == "value")
assert(fails(@() Sample.newmember("lateField", 1)))

Sample.newmember("staticField", "static value", true)
assert(Sample.rawget("staticField") == "static value")
assert(fails(@() Sample.newmember.call({}, "field", "value")))

println("ok")
