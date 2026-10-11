from "datetime" import clock
from "string" import format

function benchmark(name, runs, workload, repeats = 1) {
  local best = null
  local result = null
  for (local i = 0; i < runs; i++) {
    let start = clock()
    result = workload()
    let elapsed = clock() - start
    if (best == null || elapsed < best)
      best = elapsed
  }
  let tail = result == null ? "" : $", {result}"
  println(format("\"%s\", %.8f, %d%s", name, best / repeats, runs * repeats, tail))
}

// run_vm_bench.py format: one line per rep
function vmBench(name, workload) {
  for (local rep = 0; rep < 5; rep++) {
    let start = clock()
    let result = workload()
    println(format("BENCH %s %.3f ms (r=%s)", name, (clock() - start) * 1000, result.tostring()))
  }
}

return { benchmark, vmBench }
