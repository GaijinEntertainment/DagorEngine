// Demonstrates SQVM::ToString -> CallMetaMethod -> Call recursion (1.6)
//
// ToString() checks for MT_TOSTRING metamethod on instance's class.
// If found, it calls CallMetaMethod() -> Call() -> Execute().
// If the _tostring body does string concatenation on 'this',
// StringCat calls ToString(this) again -> infinite recursion.
//
// Call chain:
//   "" + obj
//   -> StringCat(str, obj)
//     -> ToString(obj)
//       -> GetMetaMethod(MT_TOSTRING) -> found _tostring
//       -> CallMetaMethod(_tostring, 1, res)
//         -> Call(_tostring closure)
//           -> Execute()
//             -> "obj" + this   (inside _tostring body)
//             -> StringCat("obj", this)
//               -> ToString(this)   <-- re-enters!
//                 -> CallMetaMethod again -> ...

local depth = 0

class Recursive {
  function _tostring() {
    depth++
    // This concatenation calls StringCat -> ToString(this) -> _tostring again
    return "obj" + this
  }
}

local obj = Recursive()

try {
  let s = "" + obj
  print(s)
} catch(e) {
  println($"Caught after depth={depth}: {e}")
}
