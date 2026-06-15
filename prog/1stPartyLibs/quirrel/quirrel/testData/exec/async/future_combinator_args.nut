from "async" import Future

// Synchronous argument validation: bad arguments throw at the call site (on the
// current frame), not as a faulted future. So a plain try/catch -- no await --
// catches them. Empty `all` is valid (fulfils with []); empty `race` throws.

function check(label, fn) {
  try { fn(); print(label + ": no throw\n") }
  catch (e) { print(label + ": " + e + "\n") }
}

check("all(42)", @() Future.all(42))
check("all()", @() Future.all())
check("race(42)", @() Future.race(42))
check("race()", @() Future.race())
check("race([])", @() Future.race([]))
check("all([])", @() Future.all([]))
print("script done\n")
