from "async" import Future

// Future.all fail-fast: the first faulting input settles the result with its
// value. A faulting child does not decrement the completion counter, so a later
// fulfilment cannot overwrite the fault; surplus settles are no-ops. The caught
// value is the input's thrown value (adoption preserves it).

async function main() {
  let ok = Future()
  let slow = Future()
  async function boom() { throw "input-boom" }
  // ok and slow both fulfil, but boom faults: all must fault, not fulfil.
  async function settleOthers() { ok.resolve("ok"); slow.resolve("slow") }

  let f = Future.all([ok, boom(), slow])
  settleOthers()
  try {
    let r = await f
    print("UNEXPECTED fulfil: " + r[0] + "\n")
  } catch (e) {
    print("all fail-fast: " + e + "\n")
  }
  print("script done\n")
}
main()
