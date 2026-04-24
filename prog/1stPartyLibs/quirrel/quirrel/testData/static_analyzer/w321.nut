//-file:declared-never-used
//-file:named-like-should-return
//-file:all-paths-return-value
//-file:result-not-utilized
//-file:bool-lambda-required
//-file:callback-should-return-value
//-file:param-pos

let arr = [1, 2, 3]
let tbl = {a = 1, b = 2}

function max(a, b) { return a > b ? a : b }

// ===== SHOULD WARN =====

// reduce with assignment to accumulator
let _r1 = arr.reduce(@(a, v) a = max(v, a), 0)

// map with assignment to parameter
let _m1 = arr.map(@(v) v = v * 2)

// filter with assignment
let _f1 = arr.filter(@(v) v = v > 1)

// table reduce with assignment
let _r3 = tbl.reduce(@(a, v, _k) a = a + v, 0)


// ===== SHOULD NOT WARN =====

// block-body function: assignment to param then use is ok
let _r2 = arr.reduce(function(a, v) { a = a + v; return a }, 0)

// proper reduce (returns expression)
let _r4 = arr.reduce(@(a, v) max(v, a), 0)

// proper map
let _m4 = arr.map(@(v) v * 2)

// assignment to local, not parameter
let _m5 = arr.map(function(v) {
  local res = v * 2
  return res
})

// assignment to parameter in regular named function (not lambda)
function process(x) {
  x = x * 2
  return x
}

// assignment to outer variable, not parameter
local accumulator = 0
arr.each(function(v) {
  accumulator = accumulator + v
})
