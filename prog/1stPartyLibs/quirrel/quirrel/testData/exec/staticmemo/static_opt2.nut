let fni2 = @() 335
let A8 = class { function _get(idx) { return idx + -3 }}
let ci8 = A8()
let fni11 = @() 11
let fni9 = @(p10) p10 - fni11()
let t12 = { f0=3 }
let A13 = class { function _get(idx) { return idx + 3 }}
let ci13 = A13()
let t7 = { f0=(((static(ci8[((298) | (10 - -3))]) == 999)) ? (10) : (t12["f0"])) f1=ci13[2] }
println(t7.f0)
println(t7.f1)
