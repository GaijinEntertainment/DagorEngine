let str = require("string")
let { format } = str

assert(format("%d", 42) == "42")
assert(format("%d", -5) == "-5")
assert(format("%i", 42) == "42")
assert(format("%o", 8) == "10")
assert(format("%u", 100) == "100")
assert(format("%x", 255) == "ff")
assert(format("%X", 255) == "FF")
assert(format("%c", 65) == "A")
assert(format("%c", 97) == "a")
assert(format("%s", "hi") == "hi")

assert(format("%5d|", 42) == "   42|")
assert(format("%-5d|", 42) == "42   |")
assert(format("%05d", 42) == "00042")
assert(format("%+d", 42) == "+42")
assert(format("%+d", -42) == "-42")

assert(format("%f", 1.5).indexof("1.5") != null)
assert(format("%.2f", 1.5) == "1.50")
assert(format("%g", 1.5).indexof("1.5") != null)
assert(format("%G", 1.5).indexof("1.5") != null)
assert(format("%e", 1000.0).indexof("e") != null)
assert(format("%E", 1000.0).indexof("E") != null)

assert(format("%%") == "%")
assert(format("%s=%d", "n", 7) == "n=7")
assert(format("%s-%s", "a", "b") == "a-b")

assert(format("%#x", 255) == "0xff")
assert(format("% d", 42) == " 42")

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

mustThrow("format.fmt",       @() format(null))
mustThrow("printf.fmt",       @() str.printf(null))
mustThrow("strip",            @() str.strip(null))
mustThrow("lstrip",           @() str.lstrip(null))
mustThrow("rstrip",           @() str.rstrip(null))
mustThrow("escape",           @() str.escape(null))
mustThrow("startswith.1",     @() str.startswith(null, "a"))
mustThrow("startswith.2",     @() str.startswith("abc", null))
mustThrow("endswith.1",       @() str.endswith(null, "a"))
mustThrow("endswith.2",       @() str.endswith("abc", null))
mustThrow("split.1",          @() str.split_by_chars(null, ","))
mustThrow("split.2",          @() str.split_by_chars("a,b", null))
mustThrow("split.3",          @() str.split_by_chars("a,b", ",", null))
mustThrow("regexp.ctor",      @() str.regexp(null))
let rex = str.regexp("a")
mustThrow("regexp.match",     @() rex.match(null))
mustThrow("regexp.search.1",  @() rex.search(null))
mustThrow("regexp.search.2",  @() rex.search("abc", null))
mustThrow("regexp.capture.1", @() rex.capture(null))
mustThrow("regexp.capture.2", @() rex.capture("abc", null))

println("ok")
