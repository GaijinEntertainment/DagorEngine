from "async" import Future

// A bare Future that is already settled silently ignores extra .resolve()
// calls; the terminal state is final.

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
  await section_after_terminal()
  print("script done\n")
}
runAll()
