let { clock, time, date } = require("datetime")

assert(typeof clock() == "float")
assert(clock() >= 0.0)

assert(typeof time() == "integer")
assert(time() > 0)

let d = date()
assert(typeof d == "table")
assert("sec" in d)
assert("min" in d)
assert("hour" in d)
assert("day" in d)
assert("month" in d)
assert("year" in d)
assert("wday" in d)
assert("yday" in d)
assert(d.sec >= 0 && d.sec <= 60)
assert(d.min >= 0 && d.min <= 59)
assert(d.hour >= 0 && d.hour <= 23)
assert(d.day >= 1 && d.day <= 31)
assert(d.month >= 0 && d.month <= 11)
assert(d.year >= 2020)
assert(d.wday >= 0 && d.wday <= 6)
assert(d.yday >= 0 && d.yday <= 365)

let t = time()
let dLocal = date(t, 'l')
let dUtc = date(t, 'u')
assert(typeof dLocal == "table")
assert(typeof dUtc == "table")
assert(dLocal.year >= 2020)
assert(dUtc.year >= 2020)

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

mustThrow("date.1", @() date(null))
mustThrow("date.2", @() date(t, null))

println("ok")
