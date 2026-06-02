from "async" import Future

// A bare Future that is "locked" against further settlement silently ignores
// extra .resolve() calls. Locked has two flavors:
//   - after_adopt: adopted another Future (L_Adopted); inner's settlement wins
//   - after_terminal: already fulfilled or faulted; terminal is final
// Matches JS spec for the Promise Resolution Procedure on a locked future.

async function section_after_adopt() {
  print("=== after_adopt ===\n")
  let p = Future()
  let q = Future()
  p.resolve(q)         // p adopts q; p is locked
  p.resolve(42)        // no-op
  p.resolve("xyz")     // no-op
  print("after double-resolve p.getState=" + p.getState() + "\n")  // still pending
  q.resolve("real")    // q settles -> p mirrors q with "real", NOT 42
  print("after q.resolve p.getState=" + p.getState() + "\n")
  let v = await p
  print("waiter got: " + v + "\n")
}

async function section_after_terminal() {
  print("=== after_terminal ===\n")
  let p = Future()
  p.resolve(1)
  print("first resolve, p=" + p.getState() + "\n")
  p.resolve(2)         // no-op
  print("after extra calls p=" + p.getState() + "\n")
  let v = await p
  print("awaited value: " + v + "\n")
  // Same for a faulted (task) future via throw-from-async.
  async function failer() { throw "first" }
  let r = failer()
  try { let _ = await r } catch (_) {}
  print("r after fault=" + r.getState() + "\n")
  try {
    let _ = await r
    print("BUG: r resolved\n")
  } catch (e) {
    print("r reason: " + e + "\n")
  }
}

async function runAll() {
  await section_after_adopt()
  await section_after_terminal()
  print("script done\n")
}
runAll()
