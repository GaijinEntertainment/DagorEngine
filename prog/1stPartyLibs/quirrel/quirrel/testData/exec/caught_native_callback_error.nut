let values = [1, 2, 3, 4]

try {
  values.apply(function(value) {
    if (value == 3)
      throw "stop"
    return value * 10
  })
}
catch (err) {
  println(err)
}

class CallbackError {}

try {
  [1].apply(function(_value) {
    throw CallbackError()
  })
}
catch (CallbackError _err) {
  println("typed")
}

println("continued")
