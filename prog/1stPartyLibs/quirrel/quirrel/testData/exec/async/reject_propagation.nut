from "async" import Promise

// Awaiting a rejected promise re-raises inside the async function. The
// runner uses ET_RESUME_GENERATOR_THROW to inject the rejection at the
// parked yield site; the body's surrounding try/catch catches it.

let p = Promise()
p.reject("boom")

async function main() {
  print("body start\n")
  try {
    let x = await p
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
