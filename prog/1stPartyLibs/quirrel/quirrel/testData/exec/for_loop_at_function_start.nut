// for-loops emitting the function's first instructions: the condition JZ
// lands at position 0 and must still be patched; the increment block is
// extracted starting from position 0 and must run after the body, not before.

function condFirst(b) {
  for (; b; )
    return 1
  return 2
}
println(condFirst(false))
println(condFirst(true))

local i = 0
function modifierFirst() {
  for (;; i++) {
    if (i >= 3)
      return i
    println($"iter {i}")
  }
}
println(modifierFirst())
