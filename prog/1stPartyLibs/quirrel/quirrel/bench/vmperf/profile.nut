// Takes the place of bench/quirrel/profile.nut in vmperf runs. A timed rep calls the
// workload VMPERF_INNER times, which keeps the 1 ms step of clock() small against it.
from "datetime" import clock
from "string" import format
from "system" import getenv

let INNER = (getenv("VMPERF_INNER") ?? "1").tointeger()
const REPS = 5

function run(workload) {
  local best = null
  local result = null
  for (local rep = 0; rep < REPS; rep++) {
    let start = clock()
    for (local k = 0; k < INNER; k++)
      result = workload()
    let elapsed = clock() - start
    if (best == null || elapsed < best)
      best = elapsed
  }
  println(format("VMPERF %.4f %s", best * 1000.0 / INNER, result?.tostring() ?? "null"))
}

function benchmark(_name, _runs, workload, _repeats = 1) {
  run(workload)
}

function vmBench(_name, workload) {
  run(workload)
}

return { benchmark, vmBench }
