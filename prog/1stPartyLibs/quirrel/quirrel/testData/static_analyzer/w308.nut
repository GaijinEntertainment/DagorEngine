let o = {}

function startswith(_a, _b) { return false }

let _expected = o.findindex(@(t) t.id = 10) // EXPECTED
let _falsep = o.findvalue(@(t) t.id == 10)  // FP 1


let _sr = o.findvalue(@(s) s.s == 10 && (s?.e ?? 0) > 0 ) // FP 2

o.findvalue(@(p) o?[p] ?? (p == "000")) // FP 3


let _set = o.filter(@(a) a && startswith(a, "9")) // FP 4
