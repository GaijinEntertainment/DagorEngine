from "async" import delay

// delay(0) still parks: each await yields control. The drain loop's
// per-iteration timer sweep fires the timer on the next pollOnce, so
// "a", "b", "c" interleave with control-yield boundaries.

async function main() {
  println("a")
  await delay(0)
  println("b")
  await delay(0)
  println("c")
}

println("start")
main()
println("end")
