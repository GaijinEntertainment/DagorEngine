from "async" import delay

// Single await delay(0.05): top-level finishes before the async body
// resumes; resolved value is null.

async function main() {
  println("body start")
  let r = await delay(0.05)
  println($"body resumed: {r}")
  return r
}

println("call main")
main()
println("script done")
