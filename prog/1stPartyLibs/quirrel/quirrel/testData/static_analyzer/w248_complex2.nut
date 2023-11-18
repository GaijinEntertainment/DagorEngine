function foo() {}

let a = foo()
let h = foo()

let v = a?.v
let c = h && (typeof v == "integer" || typeof v == "float")
if (!c)
  return {    ct = @() null
  }

// value != null


let _target = v.tointeger()