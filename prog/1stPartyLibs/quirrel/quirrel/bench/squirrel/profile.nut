function benchmark(name, runs, workload, repeats = 1) {
  local best = null
  local result = null
  for (local i = 0; i < runs; i++) {
    local start = clock()
    result = workload()
    local elapsed = clock() - start
    if (best == null || elapsed < best)
      best = elapsed
  }
  local tail = result == null ? "" : ", " + result
  print(format("\"%s\", %.8f, %d%s", name, best / repeats, runs * repeats, tail) + "\n")
}
