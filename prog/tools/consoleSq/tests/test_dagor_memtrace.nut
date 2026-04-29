// Squirrel module under test: dagor.memtrace (registered by sqratDagorDebug.cpp)

from "dagor.memtrace" import get_quirrel_object_size, get_quirrel_object_size_as_string,
  is_quirrel_object_larger_than, set_huge_alloc_threshold, dump_cur_vm,
  get_memory_allocated_kb


// ============================================================
// Section 1: get_memory_allocated_kb returns integer
// ============================================================
println("--- Section 1: get_memory_allocated_kb returns integer ---")

let kb = get_memory_allocated_kb()
assert(typeof kb == "integer", $"get_memory_allocated_kb type: {typeof kb}")
assert(kb >= 0, $"non-negative kb: {kb}")

println("Section 1 PASSED")


// ============================================================
// Section 2: get_quirrel_object_size on different value shapes
// ============================================================
println("--- Section 2: get_quirrel_object_size on different value shapes ---")

let s1 = get_quirrel_object_size("hello")
assert(typeof s1 == "table", $"size of string is table: {typeof s1}")
assert("size" in s1, "result table has 'size'")
assert(s1.size > 0, $"string total size > 0: {s1.size}")

let s2 = get_quirrel_object_size([1, 2, 3, 4, 5])
assert(typeof s2 == "table", "size of array is table")
assert(s2.size > 0, $"array total size > 0: {s2.size}")

let s3 = get_quirrel_object_size({a = 1, b = 2})
assert(typeof s3 == "table", "size of table is table")
assert(s3.size > 0, $"table total size > 0: {s3.size}")

println("Section 2 PASSED")


// ============================================================
// Section 3: get_quirrel_object_size_as_string returns string
// ============================================================
println("--- Section 3: get_quirrel_object_size_as_string returns string ---")

let s = get_quirrel_object_size_as_string([1, 2, 3])
assert(typeof s == "string", $"size as string type: {typeof s}")
assert(s.len() > 0, "size as string non-empty")

println("Section 3 PASSED")


// ============================================================
// Section 4: is_quirrel_object_larger_than
// ============================================================
println("--- Section 4: is_quirrel_object_larger_than ---")

let small = "x"
assert(is_quirrel_object_larger_than(small, 1) == true, "small > 1 byte")
assert(is_quirrel_object_larger_than(small, 1000000) == false, $"small not > 1M bytes")

println("Section 4 PASSED")


// ============================================================
// Section 5: set_huge_alloc_threshold round-trip
// ============================================================
// reset_cur_vm / reset_all are not invoked here: in builds without SqVarTrace
// they emit a logerr, and csq-dev's log handler sets has_errors and forces a
// non-zero exit code that would mask the test status.
println("--- Section 5: set_huge_alloc_threshold round-trip ---")

let prevThreshold = set_huge_alloc_threshold(1024 * 1024)
assert(typeof prevThreshold == "integer", $"set_huge_alloc_threshold returns previous: {typeof prevThreshold}")
set_huge_alloc_threshold(prevThreshold)

println("Section 5 PASSED")


// ============================================================
// Section 6: incorrect input
// ============================================================
println("--- Section 6: incorrect input ---")

local threw = false
try {
  println(is_quirrel_object_larger_than("x", "not int"))
} catch (e) {
  threw = true
}
assert(threw, "is_quirrel_object_larger_than with non-int threshold must throw")

threw = false
try {
  dump_cur_vm("not int")
} catch (e) {
  threw = true
}
assert(threw, "dump_cur_vm with non-int must throw")

println("Section 6 PASSED")


println("ALL TESTS PASSED")
