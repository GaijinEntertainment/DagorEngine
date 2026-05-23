from "async" import Promise

// A bare Promise that is "locked" against further settlement silently ignores
// extra .resolve()/.reject() calls. Locked has two flavors:
//   - after_adopt: adopted another Promise (L_Adopted); inner's settlement wins
//   - after_terminal: already fulfilled or rejected; terminal is final
// Matches JS spec for the Promise Resolution Procedure on a locked promise.

async function section_after_adopt() {
  print("=== after_adopt ===\n")
  let p = Promise()
  let q = Promise()
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
  let p = Promise()
  p.resolve(1)
  print("first resolve, p=" + p.getState() + "\n")
  p.resolve(2)         // no-op
  p.reject("err")      // no-op too -- terminal is locked-in
  print("after extra calls p=" + p.getState() + "\n")
  let v = await p
  print("awaited value: " + v + "\n")
  // Same for a rejected promise.
  let r = Promise()
  r.reject("first")
  r.resolve(99)
  r.reject("second")
  print("r after extras=" + r.getState() + "\n")
  try {
    let rv = await r
    print("BUG: r resolved with " + rv + "\n")
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
