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
let test_filtermap = @() testdata
  .map(@(v) v.key) //extract func
  .filter(@(r) r%17==0) //filter func

let test_filtermap_opt = function() {
  let mapfunc = @(v) v.key
  let filterfunc = @(v) v%17==0
  return testdata
    .map(function(v) {
      let res = mapfunc(v)
      if (!filterfunc(res))
        throw null
      return res
    })
} //extract func

let test_map = @() testdata
  .map(function(v) {
    let a = v.key
    if (a%17 != 0)
      throw null
    return a
  }) //extract func

// 'fully unrolled' functional code. can be theoritically done for function that are declared inplace, like code above
function test_loop_max() {
  let res = []
  foreach (i, v in testdata) {
    let r = v.key
    if (r%17 != 0)
      continue
    res.append(r)
  }
  return res
}

const numTests = 20
let profile = @(name, func) println("".concat(name, ", ", profile_it(numTests, func), $", {numTests}"))
let [a,b,c,d] = [test_filtermap, test_map, test_filtermap, test_filtermap_opt].map(@(f) f().reduce(@(a,b) a+b))

assert(a == b && b == c && c==d )
profile("\"plain_map_filter\"", test_filtermap)
profile("\"test_stupid_simple_optimization\"", test_filtermap_opt)
profile("\"map_replace_filter_with_throw\"", test_map)
profile("\"test_loop_max\"", test_loop_max)
