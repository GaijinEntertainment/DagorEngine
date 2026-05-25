from "async" import Promise

// External .resolve()/.reject() on a task-promise (the promise returned by an
// async function) must throw regardless of task lifecycle. Task identity is
// latched at construction, not derived from generator presence. Three phases:
//   - inflight: task has not finished its body
//   - after_adopt: task body returned a Promise (chain-unwrap adoption)
//   - after_done: task body fully complete
// Bare Promises remain unaffected.

async function section_inflight() {
  print("=== inflight ===\n")
  async function makeTask() { return 42 }
  let task = makeTask()
  try {
    task.resolve(99)
    print("BUG: resolve no throw\n")
  } catch (e) {
    print("resolve threw: " + e + "\n")
  }
  try {
    task.reject("nope")
    print("BUG: reject no throw\n")
  } catch (e) {
    print("reject threw: " + e + "\n")
  }
  let bare = Promise()
  bare.resolve("ok")
  print("bare: " + bare.getState() + "\n")
  let r = await task
  print("consumer got: " + r + "\n")
}

async function section_after_adopt() {
  print("=== after_adopt ===\n")
  let inner = Promise()
  async function f() { return inner }   // chain-unwrap: task-promise adopts `inner`
  let task = f()
  // One suspension so f's body reaches eDead and adopts `inner`.
  let tick = Promise()
  tick.resolve(null)
  let _ = await tick
  try {
    task.resolve(99)
    print("BUG: resolve no longer throws\n")
  } catch (e) {
    print("resolve still throws: " + e + "\n")
  }
  try {
    task.reject("x")
    print("BUG: reject no longer throws\n")
  } catch (e) {
    print("reject still throws: " + e + "\n")
  }
  inner.resolve("real")
  let r = await task
  print("consumer got: " + r + "\n")
}

async function section_after_done() {
  print("=== after_done ===\n")
  async function f() { return 42 }
  let task = f()
  let r = await task
  try {
    task.resolve(99)
    print("BUG: resolve no longer throws\n")
  } catch (e) {
    print("resolve throws: " + e + "\n")
  }
  try {
    task.reject("late")
    print("BUG: reject no longer throws\n")
  } catch (e) {
    print("reject throws: " + e + "\n")
  }
  let r2 = await task
  print("after probe, task awaits to: " + r2 + "\n")
}

async function runAll() {
  await section_inflight()
  await section_after_adopt()
  await section_after_done()
  print("script done\n")
}
runAll()
