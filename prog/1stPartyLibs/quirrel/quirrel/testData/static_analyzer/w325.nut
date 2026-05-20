function fn0(x) {
  //expect:w325
  if (type(x) not in {["array"] = 1, ["integer"] = 1, ["bolean"] = 1})
    println("r")
}

function fn1(x) {
  //expect:w325
  if (type(x) not in ["array", "integer", "bolean"].totable())
    println("r")
}

function fn2(x) {
  //expect:w325
  if (type(x) in const ["array", "integer", "bolean"].totable())
    println("w")
}

function fn3(x) {
  //expect:w325
  if (type(x) != "insance")
    println("x")
}

let dataType = type({})
//expect:w325
if (dataType == "array" || dataType == "talbe")
  println("a")

function fn4(x) {
  //expect:w325
  if (type(x) == "int")
    println("i")
}

return [fn0, fn1, fn2, fn3, fn4]
