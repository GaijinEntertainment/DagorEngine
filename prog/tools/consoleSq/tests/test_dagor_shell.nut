// Squirrel module under test: dagor.shell (registered by dagorShell.cpp)
// shell_execute would launch external programs / open URLs / files; that
// mutates OS state and is unsafe for an unattended test. The smallest
// deterministic subset we can exercise is shell_execute's argument validation
// (called with empty/missing required keys), which triggers the throw paths
// without ever invoking os_shell_execute.

from "dagor.shell" import shell_execute //disable: -see-other


// ============================================================
// Section 1: shell_execute throws when neither file nor dir set
// ============================================================
println("--- Section 1: shell_execute throws when neither file nor dir set ---")

local threw = false
try {
  shell_execute({})
} catch (e) {
  threw = true
}
assert(threw, "empty params must throw 'file or/and dir should be set'")

threw = false
try {
  shell_execute({cmd = "open"})
} catch (e) {
  threw = true
}
assert(threw, "cmd-only params must throw")

println("Section 1 PASSED")


// ============================================================
// Section 2: shell_execute throws when cmd is empty string
// ============================================================
println("--- Section 2: shell_execute throws when cmd is empty string ---")

threw = false
try {
  shell_execute({cmd = "", file = "x"})
} catch (e) {
  threw = true
}
assert(threw, "empty cmd must throw")

println("Section 2 PASSED")


// ============================================================
// Section 3: incorrect input types
// ============================================================
println("--- Section 3: incorrect input types ---")

threw = false
try {
  shell_execute("not a table")
} catch (e) {
  threw = true
}
assert(threw, "non-table arg must throw")

threw = false
try {
  shell_execute() //disable: -param-count
} catch (e) {
  threw = true
}
assert(threw, "missing arg must throw")

println("Section 3 PASSED")


// ============================================================
// Section 4: binding shape sanity
// ============================================================
println("--- Section 4: binding shape sanity ---")

assert(typeof shell_execute == "function", $"shell_execute callable: {typeof shell_execute}")

println("Section 4 PASSED")


println("ALL TESTS PASSED")
