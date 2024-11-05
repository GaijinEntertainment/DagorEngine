/*

This test shows that it is possible to optimize map().filter().map().each() into loop
Which is faster

*/

let {clock} = require("datetime")

function profile_it(cnt, f) {//for quirrel version
  local res = 0
  for (local i = 0; i < cnt; ++i) {
    let start = clock()
    f()
    let measured = clock() - start
    if (i == 0 || measured < res)
      res = measured
  }
  return res / 1.0
}

//prepare testdata
let testdata = freeze(array(17*120005).reduce(function(a, _, i) { a[i]<-{key = i}; return a;}, {}))
function log(d){
  foreach(k, v in d)
    println($"{k} = {v}")
}


// 'classic' functional code
let test_filterampreduce = @() testdata
  .map(@(v) v.key) //extract func
  .filter(@(r) r%17==0) //filter func
  .map(@(r, i) (r*1000-i)/30)
  .reduce(@(res, v) res+(v%2==0 ? 1 : -1)*v, 0) //reduce func

// 'fully unrolled' functional code. can be theoritically done for function that are declared inplace, like code above
function test_loop_max() {
  local res = 0
  foreach (i, v in testdata) {
    local r = v.key
    if (r%17 != 0)
      continue
    r = (r*1000 - i)/30
    res += (r%2==0 ? 1 : -1)*r
  }
  return res
}

// 'partially unrolled' functional code (can be use for any functions used in map\filter\reduce\each hidden loops)
function test_loop_func() {
  local res = 0
  let extract = @(v) v.key
  let filter = @(v) v%17==0
  let map2 = @(v, i) (v*1000-i)/30
  let reduce = @(res, v) res+(v%2==0 ? 1 : -1)*v
  foreach (i, v in testdata) {
    local r = extract(v)
    if (!filter(r))
      continue
    r = map2(r, i)
    res = reduce(res, r)
  }
  return res
}
const numTests = 20
const iters = 200000
let profile = @(name, func) println("".concat(name, ", ", profile_it(numTests, func), $", {numTests}"))

assert(test_filterampreduce() == test_loop_max() && test_filterampreduce() == test_loop_func())
profile("\"map_filter_map_reduce\"", test_filterampreduce)
profile("\"test_loop_max\"", test_loop_max)
profile("\"test_loop_func\"", test_loop_func)
