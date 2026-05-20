from "async" import Promise

// Pending promise: outer awaits, parks. A separate async function later
// resolves the promise, waking the outer task.

let p = Promise()

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
