//-file:declared-never-used
//-file:named-like-should-return
//-file:all-paths-return-value
//-file:result-not-utilized
//-file:bool-lambda-required

function test_warnings() {
  let arr = [1, 2, 3]
  let tbl = {a = 1, b = 2}

  // ===== SHOULD WARN =====

  // map without return
  let _m1 = arr.map(@(v) print(v))

  // filter without return
  let _f1 = arr.filter(function(v) { print(v) })

  // reduce without return
  let _r1 = arr.reduce(function(acc, v) { print(acc + v) })

  // findindex without return
  let _fi1 = arr.findindex(function(v) { print(v) })

  // findvalue without return
  let _fv1 = arr.findvalue(function(v) { print(v) })

  // sort without return
  arr.sort(function(a, b) { print(a + b) })

  // apply without return
  arr.apply(function(v) { print(v) })

  // table map without return
  let _tm1 = tbl.map(function(v) { print(v) })

  // table filter without return
  let _tf1 = tbl.filter(function(v) { print(v) })

  // partial return (not all paths)
  let _m2 = arr.map(function(v) {
    if (v > 0)
      return v * 2
  })

  // function passed as binding
  let noReturnFn = function(v) { print(v) }
  let _m5 = arr.map(noReturnFn)

  // binding with partial return
  let partialFn = function(v) {
    if (v > 0)
      return v * 2
  }
  let _f5 = arr.filter(partialFn)


  // ===== SHOULD NOT WARN =====

  // map with return
  let _m3 = arr.map(@(v) v * 2)

  // filter with return
  let _f3 = arr.filter(@(v) v > 1)

  // reduce with return
  let _r3 = arr.reduce(@(acc, v) acc + v)

  // findindex with return
  let _fi3 = arr.findindex(@(v) v == 2)

  // findvalue with return
  let _fv3 = arr.findvalue(@(v) v == 2)

  // sort with return
  arr.sort(@(a, b) a <=> b)

  // apply with return
  arr.apply(@(v) v * 2)

  // block body with return on all paths
  let _m4 = arr.map(function(v) {
    if (v > 0)
      return v * 2
    else
      return v
  })

  // each (not in the list, should not warn)
  arr.each(@(v) print(v))

  // binding with proper return
  let goodFn = @(v) v * 2
  let _m6 = arr.map(goodFn)
}
