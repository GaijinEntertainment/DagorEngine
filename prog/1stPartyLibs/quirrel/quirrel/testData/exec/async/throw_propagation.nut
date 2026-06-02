// Awaiting a faulted future re-raises inside the awaiting async function.
// The runner uses ET_RESUME_GENERATOR_THROW to inject the thrown value at
// the parked yield site; the body's surrounding try/catch catches it.

async function faulted() {
  throw "boom"
}

async function main() {
  print("body start\n")
  try {
    let x = await faulted()
    print("unreachable\n")
  } catch (e) {
    print("caught: ")
    print(e)
    print("\n")
  }
  print("body done\n")
}

main()
print("script done\n")
