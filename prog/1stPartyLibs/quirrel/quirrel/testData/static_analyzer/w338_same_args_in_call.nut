let math = require("math")
let { min, max, clamp } = math

let x = 5
let a = 10
let b = 20

function getVal() { return x }
function hdpx(v) { return v * 2 }
function copyRange(lo, hi) { return [lo, hi] }

// Positives: symmetric select builtins with duplicated arguments.
let deadClamp = max(10, 10)
let deadMin = min(x, x)
let deadMathMax = math.max(a, a)
let deadClampBounds = math.clamp(x, a, a)
let deadCallArgs = min(getVal(), getVal())

// Negatives: distinct arguments, the intended min-pixel-size idiom.
let good = max(10, hdpx(10))
let okMin = min(x, b)
let okClamp = clamp(x, a, b)

// Negative: unrelated 2-arg function where repeats are legitimate.
let copied = copyRange(a, a)

return { deadClamp, deadMin, deadMathMax, deadClampBounds, deadCallArgs, good, okMin, okClamp, copied }
