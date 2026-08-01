let function lf() {            // w344: legacy style
  return 1
}
let _u1 = lf()

let class LC {}                // w344: legacy style (class form)
let _u2 = LC

local function locf() {        // ok: reassignable function is meaningful
  return 2
}
let _u3 = locf()

function plainf() {            // ok
  return 3
}
let _u4 = plainf()

let lam = function lam() {     // ok: matching explicit binding stays silent
  return 4
}
let _u5 = lam()

let anonf = function() {       // ok: anonymous expression binding
  return 5
}
let _u6 = anonf()

let function typedlf(x: int): int { // w344: annotations do not change the form
  return x
}
let _u7 = typedlf(6)

function outer() {
  let function nested() { // w344 also inside another function
    return 1
  }
  return nested
}
let _o = outer()
