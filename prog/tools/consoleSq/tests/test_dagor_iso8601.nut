// Squirrel module under test: dagor.iso8601 (registered by sqratIso8601Time.cpp)

from "dagor.iso8601" import parse_msec, parse_unix_time, format_msec, format_unix_time


// ============================================================
// Section 1: format_unix_time round-trip
// ============================================================
println("--- Section 1: format_unix_time round-trip ---")

// 2020-01-15T10:30:45Z = 1579084245
let ts = 1579084245
let formatted = format_unix_time(ts)
assert(typeof formatted == "string", "format_unix_time returns string")
assert(formatted == "2020-01-15T10:30:45Z", $"format_unix_time: {formatted}")

let back = parse_unix_time(formatted)
assert(back == ts, $"parse_unix_time round-trip: {back} vs {ts}")

println("Section 1 PASSED")


// ============================================================
// Section 2: format_msec round-trip
// ============================================================
println("--- Section 2: format_msec round-trip ---")

let tsMsec = 1579084245123
let fmtMsec = format_msec(tsMsec)
assert(typeof fmtMsec == "string", "format_msec returns string")
let backMsec = parse_msec(fmtMsec)
// format_msec produces ISO8601 with millisecond precision; parse_msec returns it
assert(backMsec == tsMsec, $"parse_msec round-trip: {backMsec} vs {tsMsec}")

println("Section 2 PASSED")


// ============================================================
// Section 3: known UTC timestamp pairs
// ============================================================
println("--- Section 3: known UTC timestamp pairs ---")

// 2000-01-01T00:00:00Z = 946684800
let y2k = format_unix_time(946684800)
assert(y2k == "2000-01-01T00:00:00Z", $"y2k format: {y2k}")
let y2kBack = parse_unix_time(y2k)
assert(y2kBack == 946684800, $"y2k parse: {y2kBack}")

println("Section 3 PASSED")


// ============================================================
// Section 4: incorrect input
// ============================================================
println("--- Section 4: incorrect input ---")

// parse_msec returns null for unparseable strings (not throw)
let badParse = parse_msec("totally not iso8601")
assert(badParse == null, $"parse_msec invalid -> null: {badParse}")

let badParse2 = parse_unix_time("xxxxxxx")
assert(badParse2 == null, $"parse_unix_time invalid -> null: {badParse2}")

local threw = false
try {
  format_unix_time("not an int")
} catch (e) {
  threw = true
}
assert(threw, "format_unix_time with non-int must throw")

threw = false
try {
  parse_msec(123)
} catch (e) {
  threw = true
}
assert(threw, "parse_msec with non-string must throw")

println("Section 4 PASSED")


println("ALL TESTS PASSED")
