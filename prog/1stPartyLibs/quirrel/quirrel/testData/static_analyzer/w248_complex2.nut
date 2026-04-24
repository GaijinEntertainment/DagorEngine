function foo() {}

let a = foo()
let h = foo()

let v = a?.v
let c = h && (typeof v == "integer" || typeof v == "float")
if (!c) {
  return {
    ct = @() null
  }
}

// After early return: v is guaranteed to be integer or float (not null)

let _target = v.tointeger()  // No warning - v is known to be a number type
