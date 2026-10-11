let arr = [1, 2, 3]
let tbl = { a = 1, b = 2, c = 3 }

local depth = 0
let filler = []

function deep(n) {
  return n == 0 ? 0 : 1 + deep(n - 1)
}

function grow() {
  depth += 400
  deep(depth)
  filler.append(array(depth, 0), array(depth * 2, 0), array(depth * 4, 0))
}

function inThread(name, fn) {
  depth = 0
  filler.clear()
  let res = newthread(fn).call()
  println($"{name}: {res}")
}

inThread("array.each", function() {
  local ok = true
  local sum = 0
  arr.each(function(v, _i, a) {
    grow()
    ok = ok && a == arr
    sum += v
  })
  return ok && sum == 6
})

inThread("array.findindex", function() {
  local ok = true
  let idx = arr.findindex(function(v, _i, a) {
    grow()
    ok = ok && a == arr
    return v == 3
  })
  return ok && idx == 2
})

inThread("array.findvalue", function() {
  local ok = true
  let val = arr.findvalue(function(v, _i, a) {
    grow()
    ok = ok && a == arr
    return v == 3
  })
  return ok && val == 3
})

inThread("array.map", function() {
  local ok = true
  let res = arr.map(function(v, _i, a) {
    grow()
    ok = ok && a == arr
    return v * 2
  })
  return ok && res.len() == 3 && res[0] == 2 && res[1] == 4 && res[2] == 6
})

inThread("array.apply", function() {
  local ok = true
  let src = [1, 2, 3]
  src.apply(function(v, _i, a) {
    grow()
    ok = ok && a == src
    return v * 2
  })
  return ok && src[0] == 2 && src[1] == 4 && src[2] == 6
})

inThread("array.filter", function() {
  local ok = true
  let res = arr.filter(function(v, _i, a) {
    grow()
    ok = ok && a == arr
    return v != 2
  })
  return ok && res.len() == 2 && res[0] == 1 && res[1] == 3
})

inThread("array.reduce", function() {
  local ok = true
  let res = arr.reduce(function(acc, v, _i, a) {
    grow()
    ok = ok && a == arr
    return acc + v
  }, 0)
  return ok && res == 6
})

inThread("table.each", function() {
  local ok = true
  local sum = 0
  tbl.each(function(v, _k, t) {
    grow()
    ok = ok && t == tbl
    sum += v
  })
  return ok && sum == 6
})

inThread("table.findindex", function() {
  local ok = true
  local calls = 0
  let key = tbl.findindex(function(_v, _k, t) {
    grow()
    ok = ok && t == tbl
    calls++
    return calls == 3
  })
  return ok && key in tbl
})

inThread("table.findvalue", function() {
  local ok = true
  local calls = 0
  let val = tbl.findvalue(function(_v, _k, t) {
    grow()
    ok = ok && t == tbl
    calls++
    return calls == 3
  })
  return ok && val >= 1 && val <= 3
})

inThread("table.map", function() {
  local ok = true
  let res = tbl.map(function(v, _k, t) {
    grow()
    ok = ok && t == tbl
    return v * 2
  })
  return ok && res.len() == 3 && res.a == 2 && res.b == 4 && res.c == 6
})

inThread("table.filter", function() {
  local ok = true
  let res = tbl.filter(function(v, _k, t) {
    grow()
    ok = ok && t == tbl
    return v != 2
  })
  return ok && res.len() == 2 && res.a == 1 && res.c == 3
})

inThread("table.reduce", function() {
  local ok = true
  let res = tbl.reduce(function(acc, v, _k, t) {
    grow()
    ok = ok && t == tbl
    return acc + v
  }, 0)
  return ok && res == 6
})
