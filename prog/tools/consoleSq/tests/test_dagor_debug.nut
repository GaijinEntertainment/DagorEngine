// Squirrel module under test: dagor.debug (registered by sqratDagorDebug.cpp)
// Skipped APIs: fatal (terminates process), screenlog (UI). logerr is also
// not invoked because csq-dev's log error callback sets has_errors and forces
// the process exit code to 1, which would mask test status.

from "dagor.debug" import debug, logerr, console_print, debug_dump_stack, get_log_filename


// ============================================================
// Section 1: debug + console_print are total
// ============================================================
println("--- Section 1: debug + console_print are total ---")

// These functions are documented to log/print without returning a value.
// We assert that calling them with valid string arguments does not throw.
debug("test_dagor_debug section 1 ok")
console_print("test_dagor_debug console_print ok")

println("Section 1 PASSED")


// ============================================================
// Section 2: debug_dump_stack returns without error
// ============================================================
println("--- Section 2: debug_dump_stack returns without error ---")

debug_dump_stack()

println("Section 2 PASSED")


// ============================================================
// Section 3: get_log_filename returns string
// ============================================================
println("--- Section 3: get_log_filename returns string ---")

let fn = get_log_filename()
assert(typeof fn == "string", $"get_log_filename type: {typeof fn}")

println("Section 3 PASSED")


// ============================================================
// Section 4: logerr is callable (not invoked to avoid setting has_errors)
// ============================================================
println("--- Section 4: logerr binding shape ---")

assert(typeof logerr == "function", $"logerr is callable: {typeof logerr}")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  debug(123)
} catch (e) {
  threw = true
}
assert(threw, "debug with non-string must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
