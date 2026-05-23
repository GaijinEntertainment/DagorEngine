from "async" import Promise

// Chain-unwrap on .resolve(): outer.resolve(inner) makes `outer` mirror
// `inner`'s settlement. Three sub-cases per inner state:
//   - already_fulfilled: outer fulfills synchronously
//   - pending: outer adopts; later resolution of inner settles outer
//   - rejected: outer rejects synchronously with inner's reason

async function section_already_fulfilled() {
  print("=== already_fulfilled ===\n")
  let innerP = Promise()
  innerP.resolve(123)
  let outerP = Promise()
  outerP.resolve(innerP)
  print("outer=" + outerP.getState() + "\n")
  let v = await outerP
  print("waiter got: " + v + "\n")
}

async function section_pending() {
  print("=== pending ===\n")
  let outerP = Promise()
  let innerP = Promise()
  outerP.resolve(innerP)
  print("after outer.resolve(inner) outer=" + outerP.getState() + " inner=" + innerP.getState() + "\n")
  async function resolver() {
    innerP.resolve("late")
    print("resolver settled inner\n")
  }
  resolver()
  let v = await outerP
  print("waiter got: " + v + "\n")
}

async function section_rejected() {
  print("=== rejected ===\n")
  let innerP = Promise()
  innerP.reject("inner-failed")
  let outerP = Promise()
  outerP.resolve(innerP)
  print("outer=" + outerP.getState() + "\n")
  try {
    let v = await outerP
    print("unexpected: " + v + "\n")
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

async function runAll() {
  await section_already_fulfilled()
  await section_pending()
  await section_rejected()
  print("script done\n")
}
runAll()
