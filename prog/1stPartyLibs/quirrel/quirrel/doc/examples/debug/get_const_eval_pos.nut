from "debug" import get_const_eval_pos

const function [pure] title(key) {
  let pos = get_const_eval_pos()
  if (pos != null)
    println($"{key} is used in {pos.func} at line {pos.line}")
  return key.toupper()
}

const HEADER = title("header")

function caption() {
  return const title("caption")
}

println(HEADER)
println(caption())
let key = "dynamic" // not a constant, so this call of title runs at runtime
println(title(key))
