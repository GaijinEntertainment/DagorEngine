from "async" import Future

// Three async tasks all await the same Pending future. When the resolver
// settles it, resolveTaskOrManual walks the waiters FIFO and schedules a
// Send step for each. Each consumer must wake with the same resolved value.

let p = Future()

async function consumer1() {
  print("c1 waits\n")
  let x = await p
  print("c1 got: ")
  print(x)
  print("\n")
}

async function consumer2() {
  print("c2 waits\n")
  let x = await p
  print("c2 got: ")
  print(x)
  print("\n")
}

async function consumer3() {
  print("c3 waits\n")
  let x = await p
  print("c3 got: ")
  print(x)
  print("\n")
}

async function resolver() {
  print("resolver runs\n")
  p.resolve("V")
}

consumer1()
consumer2()
consumer3()
resolver()
print("script done\n")
