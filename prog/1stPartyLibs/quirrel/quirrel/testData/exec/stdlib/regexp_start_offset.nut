let { regexp } = require("string")

function mustThrow(label, fn) {
  try {
    fn()
    println($"FAIL: {label} accepted invalid input")
  } catch (_) {}
}

let rex = regexp("a")

// search/capture must validate optional start offset before pointer arithmetic.
mustThrow("regexp.search.negative_start", @() rex.search("ba", -1))
mustThrow("regexp.search.past_end_start", @() rex.search("ba", 3))
mustThrow("regexp.capture.negative_start", @() rex.capture("ba", -1))
mustThrow("regexp.capture.past_end_start", @() rex.capture("ba", 3))

assert(rex.search("ba", 1).begin == 1)
assert(rex.search("ba", 2) == null)
assert(rex.capture("ba", 2) == null)

println("ok")
