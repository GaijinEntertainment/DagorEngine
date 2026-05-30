// Awaiting a non-Future value. runStep sees the awaitable is not a Future
// instance and schedules the next step directly with the raw value as the
// Resume_Send payload, skipping any transient Future allocation.

async function main() {
  let x = await 42
  print("got int: ")
  print(x)
  print("\n")
  let s = await "hello"
  print("got str: ")
  print(s)
  print("\n")
  let n = await null
  print("got null: ")
  print(n == null ? "yes" : "no")
  print("\n")
}

main()
print("script done\n")
