// Coverage: success paths of debug introspection in sqstddebug.cpp that the
// existing tests only hit on the error side: getstackinfos(level), function
// info/decl across function shapes (fixed args, varargs, defaults, native ->
// nparamscheck_to_required_args), type_mask_to_string and resurrectunreachable.

let dbg = require("debug")

// --- getstackinfos: valid levels return a populated table -------------------
function inner() {
  let si0 = dbg.getstackinfos(0)
  let si1 = dbg.getstackinfos(1)
  assert(typeof si0 == "table")
  assert(typeof si1 == "table")
  assert("func" in si0)
  assert("line" in si0)
  assert("src" in si0)
  // far-out level returns null
  assert(dbg.getstackinfos(1000) == null)
  return si0.func
}
function outer() { return inner() }
println($"getstackinfos func: {typeof outer()}")

// --- function info / decl across argument shapes ----------------------------
// fixed arity, typed return
function fixed(a, b, c) { return a + b + c }
// varargs (nparamscheck < 0)
function variadic(a, ...) { return a }
// default parameters
function defaulted(a, b = 2, c = 3) { return a + b + c }
// typed params + return
function typed(x: int, y: float): float { return x * y }
// no params
function noargs() { return 0 }

foreach (fn in [fixed, variadic, defaulted, typed, noargs]) {
  let it = dbg.get_function_info_table(fn)
  assert(typeof it == "table")
  assert(it.native == false)
  assert(typeof it.requiredArgs == "integer")
  let ds = dbg.get_function_decl_string(fn)
  assert(ds == null || typeof ds == "string")
}

// native function info (native == true branch)
let ni = dbg.get_function_info_table(print)
assert(typeof ni == "table")
assert(ni.native == true)

println($"fixed requiredArgs: {dbg.get_function_info_table(fixed).requiredArgs}")
println($"variadic requiredArgs: {dbg.get_function_info_table(variadic).requiredArgs}")
println($"noargs requiredArgs: {dbg.get_function_info_table(noargs).requiredArgs}")

// --- type_mask_to_string for several masks ----------------------------------
println($"mask 1: {dbg.type_mask_to_string(1)}")
println($"mask 6: {dbg.type_mask_to_string(6)}")
println($"mask 0xFFFFFFFF: {dbg.type_mask_to_string(0xFFFFFFFF)}")

// --- collectgarbage / resurrectunreachable (only if GC is enabled) ----------
let info = dbg.getbuildinfo()
if (info.gc == "enabled") {
  // create an unreachable reference cycle, then sweep + resurrect
  local a = {}
  local b = {}
  a.other <- b
  b.other <- a
  a = null
  b = null
  let collected = dbg.collectgarbage()
  assert(typeof collected == "integer")
  let res = dbg.resurrectunreachable()
  assert(res == null || typeof res == "array")
  println($"gc: enabled, resurrect type {res == null ? "null" : "array"}")
} else {
  println("gc: disabled")
}

// --- decl string for native functions (doc-lookup hit and miss branches) ----
let strlib = require("string")
let ndoc = dbg.get_function_decl_string(strlib.format)  // documented native -> hit
let nplain = dbg.get_function_decl_string(print)        // native, likely no doc
assert(ndoc == null || typeof ndoc == "string")
assert(nplain == null || typeof nplain == "string")
println($"native decl string: {typeof ndoc}")

// --- format_call_stack_string with locals of many types (sqstdaux:
//     collect_stack_string / print_simple_value type switch) ----------------
function frameWithDiverseLocals() {
  local i = 42
  local f = 3.5
  local s = "text"
  local bt = true
  local nl = null
  local arr = [1, 2, 3]
  local tbl = { k = 1 }
  local fn = @(x) x
  local cls = class { x = 0 }
  local inst = cls()
  // reference them so they are not optimized away
  let sink = [i, f, s, bt, nl, arr, tbl, fn, cls, inst]
  let cs = dbg.format_call_stack_string()
  assert(typeof cs == "string" && cs.len() > 0)
  return sink.len()
}
println($"callstack locals: {frameWithDiverseLocals()}")

println("debug introspection: OK")
