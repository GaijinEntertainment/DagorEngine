from "async" import Promise

// `await` inside for / foreach. The codegen RM_PLAIN path yields loop
// counters/iterators through their own stack slot; this test exercises both
// loop forms with an OP_YIELD inside the body so a regression in counter
// preservation across the suspension would surface here.

let p = Promise()
p.resolve("ok")

async function main() {
  for (local i = 0; i < 3; i++) {
    let v = await p
    print("for i=" + i + " v=" + v + "\n")
  }

  let arr = ["a", "b", "c"]
  foreach (idx, item in arr) {
    let v = await p
    print("foreach idx=" + idx + " item=" + item + " v=" + v + "\n")
  }
}

main()
print("script done\n")
