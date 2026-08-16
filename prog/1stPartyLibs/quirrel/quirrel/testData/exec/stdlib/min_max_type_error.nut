let { min, max } = require("math")

function getError(call) {
  try {
    call()
  }
  catch (err) {
    return err
  }
  return null
}

println(getError(@() min(1, "x")))
println(getError(@() min(1, 2, "x")))
println(getError(@() max(1, 2, 3, "x")))
