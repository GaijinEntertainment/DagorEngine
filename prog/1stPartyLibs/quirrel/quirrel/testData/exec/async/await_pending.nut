from "async" import Future

// Pending future: outer awaits, parks. A separate async function later
// resolves the future, waking the outer task.

let p = Future()

async function resolver() {
  print("resolver runs\n")
  p.resolve("hello")
}

async function consumer() {
  print("consumer waits\n")
  let x = await p
  print("consumer got: ")
  print(x)
  print("\n")
}

consumer()
resolver()
print("script done\n")
