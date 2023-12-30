let { get_time_msec } = require("dagor.time")
let {memoize} = require("%sqstd/functools.nut")
let {Point2} = require("dagor.math")
let {Watched, Computed} = require("%sqstd/frp.nut")
let { log } = require("%sqstd/log.nut")()

const N = 5000
/*
function stest(fn, pref=""){
  let ct = get_time_msec()
  for (local i=0; i<N; i++) {
    fn()
  }
  let rest = get_time_msec()-ct
  log($"{pref}took time:{rest/1000.0}")
}
let table = array(20).reduce(function(res, v, i) {
  res[i]<-i
  return res
}, {})

let redu = @() table.reduce(@(res, v, k) res.append(k*v), [])
let pairs = @() table.topairs().map(@(v) v[1]*v[0])
//let keys = @() table.keys().map(@(v) v*v)
stest(redu, "reduce")
stest(pairs, "pairs")
*/
function test(fn, pref="", arg=null){
  let args = [null].extend(arg ?? [])
  let ct = get_time_msec()
  for (local i=0; i<N; i++) {
    fn.acall(args)
  }
  let rest = get_time_msec()-ct
  log($"{pref}took time:{rest/1000.0}")
}

let INDX = {}
function testOne(fn, pref="", arg=null){
  local ct
  if (arg != INDX) {
    ct = get_time_msec()
    for (local i=0; i<N; i++) {
      fn(arg)
    }
  }
  else {
    ct = get_time_msec()
    for (local i=0; i<N; i++) {
      fn(i)
    }
  }
  let rest = get_time_msec()-ct
  log($"{pref}took time:{rest/1000.0}")
}

function bench(func, prefix, args){
  let t  = func.getfuncinfos().parameters.len() > 2 ? test : testOne
  t(func, prefix, args)
  t(memoize(func),$"memoized {prefix}", args)
  println("")
}

let t1= @(i) {[i] = i}
bench(t1,"table creation: ", false)

let s2 = @(v, s) [$"{v}_{s}"]
bench(s2,"string 2: ", [1, 2])
println("")
let s1 = @(s) {
  a = $"{s}"
}
bench(s1,"string: ", false)

let s = memoize(s1)
assert (s(1) == s(2-1), "error on memoizing")
let sm2 = memoize(s2)
assert(sm2(0,1) == sm2(2-2, 2-1), "error on memoizing 2 args")

println("")
bench(@(_) Point2(),"point2: ", false)

println("test cache populating")
bench(@(i) i,"call", INDX)

println("")
bench(@(_) Watched(),"watched: ", false)

println("")
let v = Watched()
bench(@(_) Computed(@() v.value ),"Computed: ", [false])
