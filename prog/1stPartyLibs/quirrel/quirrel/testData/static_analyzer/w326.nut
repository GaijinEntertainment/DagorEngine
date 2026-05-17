function fn0(x) {
  //expect:w326
  if (typeof x == "bolean")
    println("b")
}

function fn1(x) {
  //expect:w326
  if (typeof(x) != "insance")
    println("i")
}

function fn2(x) {
  //expect:w326
  if ((typeof x) not in {["array"] = 1, ["talbe"] = 1})
    println("t")
}

function fn3(x) {
  //expect:w326
  if ((typeof x) in const ["array", "int"].totable())
    println("n")
}

let dataType = typeof({})
//expect:w326
if (dataType == "talbe")
  println("a")

function fn4(x) {
  if (typeof x == "Bolean")
    println("custom")
  if (typeof x == "zzzz")
    println("unknown")
  if (typeof x == "integer")
    println("valid")
}

return [fn0, fn1, fn2, fn3, fn4]
