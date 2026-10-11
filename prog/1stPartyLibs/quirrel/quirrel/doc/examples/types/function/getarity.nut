function add(a, b = 10) { return a + b }
let { num_params, num_required, varargs } = add.getarity()
println($"add: num_params={num_params} num_required={num_required} varargs={varargs}")

let log = function(fmt, ...) { return [fmt].extend(vargv) }
println($"log: num_params={log.getarity().num_params} varargs={log.getarity().varargs}")

function callWithIndex(cb, value, index) {
  return cb.getarity().num_params >= 2 ? cb(value, index) : cb(value)
}
println(callWithIndex(@(v) v * 2, 5, 1))
println(callWithIndex(@(v, i) v * i, 5, 3))
