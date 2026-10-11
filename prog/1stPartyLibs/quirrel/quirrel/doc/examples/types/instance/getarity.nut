class Plain { x = 1 }
println("Plain().getarity() =", Plain().getarity())

class Blaster {
  function _call(_env, target, power = 1) { return target != null && power > 0 }
}
let { num_params, num_required, varargs } = Blaster().getarity()
println($"Blaster(): num_params={num_params} num_required={num_required} varargs={varargs}")
println("Blaster()(\"tank\") =", Blaster()("tank"))
