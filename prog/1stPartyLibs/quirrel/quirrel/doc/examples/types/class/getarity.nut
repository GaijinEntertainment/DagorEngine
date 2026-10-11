class Plain { x = 1 }
println("Plain.getarity() =", Plain.getarity())

class Scaler {
  function _call(_env, value, factor = 2) { return value * factor }
}
let { num_params, num_required, varargs } = Scaler.getarity()
println($"Scaler: num_params={num_params} num_required={num_required} varargs={varargs}")
println("Scaler()(5) =", Scaler()(5))
