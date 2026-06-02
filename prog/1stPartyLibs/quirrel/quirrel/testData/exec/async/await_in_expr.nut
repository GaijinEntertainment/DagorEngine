from "async" import Future

// Multiple `await` in a single expression. Each suspension must preserve the
// partial expression evaluation state on the stack so the second OP_YIELD
// resumes with the first await's result already present. Exercises:
//   - additive composition: `await a + await b`
//   - nested call composition: `await f(await g())`

let pa = Future()
pa.resolve(10)

let pb = Future()
pb.resolve(20)

async function add() {
  let r = await pa + await pb
  print("add: " + r + "\n")
}

async function g() { return 7 }
async function f(x) { return x * 2 }

async function nested() {
  let r = await f(await g())
  print("nested: " + r + "\n")
}

async function main() {
  await add()
  await nested()
}

main()
print("script done\n")
