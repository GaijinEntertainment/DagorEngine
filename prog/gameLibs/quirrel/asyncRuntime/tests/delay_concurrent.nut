from "async" import delay

// Two concurrent tasks parked on different delays; the shorter one
// resumes first regardless of launch order.

async function task(name, sec) {
  println($"[{name}] start")
  await delay(sec)
  println($"[{name}] done")
}

println("kick off")
task("A", 0.10)
task("B", 0.04)
println("main returns")
