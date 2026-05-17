from "async" import Promise

// Awaiting an already-resolved promise: the runner sees the awaitable, finds
// it Fulfilled, and schedules the SEND step immediately (no parking).

let p = Promise()
p.resolve(42)

async function main() {
  print("body start\n")
  let x = await p
  print("body got: ")
  print(x)
  print("\n")
  return x
}

print("before call\n")
main()
print("script done\n")
