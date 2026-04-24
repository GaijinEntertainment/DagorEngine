let debugLib = require("debug")
let { getlocals, format_call_stack_string, script_watchdog_kick, set_script_watchdog_timeout_msec, get_function_info_table, seterrorhandler, setdebughook } = debugLib

function testLocals() {
  local a = 1
  local b = "hello"
  local c = [1, 2, 3]
  let locals = getlocals(1)
  assert(typeof locals == "table")
  assert(locals.a == 1)
  assert(locals.b == "hello")
  assert(locals.c.len() == 3)
}
testLocals()

let prev = set_script_watchdog_timeout_msec(1000)
assert(typeof prev == "integer")
script_watchdog_kick()
set_script_watchdog_timeout_msec(prev)

function targetFn(x: int, y: int): int { return x + y }
let info = get_function_info_table(targetFn)
assert(typeof info == "table")
assert(info.functionName == "targetFn")
assert(info.native == false)
assert(typeof info.requiredArgs == "integer")

let s = format_call_stack_string()
assert(typeof s == "string")
assert(s.len() > 0)

function myErrorHandler(_err) {}
seterrorhandler(myErrorHandler)
seterrorhandler(null)

function myDebugHook(_event, _src, _line, _name) {}
setdebughook(myDebugHook)
setdebughook(null)

try {
  seterrorhandler("bad")
  println("FAIL: seterrorhandler accepted string")
} catch (e) {
  assert(typeof e == "string")
}

try {
  setdebughook(42)
  println("FAIL: setdebughook accepted int")
} catch (e) {
  assert(typeof e == "string")
}

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

mustThrow("getstackinfos",                  @() debugLib.getstackinfos(null))
mustThrow("getlocals.1",                    @() getlocals(null))
mustThrow("getlocals.2",                    @() getlocals(1, null))
mustThrow("set_script_watchdog_timeout_msec", @() set_script_watchdog_timeout_msec(null))
mustThrow("get_function_decl_string",       @() debugLib.get_function_decl_string(null))
mustThrow("type_mask_to_string",            @() debugLib.type_mask_to_string(null))
mustThrow("get_function_info_table",        @() get_function_info_table(null))
mustThrow("doc",                            @() debugLib.doc(null))

println("ok")
