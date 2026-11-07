let values = [3, 1.5, "s"]
let n = [null][0]

let ops = [
  ["+", @(a, b) a + b],
  ["-", @(a, b) a - b],
  ["*", @(a, b) a * b],
  ["/", @(a, b) a / b],
  ["%", @(a, b) a % b],
  ["&", @(a, b) a & b],
  ["|", @(a, b) a | b],
  ["^", @(a, b) a ^ b],
  ["<<", @(a, b) a << b],
  [">>", @(a, b) a >> b],
  [">>>", @(a, b) a >>> b],
]

function show(desc, f) {
  try
    println($"{desc} = {f()}")
  catch (e)
    println($"{desc}: {e}")
}

foreach ([name, op] in ops) {
  show($"null {name} null", @() op(n, n))
  foreach (v in values) {
    show($"null {name} {v}", @() op(n, v))
    show($"{v} {name} null", @() op(v, n))
  }
}

show("null + 1 (ADDI)", @() n + 1)
show("3 + 1.5", @() values[0] + values[1])
show("1.5 / 3", @() values[1] / values[0])

local x = n
show("x++ on null", @() x++)
show("++x on null", @() ++x)
show("x += 2 on null", function() { x += 2; return x })
function localInc() {
  local y = n
  y++
  return y
}
show("local y++ on null", localInc)
let t = { k = null }
show("t.k++ on null", @() t.k++)
show("t.k += 1 on null", function() { t.k += 1; return t.k })
