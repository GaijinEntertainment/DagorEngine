function bump(tag, value) {
  ::trace.append(tag)
  return value
}

function rhs_with_pressure(seed) {
  local a0 = seed + 1
  local a1 = a0 + 1
  local a2 = a1 + 1
  local a3 = a2 + 1
  local a4 = a3 + 1
  local a5 = a4 + 1
  local a6 = a5 + 1
  local a7 = a6 + 1
  local a8 = a7 + 1
  local a9 = a8 + 1
  return bump("rhs", a9)
}

function probe_or(lhs) {
  ::trace.clear()
  local guard = "guard"
  local result = lhs || rhs_with_pressure(10)
  assert(guard == "guard")
  return [result, clone ::trace]
}

function probe_and(lhs) {
  ::trace.clear()
  local guard = "guard"
  local result = lhs && rhs_with_pressure(20)
  assert(guard == "guard")
  return [result, clone ::trace]
}

function probe_nullc(lhs) {
  ::trace.clear()
  local guard = "guard"
  local result = lhs ?? rhs_with_pressure(30)
  assert(guard == "guard")
  return [result, clone ::trace]
}

function rhs_generator(seed) {
  ::trace.append("before-yield")
  yield seed + 1
  ::trace.append("after-yield")
  return seed + 2
}

function probe_generator(lhs) {
  ::trace.clear()
  local guard = "guard"
  local result = lhs || resume rhs_generator(50)
  assert(guard == "guard")
  return [result, clone ::trace]
}

::trace <- []

let or_short = probe_or("lhs")
assert(or_short[0] == "lhs")
assert(or_short[1].len() == 0)

let or_rhs = probe_or(null)
assert(or_rhs[0] == 20)
assert(or_rhs[1].len() == 1 && or_rhs[1][0] == "rhs")

let and_short = probe_and(null)
assert(and_short[0] == null)
assert(and_short[1].len() == 0)

let and_rhs = probe_and("lhs")
assert(and_rhs[0] == 30)
assert(and_rhs[1].len() == 1 && and_rhs[1][0] == "rhs")

let nullc_short = probe_nullc("lhs")
assert(nullc_short[0] == "lhs")
assert(nullc_short[1].len() == 0)

let nullc_rhs = probe_nullc(null)
assert(nullc_rhs[0] == 40)
assert(nullc_rhs[1].len() == 1 && nullc_rhs[1][0] == "rhs")

let gen_short = probe_generator("lhs")
assert(gen_short[0] == "lhs")
assert(gen_short[1].len() == 0)

let gen_rhs = probe_generator(null)
assert(gen_rhs[0] == 51)
assert(gen_rhs[1].len() == 1 && gen_rhs[1][0] == "before-yield")

println("ok")
