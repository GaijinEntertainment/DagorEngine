// A non-class catch type is validated before any of this try's traps are armed, so it
// faults outward (to the enclosing try / error handler) instead of being swallowed by a
// sibling catch-all that would already be armed.
let NotAClass = 42

try {
  try { throw "boom" }
  catch (NotAClass e) { println("BUG specific clause ran") }
  catch (e) { println("BUG inner catch-all ran") }
}
catch (e) { println("ok: faulted out to the enclosing try") }
