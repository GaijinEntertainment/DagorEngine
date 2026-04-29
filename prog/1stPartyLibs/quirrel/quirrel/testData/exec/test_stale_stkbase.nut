// Test: stack reallocation during native constructor leaves stale _stkbase
// Bug: In _OP_CALL for OT_CLASS with native constructor (OT_NATIVECLOSURE),
// CallNative -> EnterFrame may resize the stack, but there is no RELOAD_STK
// after CallNative returns. Subsequent instructions use stale _stkbase pointer.
//
// Location: sqvm.cpp, _OP_CALL, case OT_CLASS, case OT_NATIVECLOSURE (~line 1110-1116)

let {regexp} = require("string")

// Recursive function that creates regexp instances at each depth.
// At some depth, the regexp constructor's CallNative->EnterFrame triggers
// a stack reallocation. The missing RELOAD_STK means the next instruction
// after _OP_CALL uses a stale _stkbase (use-after-free).
function test(n) {
  if (n <= 0)
    return 0

  // _OP_CALL for regexp(".") goes through:
  //   OT_CLASS -> CreateClassInstance -> CallNative(ctor) -> EnterFrame -> resize!
  //   break  (NO RELOAD_STK!)
  // Next instruction reads from stale _stkbase
  local r = regexp(".")
  local v = test(n - 1)
  return v
}

test(500)
print("done\n")
