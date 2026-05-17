from "async" import nextFrame

// `await nextFrame()` parks the coroutine until the next pollOnce. It must
// not resolve synchronously; "end" must print before "after" because the
// async function yields back to the caller before the dispatcher tick.

async function main() {
  println("before")
  let v = await nextFrame()
  println($"after, v = {v}")
}

println("start")
main()
println("end")
