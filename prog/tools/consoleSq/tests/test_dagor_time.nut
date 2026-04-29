// Squirrel module under test: dagor.time (registered by dagorTime.cpp)

from "dagor.time" import get_time_msec, get_time_usec, get_local_unixtime, format_unixtime,
  unixtime_to_local_timetbl, local_timetbl_to_unixtime, unixtime_to_utc_timetbl, utc_timetbl_to_unixtime, ref_time_ticks


// ============================================================
// Section 1: monotonic timers
// ============================================================
println("--- Section 1: monotonic timers ---")

let t0 = get_time_msec()
assert(typeof t0 == "integer", "get_time_msec returns integer")
let t1 = get_time_msec()
assert(t1 >= t0, $"get_time_msec monotonic: {t0} -> {t1}")

let r0 = ref_time_ticks()
let r1 = ref_time_ticks()
assert(r1 >= r0, "ref_time_ticks monotonic")

let elapsedA = get_time_usec(r0)
let elapsedB = get_time_usec(r0)
assert(typeof elapsedA == "integer" && typeof elapsedB == "integer", "get_time_usec returns integer")
assert(elapsedB >= elapsedA, $"get_time_usec(ref) monotonic: {elapsedA} -> {elapsedB}")

println("Section 1 PASSED")


// ============================================================
// Section 2: local_unixtime sane range
// ============================================================
println("--- Section 2: local_unixtime sane range ---")

let now = get_local_unixtime()
assert(typeof now == "integer", "get_local_unixtime is integer")
// Epoch >= 2025-01-01 (1735689600), <= 2100-01-01 (4102444800)
assert(now > 1735689600, $"now after 2025: {now}")
assert(now < 4102444800, $"now before 2100: {now}")

println("Section 2 PASSED")


// ============================================================
// Section 3: UTC time table round-trip
// ============================================================
println("--- Section 3: UTC time table round-trip ---")

// 2020-01-15T10:30:45Z = 1579084245
let ts = 1579084245
let utc = unixtime_to_utc_timetbl(ts)
assert(utc.year == 2020, $"utc year: {utc.year}")
assert(utc.month == 0, $"utc month (0-based Jan): {utc.month}")
assert(utc.day == 15, $"utc day: {utc.day}")
assert(utc.hour == 10, $"utc hour: {utc.hour}")
assert(utc.min == 30, $"utc min: {utc.min}")
assert(utc.sec == 45, $"utc sec: {utc.sec}")

let backTs = utc_timetbl_to_unixtime({year=2020, month=0, day=15, hour=10, min=30, sec=45})
assert(backTs == ts, $"utc round-trip: {backTs} vs {ts}")

println("Section 3 PASSED")


// ============================================================
// Section 4: local time table fields present
// ============================================================
println("--- Section 4: local time table fields present ---")

let lt = unixtime_to_local_timetbl(ts)
assert("year" in lt, "lt has year")
assert("month" in lt, "lt has month")
assert("day" in lt, "lt has day")
assert("dayOfWeek" in lt, "lt has dayOfWeek")

let backTs2 = local_timetbl_to_unixtime(lt)
assert(backTs2 == ts, $"local round-trip: {backTs2} vs {ts}")

println("Section 4 PASSED")


// ============================================================
// Section 5: format_unixtime
// ============================================================
println("--- Section 5: format_unixtime ---")

let s = format_unixtime("%Y", ts)
assert(s == "2020", $"format_unixtime year: {s}")

let s2 = format_unixtime("%m-%d", ts)    //disable: -format-arguments-count
assert(s2 == "01-15", $"format_unixtime month-day: {s2}")

println("Section 5 PASSED")


// ============================================================
// Section 6: incorrect input
// ============================================================
println("--- Section 6: incorrect input ---")

local threw = false
try {
  unixtime_to_utc_timetbl("not int")
} catch (e) {
  threw = true
}
assert(threw, "unixtime_to_utc_timetbl with non-int must throw")

threw = false
try {
  utc_timetbl_to_unixtime("not table")
} catch (e) {
  threw = true
}
assert(threw, "utc_timetbl_to_unixtime with non-table must throw")

println("Section 6 PASSED")


println("ALL TESTS PASSED")
