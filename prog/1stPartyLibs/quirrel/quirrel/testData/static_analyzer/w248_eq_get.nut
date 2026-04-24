if (__name__ == "__analysis__")
  return


function foo(_p) {}
let o = {}

let u = o.value.findvalue(@(u) u.l == "999")
let _x = u?.t != "a" || foo(u) ? null
  : u.t  // No warning - reaching here means u?.t == "a", so u is non-null
