// Squirrel module under test: dagor.system (registered by dagorSystem.cpp)
// Skipped APIs: exit (process-terminating), set_app_window_title (UI),
// message_box (modal UI), get_primary_screen_info (display state).

import "dagor.system" as system


// ============================================================
// Section 1: argv is a frozen array
// ============================================================
println("--- Section 1: argv is a frozen array ---")

assert(typeof system.argv == "array", $"argv type: {typeof system.argv}")
assert(system.argv.len() >= 1, $"argv has at least one element: {system.argv.len()}")
// First arg is typically the executable path
assert(typeof system.argv[0] == "string", "argv[0] is string")

local frozen = false
try {
  system.argv.append("x")
} catch (e) {
  frozen = true
}
assert(frozen, "argv is frozen")

println("Section 1 PASSED")


// ============================================================
// Section 2: get_arg_value_by_name + get_all_arg_values_by_name
// ============================================================
println("--- Section 2: get_arg_value_by_name + get_all_arg_values_by_name ---")

let v = system.get_arg_value_by_name("definitely_no_such_named_arg")
assert(v == null, $"missing named arg returns null: {v}")

let arr = system.get_all_arg_values_by_name("definitely_no_such_named_arg")
assert(typeof arr == "array", $"get_all returns array: {typeof arr}")
assert(arr.len() == 0, $"empty array for missing arg: {arr.len()}")

println("Section 2 PASSED")


// ============================================================
// Section 3: dgs_get_settings returns DataBlock
// ============================================================
println("--- Section 3: dgs_get_settings returns DataBlock ---")

let blk = system.dgs_get_settings()
assert(typeof blk == "DataBlock", $"dgs_get_settings type: {typeof blk}")
// paramCount should be a non-negative integer (or 0 if no settings).
assert(blk.paramCount() >= 0, "paramCount non-negative")

println("Section 3 PASSED")


// ============================================================
// Section 4: get_log_directory + run_in_background flag
// ============================================================
println("--- Section 4: get_log_directory + run_in_background flag ---")

let logDir = system.get_log_directory()
assert(typeof logDir == "string", $"get_log_directory type: {typeof logDir}")

let prevBg = system.get_run_in_background()
assert(typeof prevBg == "bool", "get_run_in_background bool")
system.set_run_in_background(true)
assert(system.get_run_in_background() == true, "set true round-trip")
system.set_run_in_background(false)
assert(system.get_run_in_background() == false, "set false round-trip")
system.set_run_in_background(prevBg)

println("Section 4 PASSED")


// ============================================================
// Section 5: constants
// ============================================================
println("--- Section 5: constants ---")

assert(typeof system.DBGLEVEL == "integer", "DBGLEVEL integer")
assert(typeof system.ARCH_BITS == "integer", "ARCH_BITS integer")
assert(system.ARCH_BITS == 32 || system.ARCH_BITS == 64, $"ARCH_BITS sane: {system.ARCH_BITS}")
assert(typeof system.MB_OK == "integer", "MB_OK integer constant")
assert(typeof system.MB_BUTTON_1 == "integer", "MB_BUTTON_1 integer constant")

println("Section 5 PASSED")


// ============================================================
// Section 6: incorrect input
// ============================================================
println("--- Section 6: incorrect input ---")

local threw = false
try {
  system.get_arg_value_by_name(42)
} catch (e) {
  threw = true
}
assert(threw, "get_arg_value_by_name with non-string must throw")

threw = false
try {
  system.get_all_arg_values_by_name(42)
} catch (e) {
  threw = true
}
assert(threw, "get_all_arg_values_by_name with non-string must throw")

println("Section 6 PASSED")


println("ALL TESTS PASSED")
