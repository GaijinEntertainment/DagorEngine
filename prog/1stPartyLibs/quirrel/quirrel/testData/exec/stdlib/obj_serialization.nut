let { blob } = require("iostream")

function deep_compare(a, b)
{
  if (type(a) != type(b))
    return false

  if (type(a) == "array") {
    if (a.len() != b.len())
      return false

    for (local i = 0; i < a.len(); i++)
      if (!deep_compare(a[i], b[i]))
        return false

    return true
  }
  else if (type(a) == "table") {
    if (a.len() != b.len())
      return false

    local keys = []
    foreach (k, _ in a)
      keys.append(k)
    keys.sort()

    foreach (_, key in keys)
      if (!deep_compare(a[key], b[key]))
        return false

    return true
  }
  else
    return a == b
}


let a = {x = 1.0, y = [{}, {z = 5.0}]}
let b = {y = [{}, {z = 5.0}], x = 1.0}
println($"deep_compare test1: {deep_compare(a, b)}")
b.y[1].z = 5
println($"deep_compare test2: {deep_compare(a, b)}")


function test(obj, obj_type) {
  local b = blob()
  b.writeobject(obj)
  b.seek(0)
  let y = b.readobject()
  println($"test {obj_type}: {deep_compare(obj, y)}")
}

test(null, "null")

test(true, "bool1")
test(false, "bool2")

test(0, "int0")
test(-1, "int1")
test(12, "int2")
test(128, "int3")
test(32767, "int4")
test(11132767, "int5")

test(0.0, "float0")
test(0.5, "float1")
test(10.25, "float2")
test(-1e30, "float3")

test("", "str0")
test("a", "str1")
test("aaaabc", "str2")
test("aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
     "aaaabcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "str3")


test([], "array0")
test([222], "array1")
test([222, 333, null, -5.0, "www"], "array2")

test(["www", "www", "", "", "", "www", null, "www"], "array_str")

test({}, "table0")
test({xx = "xx"}, "table1")
test({[55] = "xx", [0] = 444}, "table2")

test({x = {x = {x = {x = [1, "222", 3, {y = {y = {y = [4, null, "333", "222"]}}}, true, 6, false] }}}}, "recursion")

try {
  local c = class {}
  test(c, "err1")
} catch (e) {
  println(e)
}

try {
  local c = class {}
  test([null, {x = c}], "err1")
} catch (e) {
  println(e)
}

function testx(x) {
  try {
    local c = blob()
    let y = c.readobject()
    println(y)
  } catch (e) {
    println(e)
  }
  return x + 1
}

println(testx(110))
